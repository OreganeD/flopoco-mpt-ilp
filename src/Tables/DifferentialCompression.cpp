#include "DifferentialCompression.hpp"
#include "Table.hpp"
#include <iostream>
#include <cassert>

using std::pair;
using std::make_pair;

namespace flopoco {
	using min_max_t = pair<mpz_class, mpz_class>;

	auto groupSlices(vector<min_max_t> const & min_max_val)
	{
		auto min_max_size = min_max_val.size();
		auto ret_size = min_max_size / 2;
		vector<min_max_t> ret_val(ret_size);
		mpz_class max_dist{0};

		for (size_t i = 0 ; i < ret_size ; i++) {
			auto& [min_sl1, max_sl1] = min_max_val[i<<1];
			auto& [min_sl2, max_sl2] = min_max_val[(i<<1) + 1];
			auto minval = min(min_sl1, min_sl2);
			auto maxval = max(max_sl1, max_sl2);
			if ((maxval - minval) > max_dist)
					max_dist = maxval - minval;
			ret_val[i] = make_pair(minval, maxval);
		}
		return make_pair(ret_val, max_dist);
	}

	auto groupSlices(vector<mpz_class> const & values)
	{
		auto value_size = values.size();
		vector<min_max_t> ret(value_size);
		for (size_t i = 0 ; i < value_size ; ++i) {
			ret[i] = std::make_pair(values[i], values[i]);
		}
		return make_pair(ret, mpz_class{0});
	}

	auto find_best_subconfig(vector<min_max_t> const & min_max, int const wIn, int const wOut, int const split, int const wL, mpz_class const best_cost, table_cost_function_t const cost_model, Target* const target)
	{
		auto wHNoOverlap = wOut - wL;
		auto costFunction = [&](int wH)->mpz_class {
			auto cost_diff = cost_model(wIn, wL, target);
			auto cost_ss = cost_model(wIn - split, wH, target);
			return cost_diff + cost_ss;
		};
		auto bestWH = wHNoOverlap;
		auto estimate = costFunction(wHNoOverlap);
		bool overlapped = false;
		mpz_class lowbit_mask{1};
		lowbit_mask <<= wL;
		mpz_class overflow_detect = lowbit_mask + 1;
		for (auto const & [min, max] : min_max) {
			mpz_class min_low_bits{min & lowbit_mask};
			mpz_class delta{max - min};
			mpz_class deltaWithLowBits{delta + min_low_bits};
			// Assuption : increasing one size parameter, everything else constant, can only increase the cost
			while (deltaWithLowBits >= overflow_detect) {
				overlapped = true;
				bestWH += 1;
				estimate = costFunction(bestWH);
				if(estimate >= best_cost) {
					return make_tuple(false, overlapped, estimate, bestWH);
				}
				lowbit_mask >>= 1;
				min_low_bits = min & lowbit_mask;
				deltaWithLowBits = delta + min_low_bits;
			}
		}
		return make_tuple(true, overlapped, estimate, bestWH);
	}

	auto compressHighTable(vector<min_max_t> const & min_max, int const wH, int const wL)
	{
		mpz_class high_bits_mask{1};
		high_bits_mask <<= wH;
		high_bits_mask -= 1;
		high_bits_mask <<= wL;
		mpz_class or_acc{0};
		for (auto const &[min, max] : min_max) {
			auto high_bits = min & high_bits_mask;
			or_acc |= high_bits;
		}
		or_acc >>= wL;
		int zero_counter;
		for (zero_counter = 0 ; (zero_counter < wH) and ((or_acc & 1) != 0); zero_counter++) {
			or_acc >>= 1;
		}
		return wH - zero_counter;
	}

	auto buildCompressedTable(vector<mpz_class> const & values, int const wOut, int const s, int const wH, int const wL)
	{
		auto size = values.size();
		auto sssize = size >> s;
		auto shift = 1 << s;
		auto shift_h = wOut - wH;
		vector<mpz_class> diff_table(size);
		vector<mpz_class> subsamples_table(sssize);

		mpz_class H_mask{1}, L_mask{1};
		H_mask <<= wH;
		H_mask -= 1;
		H_mask <<= shift_h;

		L_mask <<= wL;
		for(size_t slice_idx = 0 ; slice_idx < sssize ; ++slice_idx) {
			size_t val_idx = slice_idx << s;
			mpz_class min{values[val_idx]};
			for (size_t cur_idx = val_idx + 1 ; cur_idx < val_idx + shift ; cur_idx++) {
				if (values[cur_idx] < min) {
					min = values[cur_idx];
				}
			}
			mpz_class high_bits = min & H_mask;
			mpz_class low_bits = min - high_bits;
			subsamples_table[slice_idx] = high_bits >> shift_h;
			mpz_class to_sub = min - low_bits;
			for (size_t cur_idx = val_idx ; cur_idx < val_idx + shift ; cur_idx++) {
				diff_table[cur_idx] = values[cur_idx] - to_sub;
				assert(diff_table[cur_idx] < L_mask);
			}
		}
		return make_pair(subsamples_table, diff_table);
	}


