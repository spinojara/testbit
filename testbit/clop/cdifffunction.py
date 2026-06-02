import copy
import math

from .util import Vector
from .cmatrixoperations import CMatrixOperations

NewtonThreshold: float = 1e-5
MinNewtonStep: float = 1e-10
MaxBeta: float = 100.0
CGEpsilon: float = 1e-10
MaxNewtonIterations: int = 100
MaxSDIterations: int = 100
MaxCGIterations: int = 100

class CDiffFunction:
    Dimensions: int
    vCholesky: Vector[float]
    vStep: Vector[float]
    vGCopy: Vector[float]
    vxTemp: Vector[float]


    def __init__(self, Dimensions: int) -> None:
        self.Dimensions = Dimensions
        self.vCholesky = Vector(self.Dimensions * self.Dimensions)
        self.vStep = Vector(self.Dimensions)
        self.vGCopy = Vector(self.Dimensions)
        self.vxTemp = Vector(self.Dimensions)

    def GetDimensions(self) -> int:
        return self.Dimensions

    def Newton(self, vMax: Vector[float], fTrace: bool = False) -> Vector[float]:
        vG = self.GetGradient()
        vH = self.GetHessian()
        n: int = self.GetDimensions()

        for Iterations in range(MaxNewtonIterations):
            L: float = self.GetOutput(vMax.data())
            self.ComputeGradient()
            self.ComputeHessian()

            self.vStep = copy.deepcopy(vG)
            if CMatrixOperations.Cholesky(vH.data(), self.vCholesky.data(), n):
                CMatrixOperations.Solve(self.vCholesky.data(), self.vStep.data(), n)
            else:
                self.CG(vMax.data(), fTrace)
                break

            for i in range(n):
                self.vxTemp[i] = self.Normalize(vMax[i] + self.vStep[i])

            LNew: float = self.GetOutput(self.vxTemp.data())
            NewtonStep: float = 1.0

            if (LNew != LNew or LNew < L) and NewtonStep > MinNewtonStep:
                self.CG(vMax.data(), fTrace)
                break

            vMax = self.vxTemp
            if LNew - L < NewtonThreshold:
                break
        return vMax

    def CG(self, vMax: list[float], fTrace: bool = False) -> None:
        vG = self.GetGradient()
        vPrevG: Vector[float]
        vD: Vector[float]

        Cycles: int = self.GetDimensions()

        for Iteration in range(MaxCGIterations):
            self.GetOutput(vMax)
            self.ComputeGradient()

            Cycle = Iteration % Cycles
            if Cycle == 0:
                vPrevG = copy.deepcopy(vG)
                vD = copy.deepcopy(vG)
            else:
                Num: float = 0.0
                Den: float = 0.0

                for i in range(self.GetDimensions()):
                    Num += vG[i] * (vG[i] - vPrevG[i])
                    Den += vPrevG[i] * vPrevG[i]

                if Den == 0.0 or Num / Den > MaxBeta:
                    Beta: float = MaxBeta
                else:
                    Beta = Num / Den

                for i in range(self.GetDimensions()):
                    vD[i] = vG[i] + Beta * vD[i]
                    vPrevG[i] = vG[i]

            x_: int = Iteration % self.GetDimensions()
            if vD[x_] * vD[x_] == 0.0:
                if vD[x_] >= 0:
                    vD[x_] = 1.0
                else:
                    vD[x_] = -1.0

            x: float = self.LineOpt(vMax, vD.data(), fTrace)

            Delta2: float = 0.0
            for i in range(self.GetDimensions()):
                New: float = self.Normalize(vMax[i] + x * vD[i])
                Delta: float = New - vMax[i]
                vMax[i] = New
                Delta2 += Delta * Delta

            if Cycle == 0 and Delta2 < CGEpsilon:
                return

    def LineOpt(self, vx0: list[float], vDir: list[float], fTrace: bool = False) -> float:
        Epsilon: float = 0.00001
        Big: float = 10000.0

        N2: float = 0.0
        for i in range(self.Dimensions):
            N2 += vDir[i] * vDir[i]
        Scale: float = 1.0 / math.sqrt(N2)
        if Scale == float("inf"):
            return 0.0

        tx: list[float] = [0.0, 0.0, 0.0]
        tf: list[float] = [0.0, 0.0, 0.0]

        tx[0] = 0.0
        tf[0] = self.SetLineInput(vx0, vDir, tx[0])

        tx[2] = Scale
        tf[2] = self.SetLineInput(vx0, vDir, tx[2])

        while True:
            tx[1] = tx[2] * 0.5
            tf[1] = self.SetLineInput(vx0, vDir, tx[1])

            if tx[1] < Epsilon:
                return 0.0

            if tf[1] <= tf[0]:
                tx[2] = tx[1]
                tf[2] = tf[1]
            else:
                break

        while tf[1] <= tf[2]:
            if tx[2] > Big:
                return tx[2]

            tx[1] = tx[2]
            tf[1] = tf[2]
            tx[2] = tx[1] * 2.0
            tf[2] = self.SetLineInput(vx0, vDir, tx[2])

        bma: float = tx[1] - tx[0]
        bmc: float = tx[1] - tx[2]
        fbmfa: float = tf[1] - tf[0]
        fbmfc: float = tf[1] - tf[2]

        x: float = tx[1] - 0.5 * (bma ** 2 * fbmfc - bmc ** 2 * fbmfa) / (bma * fbmfc - bmc * fbmfa)

        f: float = self.SetLineInput(vx0, vDir, x)

        if f > tf[1]:
            return x
        else:
            return tx[1]

    def SetLineInput(self, vx0: list[float], vDir: list[float], x: float) -> float:
        for i in range(self.Dimensions):
            self.vxTemp[i] = vx0[i] + x * vDir[i]
        return self.GetOutput(self.vxTemp.data())

    def Normalize(self, x: float) -> float:
        return x

    def GetOutput(self, vInput: list[float]) -> float:
        raise ValueError("Not implemented")

    def GetGradient(self) -> Vector[float]:
        raise ValueError("Not implemented")

    def GetHessian(self) -> Vector[float]:
        raise ValueError("Not implemented")

    def ComputeGradient(self) -> None:
        raise ValueError("Not implemented")

    def ComputeHessian(self) -> None:
        raise ValueError("Not implemented")
