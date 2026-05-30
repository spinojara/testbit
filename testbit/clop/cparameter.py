class CParameter:
    sName: str

    def __init__(self, sName: str) -> None:
        self.sName = sName

    def GetName(self) -> str:
        return self.sName

    def TransformToQLR(self, x: float) -> float:
        raise ValueError("Not implemented")

    def TransformFromQLR(self, x: float) -> float:
        raise ValueError("Not implemented")
