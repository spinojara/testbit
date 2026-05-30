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

    @staticmethod
    def Solve(vMatrix: list[float], v: list[float], Size: int) -> None:
        for i in range(Size):
            j: int = i
            while True:
                j -= 1
                if j < 0:
                    break
                v[i] -= v[j] * vMatrix[i * Size + j]
            v[i] /= vMatrix[i * Size + i]

        i = Size
        while True:
            i -= 1
            if i < 0:
                break

            j = i + 1
            while True:
                if j >= Size:
                    break
                v[i] -= vMatrix[j * Size + i]
                j += 1
            v[i] /= vMatrix[i * Size + i]

    @staticmethod
    def Inverse(vMatrix: list[float], vInverse: list[float], Size: int) -> None:
        raise ValueError("Not implemented")
