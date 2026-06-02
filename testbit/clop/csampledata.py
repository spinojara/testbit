class CSampleData:
    Weight: float
    Replications: int
    Index: int
    tProba: list[float]
    tCount: list[int]

    def __init__(self) -> None:
        self.Weight = 0.0
        self.Replications = 0
        self.Index = 0
        self.tProba = [0.0, 0.0]
        self.tCount = [0, 0, 0]

    def GetCount(self) -> int:
        return self.tCount[0] + self.tCount[1] + self.tCount[2]

    def GetGradient(self) -> float:
        return self.Weight * (self.tCount[0] * (self.tProba[0] - 1.0) + self.tCount[1] * (1.0 - self.tProba[1]) + self.tCount[2] * (self.tProba[0] - self.tProba[1]))

    def GetHessian(self) -> float:
        return self.Weight * ((self.tCount[0] + self.tCount[2]) * self.tProba[0] * (1.0 - self.tProba[0]) + (self.tCount[1] + self.tCount[2]) * self.tProba[1] * (1.0 - self.tProba[1]))
