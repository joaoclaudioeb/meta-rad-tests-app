import subprocess
import time

REMOTE = "root@192.168.99.1"
REMOTE_DB = "/experiment/rad-tests-app.sqlite3"
REMOTE_TMP = "/tmp/rad-tests-backup.db"
LOCAL_DEST = "./db/rad-tests-app.sqlite3"
INTERVAL = 15

while True:
    subprocess.run([
        "ssh", REMOTE,
        f"sqlite3 '{REMOTE_DB}' \".backup '{REMOTE_TMP}'\""
    ], check=True)

    subprocess.run([
        "rsync", "-az", "--checksum",
        f"{REMOTE}:{REMOTE_TMP}",
        LOCAL_DEST
    ], check=True)

    subprocess.run([
        "ssh", REMOTE,
        f"rm -f '{REMOTE_TMP}'"
    ], check=True)

    time.sleep(INTERVAL)
