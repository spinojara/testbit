import math

from .csamplingpolicy import CSamplingPolicy
from .cregression import CRegression
from .crandom import CRandom
from .util import Vector

class CSPWeight(CSamplingPolicy):
    vResult: Vector[float]
    reg: CRegression
    ReplicationThreshold: int
    nMCMC: int
    rnd: CRandom

    def __init__(self, reg: CRegression, ReplicationThreshold: int = 0, nMCMC: int = 100) -> None:
        self.vResult = Vector(reg.GetPF().GetDimensions())
        self.reg = reg
        self.ReplicationThreshold = ReplicationThreshold
        self.nMCMC = nMCMC
        self.rnd = CRandom()

    def NextSample(self, i: int) -> list[float]:
        if self.ReplicationThreshold and i > 0 and self.reg.GetReplications(i - 1) < (1 + i // self.ReplicationThreshold):
            return self.reg.GetSample(i - 1)

        for j in range(self.vResult.size()):
            if i == 0:
                self.vResult[j] = -1.0 + 2.0 * self.rnd.NextDouble()
            else:
                self.vResult[j] = self.reg.GetSample(i - 1)[j]
        LogCurrent: float = self.reg.GetLogWeight(self.vResult.data())

        for _ in range(self.nMCMC * self.reg.GetPF().GetDimensions()):
            Index: int = self.rnd.NewValue() % self.vResult.size()
            Old: float = self.vResult[Index]

            self.vResult[Index] = -1.0 + 2.0 * self.rnd.NextDouble()
            LogNew: float = self.reg.GetLogWeight(self.vResult.data())

            if self.rnd.NextDouble() < math.exp(LogNew - LogCurrent):
                LogCurrent = LogNew
            else:
                self.vResult[Index] = Old

        return self.vResult.data()

    def Seed(self, n: int) -> None:
        self.rnd.Seed(n)
