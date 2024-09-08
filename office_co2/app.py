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
    return calendar.timegm(dt.timetuple()) + ONE_DAY_S  # Extend till the end of the day


def from_sec_unix(s: float):
    return datetime.fromtimestamp(s, UTC)


def now():
    return datetime.now(UTC)


db = apsw.Connection("./hist/history.db", flags=apsw.SQLITE_OPEN_READONLY)
db.set_busy_timeout(300)
apsw.ext.log_sqlite()

timestamp_now = to_sec_unix(now())
one_week_ago = timestamp_now - ONE_DAY_S * 7

date_range = st.date_input(
    "Data range", [from_sec_unix(one_week_ago), from_sec_unix(timestamp_now)]
)
datetuple_raw = cast(tuple[date, date] | tuple[date], date_range)

if len(datetuple_raw) == 1:
    datetuple = (datetuple_raw[0], now())
else:
    datetuple = cast(tuple[date, date], datetuple_raw)

spacing = int(
    ((to_sec_unix(datetuple[1]) - to_sec_unix(datetuple[0])) / ONE_DAY_S) / 30 + 1
)  # every month, the scaling increases by 1

smoothing_range = st.slider(
    "Smoothing range (samples)", min_value=1, max_value=100, value=25
)

cursor = db.cursor()
cursor.execute(
    f"select value, dt from co2 where dt >= {to_sec_unix(datetuple[0])} AND dt <= {to_sec_unix(datetuple[1])} AND rowid % {spacing} = 0"
)
co2_df = pd.DataFrame(cast(list[tuple[int, int]], cursor.fetchall()))

cursor.execute(
    f"select value, dt from temp where dt > {to_sec_unix(datetuple[0])} AND dt <= {to_sec_unix(datetuple[1])} AND rowid % {spacing} = 0"
)
temp_df = pd.DataFrame(cast(list[tuple[int, int]], cursor.fetchall()))


def make_chart(
    df: pd.DataFrame,
    data_range: list[int],
    main_col: str,
    line_col: str,
    rolling_mean_col: str,
):
    transformed_data = pd.DataFrame(
        {
            "time": df[df.columns[1]].map(datetime.fromtimestamp, na_action="ignore"),
            "data": df[df.columns[0]],
        }
    )

    main_chart = (
        alt.Chart()
        .mark_line(color=main_col)
        .encode(
            x=alt.X("time:T", axis=alt.Axis(format="%a %d %H:%M")),
            y=alt.Y("data", scale=alt.Scale(domain=data_range)),
        )
        .interactive()
    )

    mean_line = (
        alt.Chart()
        .mark_rule(color=line_col)
        .encode(y="mean(data):Q", size=alt.SizeValue(3))
    )

    rolling_mean = (
        alt.Chart()
        .mark_line(color=rolling_mean_col)
        .transform_window(
            # The field to average
            rolling_mean="mean(data)",
            # The number of values before and after the current value to include.
            frame=[-smoothing_range, smoothing_range],
        )
        .encode(x="time:T", y="rolling_mean:Q")
    )

    st.altair_chart(
        cast(
            alt.Chart,
            alt.layer(main_chart, mean_line, rolling_mean, data=transformed_data),
        ),
        use_container_width=True,
    )


st.subheader("Temperature history")
if len(temp_df) == 0:
    st.write("No temperature history for selected period")
else:
    make_chart(temp_df, [20, 30], "#882d00", "#b22222", "#ffc261")

st.subheader("Co2 history")
if len(co2_df) == 0:
    st.write("No co2 history for selected period")
else:
    make_chart(co2_df, [300, 900], "#112a7d", "#008080", "#65f0cd")
