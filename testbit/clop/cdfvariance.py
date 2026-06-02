from .cdifffunction import CDiffFunction
from .cmatrixoperations import CMatrixOperations
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
    vH: Vector[float]
    MinSamples: int

    def __init__(self, reg: CRegression) -> None:
        super().__init__(reg.GetPF().GetDimensions())
        self.reg = reg
        self.Dimensions = reg.GetPF().GetDimensions()
        self.Parameters = reg.GetPF().GetParameters()
        self.vGradient = Vector(self.Dimensions)
        self.vH = Vector(self.Dimensions * self.Dimensions)
        self.MinSamples = 0

    def GetOutput(self, v: list[float]) -> float:
        return 0.0

    def GetGradient(self) -> Vector[float]:
        return self.vGradient

    def GetHessian(self) -> Vector[float]:
        return self.vH

    def CholeskySolve(self, v: Vector[float]) -> None:
        from .cregression import S_
        self.reg.EnsureState(S_.Cholesky)
        CMatrixOperations.Solve(self.reg.vCholesky.data(), v.data(), self.Parameters)

    def ComputeGradient(self) -> None:
        Epsilon: float = 0.0001

        v: Vector[float] = Vector(self.Dimensions)
        for i in range(self.Dimensions):
            v[i] = self.pvx[i]

        for i in range(self.Dimensions - 1, -1, -1):
            x = v[i]

            v[i] = x - Epsilon
            v0: float = self.GetOutput(v.data())

            v[i] = x + Epsilon
            v1: float = self.GetOutput(v.data())

            v[i] = x

            self.vGradient[i] = (v1 - v0) / (2 * Epsilon)

        self.GetOutput(v.data())

    def Normalize(self, x: float) -> float:
        if x < -1.0:
            return -1.0
        if x > 1.0:
            return 1.0
        return x
