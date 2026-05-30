from .cdfvariance import CDFVariance
from .util import Vector

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .cregression import CRegression

class CDFConfidence(CDFVariance):
    vz: Vector[float]
    vZ: Vector[float]
    vX: Vector[float]
    vSigmaX: Vector[float]
    r: float
    var: float
    dev: float

    def __init__(self, reg: CRegression):
        super().__init__(reg)
        self.vz = Vector(self.Dimensions)
        self.vZ = Vector(self.Dimensions)
        self.vX = Vector(self.Parameters)
        self.vSigmaX = Vector(self.Parameters)

    def ComputeVariance(self, vInput: list[float]) -> None:
        raise ValueError("Not implemented")

    def GetVariance(self) -> float:
        return self.var

    def GetDeviation(self) -> float:
        return self.dev
