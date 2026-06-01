import math

class CLogistic:
    @staticmethod
    def f(x: float) -> float:
        if x < -500.0:
            return 0.0
        return 1.0 / (1.0 + math.exp(-x))
