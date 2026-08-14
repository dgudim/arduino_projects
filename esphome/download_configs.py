#!/usr/bin/env python3
from __future__ import annotations

import argparse
import asyncio
import json
import os
import sys
import uuid
from io import StringIO
from pathlib import Path
from urllib.parse import urlparse, urlunparse

import websockets
from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq, TaggedScalar

DEFAULT_URL = "http://100.108.149.90:6052/"
OUTPUT_DIR = Path(__file__).resolve().parent / "configs"
PLACEHOLDER = "********"


def http_to_ws(url: str) -> str:
    parsed = urlparse(url)
    scheme = "wss" if parsed.scheme == "https" else "ws"
    path = parsed.path.rstrip("/") + "/ws"
    return urlunparse((scheme, parsed.netloc, path, "", "", ""))


def _yaml() -> YAML:
    yaml = YAML()
    yaml.preserve_quotes = True
    return yaml


def _blur_node(node):
    if isinstance(node, CommentedMap):
        for key in node:
            node[key] = _blur_node(node[key])
        return node
    if isinstance(node, CommentedSeq):
        for i in range(len(node)):
            node[i] = _blur_node(node[i])
        return node
    if isinstance(node, dict):
        return {key: _blur_node(value) for key, value in node.items()}
    if isinstance(node, list):
        return [_blur_node(value) for value in node]
    return PLACEHOLDER


def blur_secrets_yaml(text: str) -> str:
    """Replace every secret value with a placeholder; keep keys and comments."""
    yaml = _yaml()
    blurred = _blur_node(yaml.load(text))
    stream = StringIO()
    yaml.dump(blurred, stream)
    return stream.getvalue()


def _collect_includes(node, found: list[str]) -> None:
    tag = getattr(node, "tag", None)
    if tag == "!include":
        path = ""
        if isinstance(node, TaggedScalar):
            path = str(node.value).strip()
        elif isinstance(node, (CommentedMap, dict)):
            file_value = node.get("file")
            if file_value is not None:
                path = str(file_value).strip()
        if path and path not in found:
            found.append(path)

    if isinstance(node, (CommentedMap, dict)):
        for value in node.values():
            _collect_includes(value, found)
    elif isinstance(node, (CommentedSeq, list)):
        for value in node:
            _collect_includes(value, found)


def find_includes(text: str) -> list[str]:
    found: list[str] = []
    try:
        data = _yaml().load(text)
    except Exception:
        return found
    _collect_includes(data, found)
    return found


def unwrap_config(result: object) -> str:
    if isinstance(result, str):
        return result
    if isinstance(result, dict):
        for key in ("content", "yaml", "config"):
            value = result.get(key)
            if isinstance(value, str):
                return value
    raise TypeError(f"Unexpected config payload: {type(result).__name__}")


class DeviceBuilderClient:
    def __init__(self, ws) -> None:
        self._ws = ws

    @classmethod
    async def connect(cls, http_url: str):
        ws_url = http_to_ws(http_url)
        ws = await websockets.connect(ws_url)
        await ws.recv()  # ServerInfoMessage handshake
        return cls(ws)

    async def close(self) -> None:
        await self._ws.close()

    async def command(self, command: str, args: dict | None = None) -> object:
        message_id = str(uuid.uuid4())
        await self._ws.send(
            json.dumps({"command": command, "message_id": message_id, "args": args or {}})
        )
        while True:
            payload = json.loads(await self._ws.recv())
            if payload.get("message_id") != message_id:
                continue
            if "error_code" in payload:
                details = payload.get("details") or payload.get("error_code")
                raise RuntimeError(f"{command}: {details}")
            if "event" in payload:
                continue
            return payload.get("result")

    async def get_config(self, configuration: str) -> str:
        return unwrap_config(await self.command("devices/get_config", {"configuration": configuration}))


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not content.endswith("\n"):
        content += "\n"
    path.write_text(content, encoding="utf-8")


async def download(args: argparse.Namespace) -> int:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    client = await DeviceBuilderClient.connect(args.url)
    try:
        devices = await client.command("devices/list")
        configured = devices.get("configured", []) if isinstance(devices, dict) else []
        if not configured:
            print("No configured devices found.", file=sys.stderr)
            return 1

        queued: list[str] = []
        seen: set[str] = set()
        for device in configured:
            name = device.get("configuration")
            if isinstance(name, str) and name not in seen:
                queued.append(name)
                seen.add(name)

        print(f"Found {len(queued)} device config(s).")

        while queued:
            configuration = queued.pop(0)
            try:
                content = await client.get_config(configuration)
            except RuntimeError as err:
                print(f"skip {configuration}: {err}", file=sys.stderr)
                continue

            dest = OUTPUT_DIR / configuration
            write_text(dest, content)
            print(f"wrote {dest}")

            for include in find_includes(content):
                if include not in seen and ".." not in Path(include).parts:
                    queued.append(include)
                    seen.add(include)

        try:
            raw_secrets = await client.get_config("secrets.yaml")
        except RuntimeError as err:
            print(f"could not download secrets.yaml: {err}", file=sys.stderr)
        else:
            dest = OUTPUT_DIR / "secrets.yaml"
            write_text(dest, blur_secrets_yaml(raw_secrets))
            print(f"wrote {dest} (values blurred)")
    finally:
        await client.close()

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download ESPHome device YAML from Device Builder. "
        "secrets.yaml is saved with values replaced by ********."
    )
    parser.add_argument("--url", default=os.environ.get("ESPHOME_URL", DEFAULT_URL))
    return parser.parse_args()


def main() -> int:
    return asyncio.run(download(parse_args()))


if __name__ == "__main__":
    raise SystemExit(main())
