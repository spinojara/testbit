from .cresults import CResults

class CObserver:
    results: CResults

    def __init__(self, results: CResults):
        self.results = results
        self.results.AddObserver(self)
