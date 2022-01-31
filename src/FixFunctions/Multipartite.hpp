#ifndef MULTIPARTITE_H
#define MULTIPARTITE_H

#include <vector>
#include <map>

#include "FixFunction.hpp"
#include "../Operator.hpp"
#include "../Tables/DifferentialCompression.hpp"


using namespace std;

namespace flopoco
{
	class FixFunctionByMultipartiteTable;
	typedef vector<vector<int>> expression;

	class Multipartite
	{
	public:
		const string uniqueName_=""; // for REPORT to work
		//---------------------------------------------------------------------------- Constructors/Destructor

		Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable* mpt_, Target* target);

		Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction* f_, int inputSize_, int outputSize_, Target* target);


		int64_t TIVFunction(int x);
		int64_t TOiFunction(int x, int ti);
		//------------------------------------------------------------------------------------- Public methods

		/**
		 * @brief buildGuardBitsAndSizes : Builds decomposition's guard bits and tables sizes
		 */
		void buildGuardBitsAndSizes(bool computeGuardBits=true);

		/**
		 * @brief mkTables : fill  the TIV and TOs tables
		 * @param target : The target FPGA
		 */
		void mkTables();

		/**
		 * @brief descriptionString(): describe the various parameters in textual form
		 */
		string descriptionString();
		/**
		 * @brief descriptionString(): describe the various parameters in textual form
		 */
		string descriptionStringLaTeX();
		/**
		 * @brief fullTableDump(): as the name suggests, for debug
		 */
		string 	fullTableDump();

		/**
		 * @brief returns true if the architecture is correct; false if it exceeds the target error
		 */
		bool exhaustiveTest();

	//---------------------------------------------------------------------------------- Public attributes
		FixFunction* f;

		int inputSize;
		int inputRange;
		int outputSize;

		double epsilonT;

		/** The number of TOi */
		int m;
		/** Just as in the article  */
		int alpha;
		/** Just as in the article */
		int beta;
		/** Just as in the article */
		vector<int> gammai;
		/** Just as in the article */
		vector<int> betai;
		/** Just as in the article */
		vector<int> pi;

		/** The Table of Initial Values, just as the ARITH 15 article */
		vector<int64_t> tiv;

		/** The first part of the compressed TIV table, just as in the Hsiao article */
		vector<int64_t> ssTIV;

		/** The second part of the compressed TIV table, just as in the Hsiao article */
		vector<int64_t> diffTIV;

		/** The m Tables of Offset , just as the ARITH 15 article */
		vector<vector<int64_t>> toi;

		double mathError;

		int guardBits;
		vector<int> outputSizeTOi;
		vector<int> sizeTOi;
		vector<bool> negativeTOi;
		int sizeTIV;
		int totalSize;
		/** the number of bits that address the first part of a compressed TIV (alpha -s) */
#if 0
		int rho;
		int outputSizeSSTIV;
		int outputSizeDiffTIV;
		int nbZeroLSBsInSSTIV;
		int sizeDiffTIV;
		int sizeSSTIV;
#else
		DifferentialCompression dcTIV;
#endif

		// holds precalculated TOi math errors. Valid as long as we don't change m!
		// Also a table of TIV compressions
		FixFunctionByMultipartiteTable *mpt;

	private:

		//------------------------------------------------------------------------------------ Private methods
		void computeTOiSize (int i);
		double deltai(int i);
		double mui(int i, int Bi);
		double si(int i, int Ai);
		void computeTIVCompressionParameters();


		//------------------------------------------------------------------------------------- Private attributes
		Target* _target;

	};

}
#endif // MULTIPARTITE_H
