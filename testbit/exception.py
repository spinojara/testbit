import traceback
import sys
from datetime import datetime
import threading

lock = threading.Lock()

def log_exception():
    with lock:
        print(f"\nCaught exception at {datetime.now()}:", file=sys.stderr)
        stack = traceback.extract_stack()[:-2]
        tb = traceback.extract_tb(sys.exc_info()[2])
        combined = stack + tb
        print("Traceback (most recent call last):", file=sys.stderr)
        for line in traceback.format_list(combined):
            print(line, file=sys.stderr, end="")
        print(f"{sys.exc_info()[0].__name__}: {sys.exc_info()[1]}", file=sys.stderr)
