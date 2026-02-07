#!/usr/bin/env python3

import requests
import json
import time
import docker
import atexit
import cgroup
import sys
import threading
from docker.errors import ImageNotFound
from cgroup import CPU
import argparse

from . import tc

def cleanup(cpu: CPU):
    cpu.release()

def worker(cpu: CPU, host: str):
    client = docker.from_env()

    while True:
        try:
            response = requests.get(host + "/test/task")
            response = response.json()
        except json.JSONDecodeError:
            sys.exit(1)
        except:
            cpu.release()
            print("Sleeping for 60")
            time.sleep(60)
            continue
        id = response.get("id")
        tc = response.get("tc")
        adjudicate = response.get("adjudicate")
        adjudicatestring = ""
        if adjudicate = "draw" or adjudicate = "both":
            adjudicatestring += "-draw movenumber=40 movecount=8 score=10"
        if adjudicate = "resign" or adjudicate = "both":
            adjudicatestring += "-resign twosided=true movecount=3 score=800"

        print(response)
        if None in [id, tc, adjudicate]:
            cpu.release()
            time.sleep(60)
            continue

        cpu.claim()
        try:
            container = client.containers.run(
                image="testbit:%d" % id,
                command="fastchess -testEnv -concurrency 1 -each tc=%s proto=uci timemargin=10000 option.Debug=true -rounds 1 -games 2 -openings format=epd file=./book.epd order=random -repeat -engine cmd=./bitbit-new name=bitbit-new -engine cmd=./bitbit-old name=bitbit-old %s" % (timecontrol.tcadjust(tc), adjudicatestring),
                detach=True,
                parent_cgroup="testbit-%d" % cpu.cpu
            )
        except ImageNotFound:
            response = requests.put(host + "/test/docker/%d" % id)

        result = container.wait()
        logs: str = container.logs().decode("utf-8")

        losses = 0
        draws = 0
        wins = 0

        for line in logs.splitlines():
            print(line)
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

        if result["StatusCode"] or losses + draws + wins != 2:
            response = requests.put(host + "/test/error/%d" % id, data={"data": json.dumps({"errorlog": container.logs().decode("utf-8")})})
            print(response.json())
        else:
            response = requests.put(host + "/test/%d" % id, data={"data" : json.dumps({"losses": losses, "draws": draws, "wins": wins})})
        container.remove()
        break

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--workers", type=int, help="Amount of workers.", default=-1)

    args, _ = parser.parse_known_args()
    if args.workers < 1 and args.workers != -1:
        print("--workers must be positive or -1")
        return 1

    cpus = cgroup.make_cpu_claiming_strategy(cgroup.cpuset_cpus_effective(), args.workers)

    if not cpus:
        print("Failed to make cpu claiming strategy.")
        return 1

    threads = [threading.Thread(target=worker, args=(cpu, "http://127.0.0.1:8000"), daemon=True) for cpu in cpus]

    for cpu in cpus:
        atexit.register(cleanup, cpu)

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    return 0

if __name__ == "__main__":
    sys.exit(main())
