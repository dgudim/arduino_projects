#!/bin/bash

BACKUPS_PATH="backups"
HIST_FILE="history.db"
MAX_BACKUPS=10
granularity=90
backup_interval=960 # (960 * granularity = 86400 (1 day))

function do_backup() {
  sqlite3 $HIST_FILE .dump | gzip -c >"$BACKUPS_PATH/$HIST_FILE.backup-$(date +%s).gz"
  # zcat history.db.backup-N.gz | sqlite3 ./history.db

  # Create an array of backup directories sorted by date (newest to oldest)
  local backups=($(ls -1d $BACKUPS_PATH/* | sort -r))

  # Keep only the newest $MAX_BACKUPS directories
  local count=${#backups[@]}
  if [ $count -gt $MAX_BACKUPS ]; then
    local backups_to_delete=(${backups[@]:$MAX_BACKUPS})
    for backup in "${backups_to_delete[@]}"; do
      rm -rf "$backup"
    done
  fi
}

if [ ! -f ./CntR ]; then
  echo "CntR not found!"
  exit 1
fi

if [ ! -f ./Tamb ]; then
  echo "Tamb not found!"
  exit 1
fi

mkdir -pv $BACKUPS_PATH

if [ ! -f $HIST_FILE ]; then
  echo "Creating database..."
  sqlite3 $HIST_FILE <<EOF
PRAGMA journal_mode = WAL;

create table co2 (rowid INTEGER PRIMARY KEY, value int, dt datetime);
create table temp (rowid INTEGER PRIMARY KEY, value int, dt datetime);
EOF
fi

echo "Starting main loop, granularity: ${granularity}s"

i=0

while true; do
  co2_level=$(cat ./CntR)
  temp_level=$(cat ./Tamb)

  sqlite3 $HIST_FILE "INSERT INTO co2(value, dt) VALUES($co2_level, $(date +%s));"
  sqlite3 $HIST_FILE "INSERT INTO temp(value, dt) VALUES($temp_level, $(date +%s));"

  if [ "$i" -eq "$backup_interval" ]; then
    do_backup
    i=0
  fi
  i=$((i + 1))

  sleep $granularity
done
