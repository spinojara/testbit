#!/usr/bin/env python3

import threading
import sys
import atexit
import configparser
import docker

from . import cgroup
from .cgroup import CPU

results = []

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

def worker(cpu: CPU):
    client = docker.from_env()

    cpu.claim()



    container = client.containers.run(
        image="bitbench:1.6",
        command="./bitbit bench , quit",
        detach=True,
        cgroup_parent="testbit-%d" % cpu.cpu,
    )
    with container_lock:
        containers.append(container)

    container.wait()
    logs: str = container.logs().decode("utf-8")
    cpu.release()

    for line in logs.splitlines():
        if line.startswith("time: "):
            t = int(line.split(" ")[1])
            with container_lock:
                results.append(t)
            break


def main() -> int:
    cpus = cgroup.make_cpu_claiming_strategy(cgroup.cpuset_cpus_effective(), -1)

    if not cpus:
        print("Failed to make cpu claiming strategy.")
        return 1

    client = docker.from_env()

    client.images.build(
        path=".",
        dockerfile="Dockerfile.tcfactor",
        tag="bitbench:1.6",
        rm=True,
        forcerm=True,
    )

    for cpu in cpus:
        atexit.register(cleanup, cpu)
    # Register dockerclean up last, so that it runs first
    atexit.register(cleanup_docker)

    threads = [threading.Thread(target=worker, args=(cpu, ), daemon=True) for cpu in cpus]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    print("joined all threads")

    print(results)

    average = sum(results) / len(results)

    tcfactor = 0.833164 * average / 15000

    config = configparser.ConfigParser()
    config.read("/etc/bitbit.ini")
    if not config.has_section("timecontrol"):
        config.add_section("timecontrol")


    config.set("timecontrol", "tcfactor", str(tcfactor))

    with open("/etc/bitbit.ini", "w") as f:
        config.write(f)


if __name__ == "__main__":
    sys.exit(main())
