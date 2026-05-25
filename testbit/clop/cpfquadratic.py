from .cparametricfunction import CParametricFunction

class CPFQuadratic(CParametricFunction):
    def __init__(self, Dimensions: int) -> None:
        super().__init__(Dimensions, int((Dimensions + 1) * (Dimensions + 2) / 2))

    def GetValue(self, vParam: list[float], vx: list[float]) -> float:
        p: int = 0

        Result: float = vParam[p]
        p += 1

        i: int = self.Dimensions
        while True:
            i -= 1
            if i < 0:
                break
            Result += vParam[p] * vx[i]
            p += 1

        i: int = self.Dimensions
        while True:
            i -= 1
            if i < 0:
                break

            j: int = i + 1
            while True:
                j -= 1
                if j < 0:
                    break
                Result += vParam[p] * vx[i] * vx[j]
                p += 1

        return Result

    def GetHessian(self, vParam: list[float], vH: list[float]) -> None:
        p: int = Dimensions + 1
        i: int = Dimensions
        while True:
            i -= 1
            if i < 0:
                break

            j: int = i + 1
            while True:
                j -= 1
                if j < 0:
                    break
                if i == j:
                    vH[i * Dimensions + j] = vParam[p]
                else:
                    vH[i * Dimensions + j] = 0.5 * vParam[p]
                p += 1

    def GetMax(self, vParam: list[float], vx: list[float]) -> float:
        vMatrix: Vector[float] = Vector(self.Dimensions * self.Dimensions)
        while True:
            i -= 1
            if i < 0:
                break

            j: int = i + 1
            while True:
                j -= 1
                if j < 0:
                    break
                if i == j:
                    vMatrix[i * Dimensions + j] = -vParam[p]
                else:
                    vMatrix[j * Dimensions + i] = -0.5 * vParam[p]
                p += 1

        a = THIS_IS_

    def GetMonomials(self, vx: list[float], vMonomial: list[float]) -> None:
        p: int = 0

        vMonomial[p] = 1.0
        p += 1

        i: int = self.Dimensions
        while True:
            i -= 1
            if i < 0:
                break
            vMonomial[p] = vx[i]
            p += 1

        i: int = self.Dimensions
        while True:
            i -= 1
            if i < 0:
                break

            j: int = i + 1
            while True:
                j -= 1
                if j < 0:
                    break
                vMonomial[p] = vx[i] * vx[j]
                p += 1
