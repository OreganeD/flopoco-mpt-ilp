#include "DiffCompressedTable.hpp"
#include "Table.hpp"
#include <cassert>

namespace flopoco {

	DiffCompressedTable::DiffCompressedTable(OperatorPtr parentOp_, Target* target_, vector<mpz_class> _values, string _name, int _wIn, int _wOut, int _logicTable, int _minIn, int _maxIn) :
		Table(parentOp_, target_)
	{
		srcFileName = "DiffCompressedTable";
		setNameWithFreqAndUID(_name);
		setCopyrightString("Florent de Dinechin, Luc Forget, Maxime Christ (2020)");
		Table::init(_values, _name, _wIn, _wOut,  _logicTable,  _minIn,  _maxIn);

		diff_comp = DifferentialCompression::find_differential_compression(values, wIn, wOut, target_);
		// generate VHDL for subsampling table
		report_compression_gain();		
		string subsamplingIn = "X_subsampling";
		vhdl << tab << declare(subsamplingIn, diff_comp.subsamplingIndexSize) << " <= " << "X" << range(wIn-1, wIn-diff_comp.subsamplingIndexSize) << ";" << endl;
		string subsamplingOut = "Y_subsampling";
		string diffOut = "Y_diff";
		Table::newUniqueInstance(this, subsamplingIn, subsamplingOut,
														 diff_comp.subsampling, getName()+"_subsampling",
														 diff_comp.subsamplingIndexSize, diff_comp.subsamplingWordSize, _logicTable );
		// generate VHDL for diff table
		Table::newUniqueInstance(this, "X", diffOut,
														 diff_comp.diffs, getName()+"_diff",
														 wIn, diff_comp.diffWordSize,
														 _logicTable);

		diff_comp.insertAdditionVHDL(this, "fullOut", subsamplingOut, diffOut);
		vhdl << tab << "Y <= fullOut;" << endl;

	}

	OperatorPtr DiffCompressedTable::newUniqueInstance(OperatorPtr op,
																										 string actualInput, string actualOutput,
																										 vector<mpz_class> values, string name,
																										 int wIn, int wOut, int logicTable){
		op->schedule();
		op->inPortMap("X", actualInput);
		op->outPortMap("Y", actualOutput);
		DiffCompressedTable* t = new DiffCompressedTable(op, op->getTarget(), values, name, wIn, wOut,logicTable);
		op->vhdl << op->instance(t, name, false);
		return t;
	}

	void DiffCompressedTable::report_compression_gain()
	{
#if 0
		REPORT(INFO, "  Initial cost is:          " << diff_comp.originalWout << "x2^" << diff_comp.diffIndexSize << "=" << (diff_comp.originalWout << diff_comp.diffIndexSize));
		auto subsamplingCost = diff_comp.subsamplingWordSize << diff_comp.subsamplingIndexSize;
		REPORT(INFO, "  Best subsampling cost is: " << diff_comp.subsamplingWordSize <<   "x2^" << diff_comp.subsamplingIndexSize << "=" << subsamplingCost);
		auto diffCost = diff_comp.diffWordSize << diff_comp.diffIndexSize;
		REPORT(INFO, "  Best diff cost is:        " << diff_comp.diffWordSize << "x2^" << diff_comp.diffIndexSize << "=" << diffCost);
		REPORT(INFO, "  Overlap cost is  :        " << (diff_comp.subsamplingWordSize + diff_comp.diffWordSize - diff_comp.originalWout));
		REPORT(INFO, "  Total compressed cost is: " << (diffCost + subsamplingCost) << "/" << 100 * (1.0 -
			(float)(diffCost + subsamplingCost)/(diff_comp.originalWout << diff_comp.diffIndexSize)));
#else
		REPORT(INFO, endl << diff_comp.report());
#endif
		
		auto lutinputs = getTarget()->lutInputs();
		auto lutcost = [lutinputs](int wIn, int wOut)->int {
			auto effwIn = ((wIn - lutinputs) > 0) ? wIn - lutinputs : 0;
			return wOut << effwIn;
		};
		auto diffLutCost = lutcost(diff_comp.diffIndexSize, diff_comp.diffWordSize);
		auto subsamplingLutCost = lutcost(diff_comp.subsamplingIndexSize, diff_comp.subsamplingWordSize);
	}
}
