#pragma once

#include "TilingStrategy.hpp"

#ifdef HAVE_SCALP
#include <ScaLP/Solver.h>
#include <ScaLP/Exception.h>    // ScaLP::Exception
#include <ScaLP/SolverDynamic.h> // ScaLP::newSolverDynamic
#endif //HAVE_SCALP
#include <iomanip>
#include "BaseMultiplier.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "MultiplierTileCollection.hpp"

namespace flopoco {

/*!
 * The TilingStrategyOptimalILP class
 */
class TilingStrategyOptimalILP : public TilingStrategy
{

public:
    using TilingStrategy::TilingStrategy;
    TilingStrategyOptimalILP(
        unsigned int wX,
        unsigned int wY,
        unsigned int wOut,
        bool signedIO,
        BaseMultiplierCollection* bmc,
		base_multiplier_id_t prefered_multiplier,
		float occupation_threshold,
		int maxPrefMult,
        MultiplierTileCollection tiles_,
        unsigned guardBits,
        unsigned keepBits,
        mpz_class errorBudget,
        mpz_class &centerErrConstant,
        bool performOptimalTruncation,
        bool squarer);

    void solve() override;

private:
    float occupation_threshold_;
    int dpX, dpY, dpS, dpC, wS, max_pref_mult_;
    vector<BaseMultiplierCategory*> tiles;
    unsigned prodWidth, guardBits, keepBits;
    mpz_class eBudget, &centerErrConstant;
    unsigned long long  errorBudget;
    bool performOptimalTruncation, squarer;
#ifdef HAVE_SCALP
    void constructProblem();

        ScaLP::Solver *solver;
#endif
};

}
