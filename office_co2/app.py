from datetime import UTC, datetime, date
import calendar
from typing import cast

import altair as alt
import apsw
import apsw.bestpractice
import apsw.ext
import pandas as pd
import streamlit as st

ONE_DAY_S = 86400

def to_sec_unix(dt: datetime | date):
    if isinstance(dt, datetime):
        return dt.replace(tzinfo=UTC).timestamp()
    return calendar.timegm(dt.timetuple()) + ONE_DAY_S # Extend till the end of the day

def from_sec_unix(s: float):
    return datetime.fromtimestamp(s, UTC)

def now():
    return datetime.now(UTC)

db = apsw.Connection("./hist/history.db", flags=apsw.SQLITE_OPEN_READONLY)
db.set_busy_timeout(300)
apsw.ext.log_sqlite()

timestamp_now = to_sec_unix(now())
one_week_ago = timestamp_now - ONE_DAY_S * 7

date_range = st.date_input("Data range", [from_sec_unix(one_week_ago), from_sec_unix(timestamp_now)])
datetuple_raw = cast(tuple[date, date] | tuple[date], date_range)

if len(datetuple_raw) == 1:
    datetuple = (datetuple_raw[0], now())
else:
    datetuple = cast(tuple[date, date], datetuple_raw)

spacing = ((to_sec_unix(datetuple[1]) - to_sec_unix(datetuple[0])) / ONE_DAY_S) / 30 # every month, the scaling increases by 1

cursor = db.cursor()
cursor.execute(f"select value, dt from co2 where dt >= {to_sec_unix(datetuple[0])} AND dt <= {to_sec_unix(datetuple[1])}")
co2_df = pd.DataFrame(cast(list[tuple[int, int]], cursor.fetchall()))

cursor.execute(f"select value, dt from temp where dt > {to_sec_unix(datetuple[0])} AND dt <= {to_sec_unix(datetuple[1])}")
temp_df = pd.DataFrame(cast(list[tuple[int, int]], cursor.fetchall()))


st.subheader("Temperature history")

if len(temp_df) == 0:
    st.write("No temperature history for selected period")
else:
    temp_data = pd.DataFrame(
        {
            "time": temp_df[temp_df.columns[1]].map(
                datetime.fromtimestamp, na_action="ignore"
            ),
            "temperature": temp_df[temp_df.columns[0]],
        }
    )

    temp_chart = (
        alt.Chart()
        .mark_line(color="#FF5500")
        .encode(
            x=alt.X("time:T", axis=alt.Axis(format="%a %d %H:%M")),
            y=alt.Y("temperature", scale=alt.Scale(domain=[22, 30])),
        )
        .interactive()
    )

    temp_line = (
        alt.Chart()
        .mark_rule(color="firebrick")
        .encode(y="mean(temperature):Q", size=alt.SizeValue(3))
    )

    st.altair_chart(
        alt.layer(temp_chart, temp_line, data=temp_data), use_container_width=True
    )

st.subheader("Co2 history")

if len(co2_df) == 0:
    st.write("No co2 history for selected period")
else:

    co2_data = pd.DataFrame(
        {
            "time": co2_df[co2_df.columns[1]].map(
                datetime.fromtimestamp, na_action="ignore"
            ),
            "co2": co2_df[co2_df.columns[0]],
        }
    )

    co2_chart = (
        alt.Chart()
        .mark_line(color="#2255FF")
        .encode(
            x=alt.X("time:T", axis=alt.Axis(format="%a %d %H:%M")),
            y=alt.Y("co2", scale=alt.Scale(domain=[300, 900])),
        )
        .interactive()
    )

    co2_line = (
        alt.Chart()
        .mark_rule(color="teal")
        .encode(y="mean(co2):Q", size=alt.SizeValue(3))
    )

    st.altair_chart(
        alt.layer(co2_chart, co2_line, data=co2_data), use_container_width=True
    )
