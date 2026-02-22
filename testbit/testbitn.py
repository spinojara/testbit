#!/usr/bin/env python3

import requests
import json
import time
import docker
import atexit
import sys
import threading
from docker.errors import ImageNotFound
import argparse
import getpass
import configparser
import urllib3
import os
import signal
import random

from .tc import tcadjust
from .cgroup import CPU
from . import cgroup
from .exception import log_exception

container_lock = threading.Lock()
containers = []
cpu_lock = threading.Lock()

running = True

def handle_sigint(signum, frame):
    with cpu_lock:
        global running
        running = False
    sys.exit(0)

def cleanup_docker():
    with container_lock:
        for container in containers:
            try:
                container.stop(timeout=0)
            except:
                pass
            try:
                container.remove(force=True)
            except:
                pass

def cleanup(cpu: CPU):
    with cpu_lock:
        cpu.release()

def worker(cpu: cgroup.CPU, host: str, password: str, tcfactor: float, syzygy: str | None):
    client = docker.from_env()
    verify = not host in ["localhost", "127.0.0.1"]
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    host = "https://" + host + ":2718"

    rng = random.Random()

    if syzygy:
        syzygy_paths = syzygy.split(":")
    else:
        syzygy_paths = []

    for path in syzygy_paths:
        if not path.startswith("/"):
            print(f"syzygy path '{path}' is not absolute.", file=sys.stderr)
            return

    while True:
        try:
            response = requests.get(host + "/test/task", auth=("", password), verify=verify)
            response = response.json()
        except json.JSONDecodeError:
            log_exception()
            sys.exit(1)
        except:
            has_critical_exception = cpu.release()
            log_exception()
            print(f"{threading.get_ident()}: No connection, sleeping for 1 minute")
            if has_critical_exception:
                break
            time.sleep(60)
            continue
        id = response.get("id")
        tc = response.get("tc")
        adjudicate = response.get("adjudicate")
        task_uuid = response.get("uuid")

        adjudicatestring = ""
        if adjudicate == "draw" or adjudicate == "both":
            adjudicatestring += "-draw movenumber=40 movecount=8 score=10"
        if adjudicate == "resign" or adjudicate == "both":
            adjudicatestring += " -resign twosided=true movecount=3 score=800"

        if None in [id, tc, adjudicate, task_uuid]:
            has_critical_exception = cpu.release()
            print(f"{threading.get_ident()}: No task, sleeping for 10 seconds")
            if has_critical_exception:
                break
            time.sleep(10)
            continue

        print(f"{threading.get_ident()}: Got task")
        command=f"""
            fastchess -testEnv
                      -concurrency 1
                      -each tc={tcadjust(tc, tcfactor)} proto=uci timemargin=10000 option.Debug=true
                      -rounds 1
                      -games 2
                      -openings format=epd file=./book.epd order=random
                      -repeat
        """

        command += adjudicatestring

        # Remove potential bias
        if rng.choice([True, False]):
            command += " -engine cmd=./bitbit-new name=bitbit-new -engine cmd=./bitbit-old name=bitbit-old"
        else:
            command += " -engine cmd=./bitbit-old name=bitbit-old -engine cmd=./bitbit-new name=bitbit-new"

        if syzygy:
            command += f" -tb {syzygy} -tbadjudicate BOTH"

        print(f"{threading.get_ident()}: Claiming cpu")
        with cpu_lock:
            if running:
                cpu.claim()
            else:
                break
        print(f"{threading.get_ident()}: Running docker container")
        print(f"{threading.get_ident()}: {" ".join(command.split())}")
        try:
            container = client.containers.run(
                image="jalagaoi.se:5000/testbit:%d" % id,
                command=command,
                detach=True,
                cgroup_parent="testbit-%d" % cpu.cpu,
                volumes={path: {"bind": path, "mode": "ro"} for path in syzygy_paths},
            )

            with container_lock:
                containers.append(container)

        except ImageNotFound:
            try:
                response = requests.put(host + "/test/docker/%d" % id, json={"uuid": task_uuid}, auth=("", password), verify=verify)
            except:
                log_exception()
            finally:
                continue

        # If this fails it's probably because the container was killed
        # by cleanup_docker, so let's just exit
        try:
            result = container.wait()
            logs: str = container.logs().decode("utf-8")
        except:
            log_exception()
            break

        try:
            container.remove(force=True)
            with container_lock:
                containers.remove(container)
        except:
            pass

        print(f"{threading.get_ident()}: Docker container exited")

        losses = 0
        draws = 0
        wins = 0

        for line in logs.splitlines():
            if "Finished game " in line:
                if "(bitbit-new vs bitbit-old)" in line:
                    if " 1-0 " in line:
                        wins += 1
                    if " 0-1 " in line:
                        losses += 1
                elif "(bitbit-old vs bitbit-new)" in line:
                    if " 1-0 " in line:
                        losses += 1
                    if " 0-1 " in line:
                        wins += 1
                else:
                    continue
                if " 1/2-1/2 " in line:
                    draws += 1

        # We were killed by a signal so just exit here
        if result["StatusCode"] >= 128:
            print(f"{threading.get_ident()}: Killed by signal {result["StatusCode"]}")
            break

        try:
            if result["StatusCode"] or losses + draws + wins != 2:
                print(f"{threading.get_ident()}: Docker container had errors")
                response = requests.put(host + "/test/error/%d" % id, json={"errorlog": logs, "uuid": task_uuid}, auth=("", password), verify=verify)
            else:
                print(f"{threading.get_ident()}: Docker container had no errors")
                response = requests.put(host + "/test/%d" % id, json={"losses": losses, "draws": draws, "wins": wins, "uuid": task_uuid}, auth=("", password), verify=verify)
        except:
            log_exception()

    with cpu_lock:
        if running:
            print(f"{threading.get_ident()}: Loop exited, sending SIGINT", file=sys.stderr)
            os.kill(os.getpid(), signal.SIGINT)

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--workers", type=int, help="Amount of workers.", default=-1)
    parser.add_argument("--stdin", type=str, help="Read stdin from file.")
    parser.add_argument("--host", type=str, help="Hostname of testbitd.", default="localhost")
    parser.add_argument("--daemon", help="daemon mode.", action="store_true", default=False)
    parser.add_argument("--syzygy", help="Colon separated list of syzygy tablebases directories.", default=None)

    args, _ = parser.parse_known_args()
    if args.workers < 1 and args.workers != -1:
        print("--workers must be positive or -1", file=sys.stderr)
        return 1

    if not args.stdin:
        password = getpass.getpass("Enter passphrase: ")
    else:
        with open(args.stdin, "r") as f:
            password = f.read().split("\n")[0]

    config = configparser.ConfigParser()
    config.read("/etc/bitbit.ini")
    tcfactor = config.getfloat("timecontrol", "tcfactor")
    if args.daemon:
        args.workers = config.getint("testbitn", "workers")
        args.host = config.get("testbitn", "host")
        args.syzygy = config.get("testbitn", "syzygy", fallback=None)

    cpus = cgroup.make_cpu_claiming_strategy(cgroup.cpuset_cpus_effective(), args.workers)

    if not cpus:
        print("Failed to make cpu claiming strategy.", file=sys.stderr)
        return 1

    threads = [threading.Thread(target=worker, args=(cpu, args.host, password, tcfactor, args.syzygy), daemon=True) for cpu in cpus]

    signal.signal(signal.SIGINT, handle_sigint)

    atexit.register(cleanup_docker)
    for cpu in cpus:
        atexit.register(cleanup, cpu)

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    return 0

if __name__ == "__main__":
    sys.exit(main())
