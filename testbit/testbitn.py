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

container_lock = threading.Lock()
containers = []

def cleanup_docker():
    print("cleaning up docker")
    with container_lock:
        print("clean up aquired lock")
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
    cpu.release()

def worker(cpu: cgroup.CPU, host: str, password: str, tcfactor: float):
    client = docker.from_env()
    verify = not host in ["localhost", "127.0.0.1"]
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    host = "https://" + host + ":2718"

    rng = random.Random()

    while True:
        try:
            response = requests.get(host + "/test/task", auth=("", password), verify=verify)
            response = response.json()
        except json.JSONDecodeError:
            sys.exit(1)
        except Exception as e:
            cpu.release()
            print(str(cpu.cpu) + " " + str(e))
            time.sleep(60)
            continue
        id = response.get("id")
        tc = response.get("tc")
        adjudicate = response.get("adjudicate")
        adjudicatestring = ""
        if adjudicate == "draw" or adjudicate == "both":
            adjudicatestring += "-draw movenumber=40 movecount=8 score=10"
        if adjudicate == "resign" or adjudicate == "both":
            adjudicatestring += " -resign twosided=true movecount=3 score=800"

        if None in [id, tc, adjudicate]:
            cpu.release()
            print(str(cpu.cpu) + " " + "nothing to do, sleeping...")
            time.sleep(10)
            continue
        print(str(cpu.cpu) + " " + "got task")

        command="""
            fastchess -testEnv
                      -concurrency 1
                      -each tc=%s proto=uci timemargin=10000 option.Debug=true
                      -rounds 1
                      -games 2
                      -openings format=epd file=./book.epd order=random
                      -repeat
        """
        # Remove potential bias
        if rng.choice([True, False]):
            command += " -engine cmd=./bitbit-new name=bitbit-new -engine cmd=./bitbit-old name=bitbit-old %s"
        else:
            command += " -engine cmd=./bitbit-old name=bitbit-old %s -engine cmd=./bitbit-new name=bitbit-new"

        cpu.claim()
        try:
            container = client.containers.run(
                image="jalagaoi.se:5000/testbit:%d" % id,
                command=command % (tcadjust(tc, tcfactor), adjudicatestring),
                detach=True,
                cgroup_parent="testbit-%d" % cpu.cpu,
            )

            with container_lock:
                containers.append(container)

        except ImageNotFound:
            try:
                response = requests.put(host + "/test/docker/%d" % id, auth=("", password), verify=verify)
            finally:
                continue

        should_exit = False
        # If this fails it's probably because the container was killed
        # by cleanup_docker, so let's just exit
        try:
            result = container.wait()
            logs: str = container.logs().decode("utf-8")
            container.remove(force=True)
            with container_lock:
                containers.remove(container)
        except Exception as e:
            print(str(cpu.cpu) + " " + str(e))
            print(str(cpu.cpu) + " " + "exiting...")
            break

        losses = 0
        draws = 0
        wins = 0

        for line in logs.splitlines():
            print(str(cpu.cpu) + " " + line)
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

        try:
            if result["StatusCode"] or losses + draws + wins != 2:
                response = requests.put(host + "/test/error/%d" % id, json={"errorlog": logs}, auth=("", password), verify=verify)
                print(str(cpu.cpu) + " " + response.json())
            else:
                response = requests.put(host + "/test/%d" % id, json={"losses": losses, "draws": draws, "wins": wins}, auth=("", password), verify=verify)
                print(str(cpu.cpu) + " " + response.json())
        except:
            pass

    os.kill(os.getpid(), signal.SIGINT)

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--workers", type=int, help="Amount of workers.", default=-1)
    parser.add_argument("--stdin", type=str, help="Read stdin from file.")
    parser.add_argument("--host", type=str, help="Hostname of testbitd.", default="localhost")
    parser.add_argument("--daemon", help="daemon mode.", action="store_true", default=False)

    args, _ = parser.parse_known_args()
    if args.workers < 1 and args.workers != -1:
        print("--workers must be positive or -1")
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

    cpus = cgroup.make_cpu_claiming_strategy(cgroup.cpuset_cpus_effective(), args.workers)

    if not cpus:
        print("Failed to make cpu claiming strategy.")
        return 1

    threads = [threading.Thread(target=worker, args=(cpu, args.host, password, tcfactor), daemon=True) for cpu in cpus]

    for cpu in cpus:
        atexit.register(cleanup, cpu)
    # Register dockerclean up last, so that it runs first
    atexit.register(cleanup_docker)

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    print("joined all threads")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except:
        sys.exit(1)
