from typing import TypeVar, Generic

T = TypeVar("T")

UINT64_MAX = 0xFFFFFFFFFFFFFFF

class CRandom:
    _Seed: int

    def __init__(self, n = 0) -> None:
        self._Seed = (n + 1274012836) & UINT64_MAX

    def Seed(self, ulSeed: int) -> None:
        self._Seed = (ulSeed + 1274012836) & UINT64_MAX

    def NewValue(self) -> int:
        self._Seed ^= (self._Seed >> 12) & UINT64_MAX
        self._Seed ^= (self._Seed << 25) & UINT64_MAX
        self._Seed ^= (self._Seed >> 27) & UINT64_MAX
        return (self._Seed * 2685821657736338717) & UINT64_MAX

    def NextDouble(self) -> float:
        return float(self.NewValue()) / UINT64_MAX
