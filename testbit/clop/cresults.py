from .util import Vector
from .coutcome import COutcome

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .cobserver import CObserver

class CResults:
    Dimensions: int
    vSample: Vector[float]
    vOutcome: Vector[COutcome]
    lObs: Vector[CObserver]
    Samples: int

    def __init__(self, Dimensions: int) -> None:
        self.Dimensions = Dimensions
        self.Samples = 0
        self.vOutcome = Vector()
        self.vSample = Vector()
        self.lObs = Vector()

    def AddOutcome(self, i: int, outcome: COutcome) -> None:
        self.vOutcome[i] = outcome

        for obs in self.lObs.data():
            obs.OnOutcome(i)

    def GetOutcome(self, i: int) -> COutcome:
        return self.vOutcome[i]

    def AddSample(self, v: list[float], outcome: COutcome | None = None) -> int:
        self.Samples += 1

        if self.vOutcome.size() < self.Samples:
            self.vOutcome.resize(self.Samples)
            self.vSample.resize(self.Samples * self.Dimensions)

        for i in range(len(v)):
            self.vSample[(self.Samples - 1) * self.Dimensions + i] = v[i]

        self.vOutcome[self.Samples - 1] = COutcome.InProgress

        for obs in self.lObs.data():
            obs.OnSample()

        if outcome is not None:
            self.AddOutcome(self.Samples - 1, outcome)

        return self.Samples - 1

    def AddObserver(self, obs: CObserver) -> None:
        self.lObs.push_back(obs)

    def GetSamples(self) -> int:
        return self.Samples

    def Reserve(self, n: int) -> None:
        self.vSample.resize(n * self.Dimensions)
        self.vOutcome.resize(n)

    def GetSample(self, i: int) -> list[float]:
        return self.vSample.data()[i * self.Dimensions:(i + 1) * self.Dimensions]
