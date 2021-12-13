#ifndef FPCOMPARATPOR_HPP
#define FPCOMPARATPOR_HPP
#include "../Operator.hpp"


namespace flopoco{
	class IntComparator : public Operator{
	public:
		/**
		 * The IntComparator constructor
		 * @param[in]		parentOp	parent operator in the instance hierarchy
		 * @param[in]		target		target device
		 * @param[in]		w				  input width
		 * @param[in]		flags			if bit 0 set, output  X<Y, if bit 1 set, X=Y, if bit 2 set, X>Y
		 */
		IntComparator(OperatorPtr parentOp, Target* target, int w, int flags=7, int method=0);

		/**
		 * IntComparator destructor
		 */
		~IntComparator();

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
		/** bit width */
		int w;
		/** cmpflags */
		int flags;
		/** method */
		int method;
	};
}
#endif
