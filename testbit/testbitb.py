#!/usr/bin/env python3

import sys
import time
import requests
import argparse
import getpass
from pathlib import Path
import re
from datetime import datetime, timedelta

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dbbackup", type=str, help="Directory containing backups")
    parser.add_argument("--stdin", type=str, help="Read stdin from file.")

    args, _ = parser.parse_known_args()
    backup_directory = Path(args.dbbackup);
    if not args.stdin:
        password = getpass.getpass("Enter passphrase: ")
    else:
        with open(args.stdin, "r") as f:
            password = f.read().split("\n")[0]

    response = requests.post("https://localhost:2718/test/backup", auth=("", password), verify=False)
    response = response.json()
    if response.get("message", "") != "ok":
        return 1

    pattern = r"^bitbit.sqlite3.backup-(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})$"

    backups = []

    for backup in backup_directory.iterdir():
        if not backup.is_file():
            continue
        match = re.match(pattern, backup.name)
        if not match:
            continue

        dt = datetime.fromisoformat(backup.name[22:])
        backups.append(dt)

    if not backups:
        return 1

    backups.sort()

    today = backups.pop(-1)
    backups = [backup for backup in backups if today - backup <= timedelta(days=365)]
    store = []
    for backup in backups:
        if today - backup <= timedelta(days=365) and today - backup > timedelta(days=183):
            backups.remove(backup)
            store.append(backup)
            break

    for backup in backups:
        if today - backup <= timedelta(days=183) and today - backup > timedelta(days=29):
            backups.remove(backup)
            store.append(backup)
            break

    for backup in backups:
        if today - backup <= timedelta(days=29) and today - backup > timedelta(days=8):
            backups.remove(backup)
            store.append(backup)
            break

    for backup in backups:
        if today - backup <= timedelta(days=8) and today - backup > timedelta(days=6):
            backups.remove(backup)
            store.append(backup)
            break

    for backup in backups:
        current = backup_directory / ("bitbit.sqlite3.backup-" + backup.strftime("%Y-%m-%dT%H:%M:%S"))
        current.unlink()

    return 0


if __name__ == "__main__":
    sys.exit(main())
