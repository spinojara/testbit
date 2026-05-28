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

        i = self.Dimensions
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
        p: int = self.Dimensions + 1
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
                if i == j:
                    vH[i * self.Dimensions + j] = vParam[p]
                else:
                    vH[i * self.Dimensions + j] = 0.5 * vParam[p]
                p += 1

    def GetMax(self, vParam: list[float], vx: list[float]) -> float:
        raise ValueError("Not done")

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

        i = self.Dimensions
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
