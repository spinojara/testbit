from .cdifffunction import CDiffFunction
from .util import Vector

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .cregression import CRegression

class CDFVariance(CDiffFunction):
    reg: CRegression
    Dimensions: int
    Parameters: int

    pvx: list[float]
    vGradient: Vector[float]
    MinSamples: int

    def __init__(self, reg: CRegression) -> None:
        super().__init__(self.reg.GetPF().GetDimensions())
        self.reg = reg
        self.Dimensions = self.reg.GetPF().GetDimensions()
        self.Paramaters = self.reg.GetPF().GetParameters()
        self.vGradient = Vector(self.Dimensions)
        self.MinSamples = 0

