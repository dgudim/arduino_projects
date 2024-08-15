#!/bin/sh

granularity=90

if [ ! -f ./CntR ]; then
  echo "CntR not found!"
  exit 1
fi

if [ ! -f ./Tamb ]; then
  echo "Tamb not found!"
  exit 1
fi

if [ ! -f ./history.db ]; then
  echo "Creating database..."
  sqlite3 ./history.db <<EOF
PRAGMA journal_mode = WAL;

create table co2 (rowid INTEGER PRIMARY KEY, value int, dt datetime);
create table temp (rowid INTEGER PRIMARY KEY, value int, dt datetime);
EOF
fi

echo "Starting main loop, granularity: ${granularity}s"

while true; do
    co2_level=$(cat ./CntR)
    temp_level=$(cat ./Tamb)

    sqlite3 ./history.db "INSERT INTO co2(value, dt) VALUES($co2_level, $(date +%s));"
    sqlite3 ./history.db "INSERT INTO temp(value, dt) VALUES($temp_level, $(date +%s));"

    sleep $granularity
done


