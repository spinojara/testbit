from typing import Callable

from .cparameter import CParameter
from .cresults import CResults
from .cspweight import CSPWeight
from .cregression import CRegression
from .cmesamplemean import CMESampleMean
from .cpfquadratic import CPFQuadratic
from .coutcome import COutcome


class CExperiment:
    paramcol: list[CParameter]
    results: CResults
    me: CMESampleMean
    sp: CSPWeight
    reg: CRegression

    def __init__(self, paramcol: list[CParameter], seed: int) -> None:
        Dimensions = len(paramcol)
        self.paramcol = paramcol

        self.results = CResults(Dimensions)

        self.reg: CRegression = CRegression(self.results, CPFQuadratic(Dimensions))
        self.reg.SetRefreshRate(0.1)
        self.sp: CSPWeight = CSPWeight(self.reg)
        self.me: CMESampleMean = CMESampleMean(self.reg)
        self.sp.Seed(seed)

    def next_sample(self) -> tuple[dict[str, float], int, float]:
        Seed: int = self.results.GetSamples()
        self.results.AddSample(self.sp.NextSample(Seed))

        Seed = self.results.GetSamples()
        self.results.Reserve(Seed + 1)
        self.results.AddSample(self.results.GetSample(Seed - 1))

        return self.dict_from_sample(self.results.GetSample(Seed - 1)), Seed - 1, self.reg.GetWeight(self.results.GetSample(Seed - 1))

    def add_outcome(self, Seed: int, w: int, d: int, l: int) -> None:
        for i in range(w):
            self.results.AddOutcome(Seed, COutcome.Win)
            Seed += 1
        for i in range(d):
            self.results.AddOutcome(Seed, COutcome.Draw)
            Seed += 1
        for i in range(l):
            self.results.AddOutcome(Seed, COutcome.Loss)
            Seed += 1

    def add_sample(self, parammap: dict[str, float], w: int, d: int, l: int) -> None:
        sample: list[float] = self.sample_from_dict(parammap)

        for i in range(w):
            self.results.AddSample(sample, COutcome.Win)
        for i in range(d):
            self.results.AddSample(sample, COutcome.Draw)
        for i in range(l):
            self.results.AddSample(sample, COutcome.Loss)

    def sample_from_dict(self, parammap: dict[str, float]) -> list[float]:
        return [param.TransformToQLR(parammap[param.GetName()]) for param in self.paramcol]

    def dict_from_sample(self, sample: list[float]) -> dict[str, float]:
        return {param.GetName(): param.TransformFromQLR(sample[index]) for index, param in enumerate(self.paramcol)}
