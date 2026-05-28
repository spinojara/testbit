from .cresults import CResults

class CObserver:
    results: CResults

    def __init__(self, results: CResults):
        self.results = results
        self.results.AddObserver(self)

    def OnOutcome(self, i: int) -> None:
        raise ValueError("Not implemented")

    def OnSample(self) -> None:
        raise ValueError("Not implemented")
