#ifndef FPREALKCM_HPP
#define FPREALKCM_HPP
#include "../Operator.hpp"
#include "../Tables/Table.hpp"

namespace flopoco{



	class FPRealKCM : public Operator
	{
	public:
		FPRealKCM (Target* target, int wE, int wF,  string constant, map<string, double> inputDelays = emptyDelayMap);
		~FPRealKCM();

		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);

		int wE;
		int wF;
		string constant;
		mpfr_t mpC;

		static OperatorPtr parser(Target *target, vector<string> &args);
		static void registerFactory(void);


	};



}


#endif
