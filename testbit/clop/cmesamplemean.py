from .cmaxestimator import CMaxEstimator
from .cregression import CRegression

class CMESampleMean(CMaxEstimator):
    reg: CRegression

    def __init__(self, reg: CRegression) -> None:
        self.reg = reg

    def ComputeLocalWeights(self) -> None:
        self.reg.ComputeLocalWeights()

    def MaxParameter(self, vMax: list[float]) -> bool:
        TotalWeight: float = self.reg.GetTotalWeight()
        vTotalWeightedSample = self.reg.GetTotalWeightedSample()

        if TotalWeight > 0.0:
            for i in range(self.reg.GetPF().GetDimensions()):
                vMax[i] = vTotalWeightedSample[i] / TotalWeight

        return True
