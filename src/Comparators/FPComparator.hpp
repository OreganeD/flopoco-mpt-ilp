#ifndef FPCOMPARATPOR_HPP
#define FPCOMPARATPOR_HPP
#include "../Operator.hpp"


namespace flopoco{
	class FPComparator : public Operator{
	public:
		/**
		 * The FPComparator constructor
		 * @param[in]		parentOp	parent operator in the instance hierarchy
		 * @param[in]		target		target device
		 * @param[in]		wE				with of the exponent
		 * @param[in]		wF				with of the fraction
		 */
		FPComparator(OperatorPtr parentOp, Target* target, int wE, int wF, int flags=31, int method=-1);

		/**
		 * FPComparator destructor
		 */
		~FPComparator();

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);

		static TestList unitTest(int index);

		/** Factory register method */ 
		static void registerFactory();

		/** emulate() function  */
		void emulate(TestCase * tc);


		/** Regression tests */
		void buildStandardTestCases(TestCaseList* tcl);

	private:
		/** bit width of the exponent */
		int wE;
		/** bit width of the fraction */
		int wF;
		/** cmpflags, see flopoco doc */
		int flags;
		/** method, see flopoco doc */
		int method;
	};
}
#endif