	DifferentialCompression DifferentialCompression::find_differential_compression(vector<mpz_class> const & values, int wIn, int wOut, Target * target, table_cost_function_t costModel)
	{
		auto [min_max, max_dist] = groupSlices(values);
		auto costFunction = [&](int wB, int wH, int wL)->mpz_class{
			auto costSubSampleSize = costModel(wB, wH, target);
			auto costDiffTable = costModel(wIn, wL, target);
			auto ret = costDiffTable + costSubSampleSize;
			return ret;
		};
		mpz_class original_cost = costModel(wIn, wOut, target);
		mpz_class best_cost = original_cost;
		int best_split = 0;
		int best_wh = wOut;
		int best_wl = 0;

		for (int s = 1 ; s < wIn ; s++) {
			std::tie(min_max, max_dist) = groupSlices(min_max);
			auto minWL = intlog2(max_dist);
			for (auto wL = minWL ; minWL < wOut - 1 ; ++minWL) {
				auto [interestingSol, overlapped, costBestSol, bestLocalWH] = find_best_subconfig(
						min_max,
						wIn,
						wOut,
						s,
						wL,
						best_cost,
						costModel,
						target);
				if (interestingSol) {
					if (!overlapped) {
						bestLocalWH = compressHighTable(min_max, bestLocalWH, wL);
						costBestSol = costFunction(wIn - s, bestLocalWH, wL);
					}
					if (costBestSol < best_cost) {
						best_split = s;
						best_wh = bestLocalWH;
						best_wl = wL;
						best_cost = costBestSol;
					}
				}
			}
		}

		auto [best_subsampling, best_diff] = buildCompressedTable(values, wOut, best_split, best_wh, best_wl);


		auto bestSS_cost = costModel(wIn-best_split, best_wh, target);
		auto bestDiffCost = costModel(wIn, best_wl, target);
		DifferentialCompression difcompress {best_subsampling, best_diff, wIn - best_split, best_wh, best_wl, wIn, wOut, original_cost, bestSS_cost, bestDiffCost};
		assert(difcompress.getInitialTable() == values);
		return difcompress;
	}

	DifferentialCompression DifferentialCompression::find_differential_compression(vector<mpz_class> const & values, int wIn, int wOut, Target * target)
	{
		table_cost_function_t cm = getGlobalCostModel();
		return find_differential_compression(values, wIn, wOut, target, cm);
	}

	vector<mpz_class> DifferentialCompression::getInitialTable() const {
		vector<mpz_class> reconstructedTable(1 << diffIndexSize);
		int stride = diffIndexSize - subsamplingIndexSize;

		for(int i = 0; i < 1 << diffIndexSize; i++){
					reconstructedTable[i] = (subsampling[i >> stride] << subsamplingShift()) + diffs[i];
				}
		return reconstructedTable;
	}

	size_t DifferentialCompression::subsamplingShift() const{
		return originalWout-subsamplingWordSize;
	}
	size_t DifferentialCompression::subsamplingStorageSize() const
	{
		return subsamplingWordSize << subsamplingIndexSize;
	}

	size_t DifferentialCompression::diffsStorageSize() const
	{
		return diffWordSize << diffIndexSize;
	}

