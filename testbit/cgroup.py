#!/usr/bin/env python3

from pathlib import Path
from typing import List, Self, Set

class CPU:
    cpu: int
    claimed: bool
    performance: bool
    thread_siblings: Set[int]
    cpu0: bool

    def __init__(self: Self, cpu: int, claimed: bool, performance: bool, thread_siblings: Set[int]):
        self.cpu = cpu
        self.claimed = claimed
        self.performance = performance
        self.thread_siblings = thread_siblings
        self.cpu0 = cpu == 0 or 0 in thread_siblings

    def __repr__(self):
        return f"cpu: {self.cpu}\nclaimed: {self.claimed}\nperformance: {self.performance}\nthread_siblings: {self.thread_siblings}\ncpu0: {self.cpu0}\n"

    def claim(self: Self):
        if self.claimed:
            return
        self.claimed = True
        cgroup = Path("/sys/fs/cgroup/testbit-%d" % self.cpu)
        cgroup.mkdir()

        with open(cgroup / "cpuset.cpus", "w") as f:
            f.write("%d" % self.cpu)
        with open(cgroup / "cpuset.cpus.partition", "w") as f:
            f.write("isolated")

        for cpu in self.thread_siblings:
            with open("/sys/devices/system/cpu/cpu%d/online" % cpu, "w") as f:
                f.write("0")

    def release(self: Self):
        if not self.claimed:
            return
        Path("/sys/fs/cgroup/testbit-%d" % self.cpu).rmdir()
        for cpu in self.thread_siblings:
            with open("/sys/devices/system/cpu/cpu%d/online" % cpu, "w") as f:
                f.write("1")
        self.claimed = False

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
