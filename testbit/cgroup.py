#!/usr/bin/env python3

from pathlib import Path
from typing import List, Self, Set
import time
import sys
import select

from .exception import log_exception

def echo(file, message):
    with open(file, "w") as f:
        f.write(message)

class CPU:
    cpu: int
    claimed: bool
    performance: bool
    thread_siblings: Set[int]
    cpu0: bool
    cgroup: Path

    def __init__(self: Self, cpu: int, claimed: bool, performance: bool, thread_siblings: Set[int]):
        self.cpu = cpu
        self.claimed = claimed
        self.performance = performance
        self.thread_siblings = thread_siblings
        self.cpu0 = cpu == 0 or 0 in thread_siblings
        self.cgroup = Path("/sys/fs/cgroup/testbit-%d" % self.cpu)

    def __repr__(self):
        return f"cpu: {self.cpu}\nclaimed: {self.claimed}\nperformance: {self.performance}\nthread_siblings: {self.thread_siblings}\ncpu0: {self.cpu0}\n"

    def claim(self: Self):
        if self.claimed:
            return
        self.claimed = True
        self.cgroup.mkdir(exist_ok=True)

        echo(self.cgroup / "cpuset.cpus", "%d\n" % self.cpu)
        echo(self.cgroup / "cpuset.cpus.partition", "isolated\n")

        for cpu in self.thread_siblings:
            echo("/sys/devices/system/cpu/cpu%d/online" % cpu, "0\n")

    def release(self: Self) -> bool:
        if not self.claimed:
            return False

        has_critical_exception = False
        for cpu in self.thread_siblings:
            try:
                echo("/sys/devices/system/cpu/cpu%d/online" % cpu, "1\n")
            except:
                has_critical_exception = True
                log_exception()
                print("Failed to turn cpu%d back online" % cpu, file=sys.stderr)

        try:
            echo(self.cgroup / "cpuset.cpus.partition", "member\n")
        except:
            has_critical_exception = True
            log_exception()
            print("Failed to make cgroup testbit-%d non isolated" % (self.cpu, self.cpu), file=sys.stderr)

        try:
            echo(self.cgroup / "cpuset.cpus", "\n")
        except:
            has_critical_exception = True
            log_exception()
            print("Failed to release cpu%d from cgroup testbit-%d" % (self.cpu, self.cpu), file=sys.stderr)

        try:
            echo(self.cgroup / "cgroup.kill", "1\n")
        except:
            has_critical_exception = True
            log_exception()
            print("Failed to kill cgroup testbit-%d" % self.cpu, file=sys.stderr)

        self.claimed = False

        return has_critical_exception

    def remove(self: Self):
        # TODO: Improve this
        # time.sleep(0.01)
        try:
            with open(self.cgroup / "cgroup.events", "r") as f:
                p = select.poll()
                p.register(f, select.POLLPRI | select.POLLERR)
                while True:
                    f.seek(0)
                    if "populated 1" not in f.read():
                        break
                    p.poll()
        except:
            has_critical_exception = True
            log_exception()
            print("Failed to poll cgroup.events for cgroup testbit-%d" % self.cpu, file=sys.stderr)

        try:
            self.cgroup.rmdir()
        except OSError:
            pass
        except:
            has_critical_exception = True
            log_exception()
            print("Failed to remove cgroup testbit-%d" % self.cpu, file=sys.stderr)

def is_performance(cpu: int) -> bool:
    performance = True
    try:
        with open("/sys/devices/cpu_core/cpus", "r") as f:
            if cpu not in parse_cpus(f.read()):
                performance = False
    except:
        pass

    return performance

def cpuset_cpus_effective() -> Set[CPU]:
    cpus: Set[CPU] = set()
    with open("/sys/fs/cgroup/cpuset.cpus.effective", "r") as f:
        cpus_effective = parse_cpus(f.read())

    for cpu in cpus_effective:
        with open("/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list" % cpu, "r") as f:
            thread_siblings = parse_cpus(f.read()) - {cpu}

        cpus.add(CPU(cpu=cpu, claimed=False, performance=is_performance(cpu), thread_siblings=thread_siblings))

    return cpus

def list_cpus(cpus: str) -> List[str]:
    l: List[str] = []
    current_int = ""
    for c in cpus:
        if c == "\n":
            break
        elif c == "-":
            l.extend([current_int, "-"])
            current_int = ""
        elif c == ",":
            l.extend([current_int, ","])
            current_int = ""
        else:
            current_int += c
    if current_int:
        l.append(current_int)
    return l

def parse_cpus(cpus: str | List[str]) -> Set[int]:
    if isinstance(cpus, str):
        cpus = list_cpus(cpus)
    if not cpus:
        return set()
    if len(cpus) == 1:
        return {int(cpus[0])}
    if not len(cpus) % 2:
        raise Exception("Length of list is not odd")
    cpu0 = int(cpus[0])
    op = cpus[1]
    if op == "-":
        cpu1 = int(cpus[2])
        return set(range(cpu0, cpu1 + 1)) | parse_cpus(cpus[4:])
    elif op == ",":
        return {cpu0} | parse_cpus(cpus[2:])
    else:
        raise Exception("Bad operator")



def make_cpu_claiming_strategy(cpus: Set[CPU], n: int) -> Set[CPU] | None:
    claimed: Set[CPU] = set()
    for cpu in cpus:
        if (
            cpu.cpu0
            or not cpu.performance
            or cpu.cpu in {cpu.cpu for cpu in claimed} | {c for cpu in claimed for c in cpu.thread_siblings}
        ):
            continue
        n -= 1
        claimed.add(cpu)
        if n == 0:
            break
    if n > 0:
        return None
    return claimed
