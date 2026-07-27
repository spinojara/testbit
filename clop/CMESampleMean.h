/////////////////////////////////////////////////////////////////////////////
//
// CMESampleMean.h
//
// Rémi Coulom
//
// June, 2010
//
/////////////////////////////////////////////////////////////////////////////
#ifndef CMESampleMean_Declared
#define CMESampleMean_Declared

#include "CMaxEstimator.h"
#include "CRegression.h"

class CRegression;

class CMESampleMean : public CMaxEstimator // mesm
{
      private: ///////////////////////////////////////////////////////////////////
	CRegression &reg;

      public: ////////////////////////////////////////////////////////////////////
	explicit CMESampleMean(CRegression &reg) : reg(reg) {}

	void ComputeLocalWeights() const override { reg.ComputeLocalWeights(); }

	bool MaxParameter(double vMax[]) const;
};

#endif
