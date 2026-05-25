class CSampleData:
    Weight: float = 0.0
    Replications: int = 0
    Index: int = 0
    tProba: list[float] = [0.0, 0.0]
    tCount: list[int] = [0, 0, 0]

    def GetCount(self) -> int:
        return self.tCount[0] + self.tCount[1] + self.tCount[2]

    def GetGradient(self) -> float:
        return self.Weight * (self.tCount[0] * (self.tProba[0] - 1.0) + self.tCount[1] * (1.0 - self.tProba[1]) + self.tCount[2] * (self.tProba[0] - self.tProba[1]))

    def GetHessian(self) -> float:
        return self.Weight * ((self.tCount[0] + self.tCount[2]) * self.tProba[0] * (1.0 - self.tProba[0]) + (self.tCount[1] + self.tCount[2]) * self.tProba[1] * (1.0 - self.tProba[1]))
