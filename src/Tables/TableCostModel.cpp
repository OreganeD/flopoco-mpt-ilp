#include "TableCostModel.hpp"
#include <cmath>
#include <iostream>

using std::pow;

namespace flopoco {

	mpz_class tableBitSize(int wIn, int wOut, Target *)
	{
		if (wIn == 0) return {0};
		return mpz_class{wOut} << wIn;
	}

	mpz_class GJHeuristic(int wIn, int wOut, Target *)
	{
		if (wIn == 0) return {0};
		auto wMin = min(wIn, wOut);
		auto wMax = max(wIn, wOut);
		auto shiftVal = 0.65*wMin + 0.19*(wMax-wMin);

		auto cost = pow(2., shiftVal);

		auto res = mpz_class{cost};
		return res;
	}

	mpz_class lutCost(int wIn, int wOut, Target * target)
	{
		if (wIn == 0) return {0};
		auto lutWORouting = target->lutConsumption(wIn);
		if (lutWORouting > 0.) {
			return mpz_class{lutWORouting * wOut};
		}
		auto maxLutWidth = target->maxLutInputs();
		auto maxLutCost = target->lutConsumption(maxLutWidth);
		auto remain = wIn - maxLutWidth;
		auto finLutCost = mpz_class{maxLutCost * wOut};
		finLutCost <<= remain;
		return finLutCost;
	}

	static table_cost_function_t _globalTableCostModel = tableBitSize;

	table_cost_function_t getGlobalCostModel()
	{
		return _globalTableCostModel;
	}

	bool setGlobalCostModel(string const & key) {
		for (auto& i : table_cost_models) {
			if (std::get<0>(i) == key) {
				_globalTableCostModel = std::get<2>(i);
				return true;
			}
		}
		return false;
	}
}
