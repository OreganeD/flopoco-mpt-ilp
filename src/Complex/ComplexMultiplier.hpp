
#ifndef COMPLEXMULTIPLIER_HPP
#define COMPLEXMULTIPLIER_HPP
#include <vector>
#include <sstream>

#include "../Operator.hpp"
#include "../FPMultiplier.hpp"
#include "../FPAddSinglePath.hpp"

namespace flopoco{

	class ComplexMultiplier : public Operator
	{
	public:
		ComplexMultiplier(Target* target, int wE, int wF, bool hasLessMultiplications);
		~ComplexMultiplier();
		
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		TestCase* buildRandomTestCase(int i);

		int wE, wF;

	};
}
#endif
