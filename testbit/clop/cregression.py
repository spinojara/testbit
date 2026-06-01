# Regression
from enum import IntEnum
from typing import Callable
from dataclasses import dataclass, field
import copy
import math

from .cparametricfunction import CParametricFunction
from .cresults import CResults
from .cobserver import CObserver
from .util import Vector
from .coutcome import COutcome
from .csampledata import CSampleData
from .clogistic import CLogistic
from .cdflogp import CDFLogP
from .cmatrixoperations import CMatrixOperations
from .cdfconfidence import CDFConfidence

class S_(IntEnum):
    NoState = 0x0
    LogP = 0x1
    Gradient = 0x2
    Hessian = 0x4
    Cholesky = 0x8
    CholeskyInverse = 0x10
    MAP = 0x20
    All = 0x3f

@dataclass
class CWeighting:
    vParam: Vector[float] = field(default_factory=Vector)
    Radius: float = 0.0
    Mean: float = 0.0

class CRegression(CObserver):
    pf: CParametricFunction
    Samples: int
    vsd: Vector[CSampleData]
    tCount: list[int] = [0, 0, 0]
    DrawRating: float = 100 * math.log(10.0) / 400.0
    vParamMAP: Vector[float]
    vMonomial: Vector[float]
    L: float
    vGradient: Vector[float]
    vHessian: Vector[float]
    vCholesky: Vector[float]
    vCholeskyInverse: Vector[float]
    fCholesky: bool
    fAutoLocalize: bool = True
    fDirty: bool
    State: int
    RefreshRate: float
    Refreshcount: int
    NextRefresh: int
    LocalizationHeight: float = 3.0
    LocalizationPower: float = 0.0
    MaxWeightIterations: int = 7
    vWeighting: Vector[CWeighting]
    TotalWeight: float
    vTotalWeightedSample: Vector[float]
    lObs: list[Callable[[CRegression], None]]

    def __init__(self, results: CResults, pf: CParametricFunction) -> None:
        super().__init__(results)
        self.pf = pf
        self.vParamMAP = Vector(pf.GetParameters())
        self.vMonomial = Vector(pf.GetParameters())
        self.vGradient = Vector(pf.GetParameters())
        self.vHessian = Vector(pf.GetParameters() * pf.GetParameters())
        self.vCholesky = Vector(pf.GetParameters() * pf.GetParameters())
        self.vCholeskyInverse = Vector(pf.GetParameters() * pf.GetParameters())
        self.RefreshRate = 0.0
        self.vTotalWeightedSample = Vector(pf.GetDimensions())
        self.vsd = Vector(0, CSampleData)
        self.vWeighting = Vector()
        self.lObs = []

        self.OnReset()

    def AddObserver(self, obs: Callable[[CRegression], None]) -> None:
        self.lObs.append(obs)

    def GetSample(self, i: int) -> list[float]:
        return self.results.GetSample(i)

    def GetPF(self) -> CParametricFunction:
        return self.pf

    def OnReset(self) -> None:
        self.Samples = self.results.GetSamples()
        if self.vsd.size() < self.Samples:
            self.vsd.resize(self.Samples)

        self.tCount = [0, 0, 0]
        for i in range(self.Samples):
            if self.results.GetOutcome(i) < 3:
                self.tCount[self.results.GetOutcome(i)] += 1

        self.RefreshCounter = 0
        self.NextRefresh = 0

        self.SetUniformWeights()
        self.pf.GetPriorParam(self.vParamMAP.data())

        self.State = S_.NoState
        self.fDirty = False
        self.fAutoLocalize = True

    def OnSample(self) -> None:
        self.Samples = self.results.GetSamples()
        if self.vsd.size() < self.Samples:
            self.vsd.resize(self.Samples)

        k = self.Samples - 1

        if k > 0 and self.GetSample(k - 1) == self.GetSample(k):
            self.vsd[k] = copy.deepcopy(self.vsd[k - 1])
            self.vsd[k].Replications += 1
        else:
            self.vsd[k].Weight = self.GetWeight(self.GetSample(k))
            self.vsd[k].Index = k
            self.vsd[k].Replications = 1
            self.vsd[k].tCount = [0, 0, 0]

        self.AddWeightedSample(k, 1)

    def OnOutcome(self, i: int) -> None:
        self.fDirty = True
        outcome: COutcome = self.results.GetOutcome(i)
        if outcome < 3:
            self.tCount[outcome] += 1
            self.vsd[self.vsd[i].Index].tCount[outcome] += 1

        if self.RefreshCounter == self.NextRefresh:
            self.State = S_.NoState
            self.fDirty = False
            self.NextRefresh += 1 + int(self.NextRefresh * self.RefreshRate)

            if self.fAutoLocalize:
                self.ComputeLocalWeights()

        self.RefreshCounter += 1

    def GetLogWeight(self, vx: list[float]) -> float:
        lw: float = 0.0

        for i in range(self.vWeighting.size()):
            v_i = self.pf.GetValue(self.vWeighting[i].vParam.data(), vx)
            lw_i = (v_i - self.vWeighting[i].Mean) / self.vWeighting[i].Radius
            if lw_i < lw:
                lw = lw_i

        return lw

    def GetWeight(self, vx: list[float]) -> float:
        return math.exp(self.GetLogWeight(vx))

    def ResetWeightedSample(self) -> None:
        self.vTotalWeightedSample.fill(0.0)
        self.TotalWeight = 0.0

    def AddWeightedSample(self, k: int, Count: int) -> None:
        Weight: float = self.vsd[k].Weight * Count
        self.TotalWeight += Weight
        for i in range(self.pf.GetDimensions()):
            self.vTotalWeightedSample[i] += Weight * self.GetSample(k)[i]

    def UpdateWeights(self) -> None:
        self.ResetWeightedSample()
        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break
            k = self.vsd[k].Index
            self.vsd[k].Weight = self.GetWeight(self.GetSample(k))
            self.AddWeightedSample(k, self.vsd[k].GetCount())

    def GetMeanAndDeviation(self) -> tuple[float, float]:
        tWC: list[float] = [0.0, 0.0, 0.0]
        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break
            k = self.vsd[k].Index
            tWC[0] += self.vsd[k].Weight * self.vsd[k].tCount[0]
            tWC[1] += self.vsd[k].Weight * self.vsd[k].tCount[1]
            tWC[2] += self.vsd[k].Weight * self.vsd[k].tCount[2]

        GammaD: float = math.exp(-self.DrawRating)
        Delta: float = (tWC[1] - tWC[0]) ** 2 + 4 * GammaD ** 2 * (tWC[0] + tWC[2]) * (tWC[1] + tWC[2])
        Gamma: float = (tWC[1] - tWC[0] + math.sqrt(Delta)) / (2 * GammaD * (tWC[0] + tWC[2]))
        Mean: float = math.log(Gamma)

        PWin: float = CLogistic.f(Mean - self.DrawRating)
        PLoss: float = CLogistic.f(-Mean - self.DrawRating)
        x: float = (tWC[1] + tWC[2]) * PWin * (1.0 - PWin) + (tWC[0] + tWC[2]) * PLoss * (1.0 - PLoss)
        Deviation: float = math.sqrt(1.0 / x)

        return Mean, Deviation

    def LocalizationIteration(self) -> None:
        if self.LocalizationHeight > 0 and self.tCount[COutcome.Loss] > 1 and self.tCount[COutcome.Win] > 1:
            Mean, Deviation = self.GetMeanAndDeviation()

            self.EnsureState(S_.MAP)
            weighting: CWeighting = CWeighting()
            weighting.vParam = self.vParamMAP
            weighting.Radius = Deviation * self.LocalizationHeight * self.Samples ** self.LocalizationPower
            weighting.Mean = Mean
            self.vWeighting.push_back(weighting)

            self.UpdateWeights()
            self.State = S_.NoState

    def SetUniformWeights(self) -> None:
        self.vWeighting.resize(0)

        self.ResetWeightedSample()
        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break
            k = self.vsd[k].Index
            self.vsd[k].Weight = 1.0
            self.AddWeightedSample(k, self.vsd[k].GetCount())

        self.vParamMAP.fill(0.0)

        self.State = S_.NoState

    def ComputeLocalWeights(self) -> None:
        self.SetUniformWeights()
        PreviousTotalWeight: float

        Iterations: int = 0
        while True:
            PreviousTotalWeight = self.TotalWeight
            self.LocalizationIteration()
            Iterations += 1
            if self.MaxWeightIterations and Iterations >= self.MaxWeightIterations:
                break
            if self.TotalWeight < PreviousTotalWeight * 0.99:
                break

        if self.MaxWeightIterations == 0 and self.vWeighting.size() > 0:
            self.vWeighting.pop_back()
            self.UpdateWeights()

        for obs in self.lObs:
            obs(self)

    def GetPosteriorInfo(self, vLocation: list[float]) -> tuple[float, float]:
        Rating = self.pf.GetValue(self.vParamMAP.data(), vLocation)
        dfconf: CDFConfidence = CDFConfidence(self)
        dfconf.ComputeVariance(vLocation)
        Variance = dfconf.GetVariance()
        return Rating, Variance

    def GetParamPositivity(self, i: int) -> float:
        self.EnsureState(S_.MAP | S_.Hessian)
        x = self.vParamMAP[i]
        h = self.vHessian[i * (self.pf.GetParameters() + 1)]
        return math.sqrt(h) * x

    def EnsureState(self, Flags: int) -> None:
        while (Flags & ~self.State):
            if self.fDirty:
                self.fDirty = False
                self.State = S_.NoState

            if (Flags & ~self.State) & S_.MAP:
                self.Newton()

            if (Flags & ~self.State) & S_.LogP:
                self.UpdateLogP()

            if (Flags & ~self.State) & S_.Gradient:
                self.ComputeGradient()

            if (Flags & ~self.State) & S_.Hessian:
                self.ComputeHessian()

            if (Flags & ~self.State) & S_.Cholesky:
                self.EnsureState(S_.Hessian)
                self.fCholesky = CMatrixOperations.Cholesky(self.vHessian.data(), self.vCholesky.data(), self.pf.GetParameters())
                self.State |= S_.Cholesky

            if (Flags & ~self.State) & S_.CholeskyInverse:
                self.EnsureState(S_.Cholesky)
                CMatrixOperations.Inverse(self.vCholesky.data(), self.vCholeskyInverse.data(), self.pf.GetParameters())
                self.State |= S_.CholeskyInverse

    def Newton(self) -> None:
        func: CDFLogP = CDFLogP(self)
        v: Vector[float] = copy.deepcopy(self.vParamMAP)

        v = func.Newton(v)
        self.vParamMap = copy.deepcopy(v)
        func.GetOutput(v.data())

        self.State = S_.MAP | S_.LogP

    def UpdateLogP(self) -> None:
        self.L = self.pf.GetPrior(self.vParamMAP.data())

        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break

            k = self.vsd[k].Index

            r: float = self.pf.GetValue(self.vParamMAP.data(), self.GetSample(k))
            self.vsd[k].tProba[0] = CLogistic.f(-r - self.DrawRating)
            self.vsd[k].tProba[1] = CLogistic.f(+r - self.DrawRating)

            tLogP: list[float] = [0.0, 0.0, 0.0]
            if self.vsd[k].tProba[0] > 0.0:
                tLogP[COutcome.Loss] = math.log(self.vsd[k].tProba[0])
            else:
                tLogP[COutcome.Loss] = -r - self.DrawRating

            if self.vsd[k].tProba[1] > 0.0:
                tLogP[COutcome.Win] = math.log(self.vsd[k].tProba[1])
            else:
                tLogP[COutcome.Win] = +r - self.DrawRating

            tLogP[COutcome.Draw] = tLogP[COutcome.Loss] + tLogP[COutcome.Win]

            self.L += self.vsd[k].Weight * (tLogP[0] * self.vsd[k].tCount[0] + tLogP[1] * self.vsd[k].tCount[1] + tLogP[2] * self.vsd[k].tCount[2])
        self.State |= S_.LogP

    def ComputeGradient(self) -> None:
        self.EnsureState(S_.LogP)

        self.pf.GetPriorGradient(self.vParamMAP.data(), self.vGradient.data())

        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break
            k = self.vsd[k].Index
            TotalMul: float = self.vsd[k].GetGradient()

            self.pf.GetMonomials(self.GetSample(k), self.vMonomial.data())
            for p in range(self.pf.GetParameters()):
                self.vGradient[p] += TotalMul * self.vMonomial[p]

        self.State |= S_.Gradient

    def ComputeHessian(self) -> None:
        self.EnsureState(S_.LogP)

        self.pf.GetPriorHessian(self.vParamMAP.data(), self.vHessian.data())

        k: int = self.Samples
        while True:
            k -= 1
            if k < 0:
                break
            k = self.vsd[k].Index
            TotalMul: float = self.vsd[k].GetHessian()

            self.pf.GetMonomials(self.GetSample(k), self.vMonomial.data())
            p1: int = self.pf.GetParameters()
            while True:
                p1 -= 1
                if p1 < 0:
                    break

                p2: int = self.pf.GetParameters()
                p2 -= 1
                if p2 < p1:
                    break
                self.vHessian[p1 * self.pf.GetParameters() + p2] += TotalMul * self.vMonomial[p1] * self.vMonomial[p2]
        self.State |= S_.Hessian

    def GetTotalWeight(self) -> float:
        return self.TotalWeight

    def GetTotalWeightedSample(self) -> list[float]:
        return self.vTotalWeightedSample.data()

    def GetReplications(self, i: int) -> int:
        return self.vsd[i].Replications

    def SetRefreshRate(self, x: float) -> None:
        self.RefreshRate = x

    def SetAutoLocalize(self, f: bool) -> None:
        self.fAutoLocalize = f

    def GetSampleData(self, i: int) -> CSampleData:
        return self.vsd[i]

    def MAP(self) -> list[float]:
        self.EnsureState(S_.MAP)
        return self.vParamMAP.data()
