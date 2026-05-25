import math

class CLogistic:
    @staticmethod
    def f(x: float) -> float:
        return 1.0 / (1.0 + math.exp(-x))
