#ifndef INTCONSTANTCOMPARATOR_HPP
#define INTCONSTANTCOMPARATOR_HPP
#include "../Operator.hpp"



namespace flopoco{
	class IntConstantComparator : public Operator{
	public:
		/**
		 * The IntConstantComparator constructor
		 * @param[in]		parentOp	parent operator in the instance hierarchy
		 * @param[in]		target		target device
		 * @param[in]		w				  input width
		 * @param[in]		flags			if bit 0 set, output  X<Y, if bit 1 set, X=Y, if bit 2 set, X>Y
		 */
		IntConstantComparator(OperatorPtr parentOp, Target* target, int w, mpz_class c, int flags=7, int method=-1);

		/**
		 * IntConstantComparator destructor
		 */
		~IntConstantComparator();

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

		/** cmpflags bit 0: <   bit 1: =  bit 2: >  */
		int flags;
		
		/** method */
		int method;

		/** the constant to compare to */
		mpz_class mpC;
	};
}
#endif
