class CParametricFunction:
    PriorStrength: float
    Dimensions: int
    Parameters: int

    def __init__(self, Dimensions: int, Parameters: int) -> None:
        self.PriorStrength = 1e-2
        self.Dimensions = Dimensions
        self.Parameters = Parameters

    def GetDimensions(self) -> int:
        return self.Dimensions

    def GetParameters(self) -> int:
        return self.Parameters

    def GetPriorParam(self, vParam: list[float]) -> None:
        for i in range(self.Parameters):
            vParam[i] = 0.0

    def GetPrior(self, vParam: list[float]) -> float:
        Result: float = 0.0

        for i in range(self.Parameters):
            Result -= self.PriorStrength * vParam[i] * vParam[i] * 0.5

        return Result

    def GetPriorGradient(self, vParam: list[float], vGradient: list[float]) -> None:
        for i in range(self.Parameters):
            vGradient[i] = -self.PriorStrength * vParam[i]

    def GetPriorHessian(self, vParam: list[float], vHessian: list[float]) -> None:
        for i in range(self.Parameters * self.Parameters):
            vHessian[i] = 0.0
        for i in range(self.Parameters):
            vHessian[i * (self.Parameters + 1)] = self.PriorStrength

    def GetValue(self, vParam: list[float], vx: list[float]) -> float:
        raise ValueError("Not implemented")

    def GetMonomials(self, vx: list[float], vMonomial: list[float]) -> None:
        raise ValueError("Not implemented")
