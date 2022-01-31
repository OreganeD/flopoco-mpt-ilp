#ifndef TABLECOSTMODEL_HPP
#define TABLECOSTMODEL_HPP

#include <array>
#include <functional>
#include <tuple>

#include "gmpxx.h"

#include "Target.hpp"

namespace flopoco {
	using table_cost_function_t = std::function<mpz_class(int, int, Target *)>;
	using table_cost_description_t = std::tuple<const char *, const char*, table_cost_function_t>;

	mpz_class tableBitSize(int wIn, int wOut, Target *);

	// Use surface cost estimation from "An Empirical Study on Standard Cell Synthesis of Elementary Function Lookup Tables"
	// by Gustafsson and Johansson
	mpz_class GJHeuristic(int wIn, int wOut, Target *);

	//compute the lust number of luts needed to store the table
	mpz_class lutCost(int wIn, int wOut, Target * target);

	static std::array<table_cost_description_t, 3> const table_cost_models{
		table_cost_description_t{"StorageSize", "Cost is the number of bits to store in the table", tableBitSize},
		table_cost_description_t{"SCAModel", "Estimate area needed when building rom with standard cells following cost estimation in \"An Empirical Study on Standard Cell Synthesis of Elementary Function Lookup Tables\"", GJHeuristic},
		table_cost_description_t{"LutCost", "Cost is the estimated number of Luts needed to store the table", lutCost}
	};

	table_cost_function_t getGlobalCostModel();

	bool setGlobalCostModel(string const & key);
}

#endif // TABLECOSTMODEL_HPP
