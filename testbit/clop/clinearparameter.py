from .cparameter import CParameter

class CLinearParameter(CParameter):
    Min: float
    Max: float

    def __init__(self, s: str, Min: float, Max: float) -> None:
        super().__init__(s)
        self.Min = Min
        self.Max = Max

    def TransformToQLR(self, x: float) -> float:
        if self.Max < self.Min:
            return 0.0
        else:
            return -1.0 + 2.0 * (x - self.Min) / (self.Max - self.Min)

    def TransformFromQLR(self, x: float) -> float:
        return self.Min + (self.Max - self.Min) * (x + 1.0) / 2
