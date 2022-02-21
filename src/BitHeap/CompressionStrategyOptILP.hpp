#pragma once

#include "./../BitHeap/CompressionStrategy.hpp"

#ifdef HAVE_SCALP
#include <ScaLP/Solver.h>
#include <ScaLP/Exception.h>    // ScaLP::Exception
#include <ScaLP/SolverDynamic.h> // ScaLP::newSolverDynamic
#endif //HAVE_SCALP
#include <iomanip>


namespace flopoco {

/*!
 * The TilingAndCompressionOptILP class
 */
class CompressionStrategyOptILP : public CompressionStrategy
{

public:
    CompressionStrategyOptILP(BitHeap* bitheap);

    void solve();
	void compressionAlgorithm();

private:

    int dpS, wS, dpK, dpC, dpSt, s_max;
    int wIn, mod;
    bool consider_final_adder;
    using CompressorType = BasicCompressor::CompressorType;
    using subType = BasicCompressor::subType;

#ifdef HAVE_SCALP
    BasicCompressor* flipflop;
    vector<BasicCompressor*> pseudocompressors;
    void constructProblem(int s_max);
    bool addFlipFlop();
    void addRowAdder();
    void remove_all_but_Adders();

    ScaLP::Solver *solver;

    void resizeBitAmount(unsigned int stages);
    void printIOHeights(void);

    void C0_bithesp_input_bits(int s, int c, vector<ScaLP::Term> &bitsinColumn,
                               vector<vector<ScaLP::Variable>> &bitsInColAndStage);
    void C1_compressor_input_bits(int s, int c, vector<ScaLP::Term> &bitsinCurrentColumn,
                                  vector<vector<ScaLP::Variable>> &bitsInColAndStage);
    void C2_compressor_output_bits(int s, int c, vector<ScaLP::Term> &bitsinNextColumn,
                                   vector<vector<ScaLP::Variable>> &bitsInColAndStage);
    void C3_limit_BH_height(int s, int c, vector<vector<ScaLP::Variable>> &bitsInColAndStage, bool consider_final_adder);
    void C5_RCA_dependencies(int s, int c, vector<vector<ScaLP::Term>> &rcdDependencies);

    void drawBitHeap();
    void replace_row_adders(BitHeapSolution &solution, vector<vector<vector<int>>> &row_adders);
    void handleRowAdderDependencies(const ScaLP::Variable &tempV, vector<ScaLP::Term> &bitsinColumn,
                                    vector<vector<ScaLP::Term>> &rcdDependencies,
                                    unsigned int c, unsigned int e) const;
#endif



    };

}
