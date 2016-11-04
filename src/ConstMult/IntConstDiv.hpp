/*
  Integer division by a small constant.

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2010.
  All rights reserved.

*/
#ifndef __INTCONSTDIV_HPP
#define __INTCONSTDIV_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Target.hpp"
#include "Table.hpp"

#define INTCONSTDIV_LINEAR_ARCHITECTURE 0      // from the ARC 2012 paper by Dinechin Didier
#define INTCONSTDIV_LOGARITHMIC_ARCHITECTURE 1 // from the 2015 Arith paper by Udurgag et al 
#define INTCONSTDIV_RECIPROCAL_ARCHITECTURE 2 // From the 2102 TCAS paper by Drane et al  
namespace flopoco{


	class IntConstDiv : public Operator
	{
	public:

#define INTCONSTDIV_OLDTABLEINTERFACE 0 // 0 for master, 1 for newPipelineFramework

#if INTCONSTDIV_OLDTABLEINTERFACE // deprecated overloading of Table method

		/** @brief This table inputs a number X on alpha + gammma bits, and computes its Euclidean division by d: X=dQ+R, which it returns as the bit string Q & R */
		class EuclideanDivTable: public Table {
		public:
			int d;
			int alpha;
			int rSize;
			EuclideanDivTable(Target* target, int d, int alpha, int rSize);
			mpz_class function(int x);
		};


		/** @brief This table is the CBLK table of the Arith23 paper by Ugurdag et al */
		class CBLKTable: public Table {
		public:
			int level; /**< input will be a group of 2^level alpha-bit digits*/
			int d;
			int alpha;
			int rSize;
			int rho;
			CBLKTable(Target* target, int level, int d, int alpha, int rSize, int rho);
			mpz_class function(int x);
		};
#else
		vector<mpz_class>  euclideanDivTable(int d, int alpha, int rSize);
		vector<mpz_class>  firstLevelCBLKTable(int d, int alpha, int rSize);
		vector<mpz_class>  otherLevelCBLKTable(int level, int d, int alpha, int rSize, int rho );

#endif // deprecated overloading of Table method


		/** @brief The atomic constructor, to be used for small constants
		* @param d The divisor.
		* @param n The size of the input X.
		* @param alpha The size of the chunk, or, use radix 2^alpha
		* @param architecture Architecture used, can be 0 for o(n) area, o(n) time, or 1 for o(n.log(n)) area, o(log(n)) time
		* @param remainderOnly As the name suggests
		*/

		IntConstDiv(Target* target, int wIn, int d, int alpha=-1, int architecture=0, bool computeQuotient=true, bool computeRemainder=true);


		/** @brief The composite constructor
		* @param dList The list of divisors
		* @param n The size of the input X.
		* @param alpha The size of the chunk, or, use radix 2^alpha
		* @param architecture Architecture used, can be 0 for linear are, linear time, or 1 for n log n area, log n time
		* @param remainderOnly As the name suggests
		*/
		IntConstDiv(Target* target, int wIn, vector<int> d, int alpha=-1, int architecture=0, bool computeQuotient=true, bool computeRemainder=true);

		~IntConstDiv();

		/** @brief the code generation method for an atomic divider */
		void generateVHDL();
		
		// Overloading the virtual functions of Operator
		// void outputVHDL(std::ostream& o, std::string name);

		void emulate(TestCase * tc);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);

		static TestList unitTest(int index);

		/** Factory register method */ 
		static void registerFactory();

	public:
		int quotientSize();   /**< Size in bits of the quotient output */
		int remainderSize();  /**< Size in bits of a remainder; rSize=ceil(log2(d-1)) */



	private:
		int d; /**<  Divisor*/
		int wIn;  /**<  Size in bits of the input X */
		int alpha; /**< Size of the chunk (should be between 1 and 16)*/
		int architecture; /** selects architecture. See the user interface help */
		bool computeQuotient; /**<  as the name suggests */
		bool computeRemainder; /**< as the name suggests */
		int rSize;  /**< Size in bits of a remainder; rSize=ceil(log2(d-1)) */
		int qSize;   /**< Size in bits of the quotient output */
		int rho;    /**< Size in bits of the quotient of one digit */

	};

}
#endif