	string DifferentialCompression::report() const
	{
 
		ostringstream t;
		t << "  Initial cost is:          " << originalWout << "x2^" << diffIndexSize << "=" << originalCost << endl;
		//t << "Initial estimated lut cost is :" << size_in_LUTs()<< endl;
		t << "  Best subsampling cost is: " << subsamplingWordSize <<	"x2^" << subsamplingIndexSize << "=" << subsamplingCost<< endl;
		//auto lutinputs = getTarget()->lutInputs()<< endl;
		//auto lutcost = [lutinputs](int diffIndexSize, int wOut)->int {
		//								 auto effdiffIndexSize = ((diffIndexSize - lutinputs) > 0) ? diffIndexSize - lutinputs : 0;
		//								 return wOut << effwIn;
		//							 };

		// auto subsamplingLUTCost = lutcost(subsamplingIndexSize, subsamplingWordSize);
		// t << "Best subsampling LUT cost:" << subsamplingLUTCost<< endl;
		// auto diffLutCost = lutcost(diffIndexSize, diffWordSize);
		t << "  Best diff cost is:        " << diffWordSize << "x2^" << diffIndexSize << "=" << diffCost<< endl;
		// t << "Best diff LUT cost: "<< diffLutCost<< endl;
		mpz_class compressedCost = diffCost + subsamplingCost;
		float ratio = compressedCost.get_d() / originalCost.get_d();
		ratio=roundf(ratio*100)/100; // 2 chiffres significatifs
		t << "  Total compressed cost is: " << compressedCost <<   "        compression ratio: " << ratio << " %";
		// t << "Total LUT cost: " << (diffLutCost + subsamplingLUTCost)<< endl;
#define LATEX_OUTPUT 1 // for the paper
#if LATEX_OUTPUT
		t  << endl << "Latex table line : " << endl;
		// first line
		t << " & " << originalCost << " & " <<  compressedCost << "  (" << ratio  << ") & "
			<<  subsamplingCost << " & " << diffCost << " & \\\\" <<  endl;
		// second line
		t << " & $" << originalWout << "\\cdot 2^{" << diffIndexSize << "}$ "
			<< "& & $" <<subsamplingWordSize << "\\cdot 2^{" << subsamplingIndexSize << "}$  &  $"
			<< diffWordSize << "\\cdot 2^{" << diffIndexSize << "}$  &   "
			<< subsamplingWordSize - (originalWout-diffWordSize) << " \\\\"<< endl;	
#endif
		return t.str();

	}

	string DifferentialCompression::newUniqueInstance(OperatorPtr op,
																										string actualInputName, string actualOutputName,
																										vector<mpz_class> values, string name,
																										int wIn, int wOut, int logicTable)
	{
		if (wIn==-1 || wOut==-1){
			ostringstream o; o << " DiffCompressedTable::newUniqueInstance needs explicit wIn and wOut" << endl;
			throw o.str();
		}
		// compress
		auto t = find_differential_compression(values, wIn, wOut, op->getTarget());
		string subsamplingOutName = actualOutputName+"_subsampling";
		string diffOutputName = actualOutputName+"_diff";
		string subsamplingIn = actualInputName+"_"+name+"_subsampling";
		// generate VHDL for subsampling table
		op->vhdl << tab << op->declare(subsamplingIn, t.subsamplingIndexSize) << " <= " << actualInputName << range(wIn-1, wIn-t.subsamplingIndexSize) << ";" << endl;
		Table::newUniqueInstance( op, subsamplingIn, subsamplingOutName,
															t.subsampling, name+"_subsampling",
															t.subsamplingIndexSize, t.subsamplingWordSize, logicTable );
		// generate VHDL for diff table
		Table::newUniqueInstance( op, actualInputName, diffOutputName,
															t.diffs, name+"_diff",
															wIn, t.diffWordSize );

		t.insertAdditionVHDL(op, actualOutputName, subsamplingOutName, diffOutputName);
		return t.report(); // don't know what Operator to return, hope it is harmless here
	}


	void DifferentialCompression::insertAdditionVHDL(OperatorPtr op,
																									 string actualOutputName, string subsamplingOutName,string diffOutputName)
	{
		int wIn = diffIndexSize;
		int wOut=originalWout;
		int nonOverlapMSBBits = wOut-diffWordSize;
		int overlapMiddleBits    = subsamplingWordSize - nonOverlapMSBBits;
		cerr << endl << "*****************" <<diffWordSize-overlapMiddleBits-1 << endl;
		// TODO an intadder when needed, but this is proably never useful
		op->vhdl << tab << op->declare(op->getTarget()->adderDelay(subsamplingWordSize),
																	 actualOutputName+"_topbits", subsamplingWordSize) << " <= " << subsamplingOutName
		<< " + (" << zg(nonOverlapMSBBits) << "& (" << diffOutputName << range(diffWordSize-1, diffWordSize-overlapMiddleBits) << "));" << endl;
		op->vhdl << tab << op->declare(actualOutputName, wOut) << " <= " << actualOutputName+"_topbits";
		if(diffWordSize-overlapMiddleBits-1 >=0 ) {
			op->vhdl << " & (" <<diffOutputName << range(diffWordSize-overlapMiddleBits-1,0) << ")";
		}
		op->vhdl << ";" << endl;
	}
}
