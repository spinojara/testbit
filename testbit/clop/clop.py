import random

from .cregression import CRegression
from .csamplingpolicy import CSamplingPolicy
from .cpfquadratic import CPFQuadratic
from .cresults import CResults, COutcome
from .cspweight import CSPWeight
from .cmesamplemean import CMESampleMean
from .util import Vector

def play_game(sample: list[float]) -> Outcome:

    weight = ((sample[0] - 0.44) ** 2 + (sample[1] + 0.23) ** 2 + (sample[2] - 0.78) ** 2) / 4
    if weight > 0.7:
        weight = 0.7

    weights = [weight, 0.3, 0.7 - weight]

    return random.choices([COutcome.Loss, COutcome.Draw, COutcome.Win], weights=weights, k=1)[0]


def main() -> int:

    Dimensions = 3
    results: CResults = CResults(Dimensions)
    reg: CRegression = CRegression(results, CPFQuadratic(Dimensions))
    sp: CSPWeight = CSPWeight(reg)
    me: CMESampleMean = CMESampleMean(reg)

    Replications: int = 2

    while True:
        Seed: int = results.GetSamples()
        print(Seed)
        if Seed % Replications == 0:
            results.AddSample(sp.NextSample(Seed))
        else:
            results.Reserve(Seed + 1)
            results.AddSample(results.GetSample(Seed - 1))

        outcome: Coutcome = play_game(results.GetSample(Seed))
        results.AddOutcome(Seed, outcome)
        print(results.GetSample(Seed))
        me.ComputeLocalWeights()
        vMax: Vector[float] = Vector(Dimensions)
        me.MaxParameter(vMax.data())
        print(vMax)

    return 0
