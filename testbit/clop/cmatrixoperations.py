import math

CholeksyThreshold = 1e-10

class CMatrixOperations:
    @staticmethod
    def Cholesky(vMatrix: list[float], vCholesky: list[float], Size: int) -> bool:
        for i in range(Size):
            for j in range(i, Size):
                Sum = vMatrix[i * Size + j]

                for k in range(i):
                    Sum -= vCholesky[i * Size + k] * vCholesky[j * Size + k]

                if i == j:
                    if Sum < CholeksyThreshold:
                        return False
                    else:
                        vCholesky[i * Size + i] = math.sqrt(Sum)
                else:
                    vCholesky[j * Size + i] = Sum / vCholesky[i * Size + i]
        return True
