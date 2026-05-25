from .cdifffunction import CDiffFunction

class CDFLogP(CDiffFunction):
    reg: CRegression

    def __init__(self, reg: CRegression) -> None:
        super().__init__(reg.GetPF().GetParameters())
        self.reg = reg

    def GetGradient(self) -> Vector[float]:
        return self.reg.vGradient

    def GetHessian(self) -> Vector[float]:
        return self.reg.vHessian

    def GetOutput(self, vInput: list[float]) -> float:
        for i in range(self.reg.GetPF().GetParameters()):
            if vInput[i] != self.reg.vParamMAP[i]:
                self.reg.vParamMAP[i] = vInput[i]
                self.reg.State = 0
            from .cregression import S_
            self.reg.EnsureState(S_.LogP)
        return self.reg.L

    def ComputeGradient(self) -> None:
        from .cregression import S_
        self.reg.EnsureState(S_.Gradient)

    def ComputeHessian(self) -> None:
        from .cregression import S_
        self.reg.EnsureState(S_.Hessian)
