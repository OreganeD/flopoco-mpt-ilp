#ifndef DIFF_COMPRESSED_TABLE_HPP
#define DIFF_COMPRESSED_TABLE_HPP

#include "Table.hpp"
#include "DifferentialCompression.hpp"


namespace flopoco {
	/** This class is a drop-in replacement for Table in the cases when Hsiao's differential compression can be used.
			It is an Operator that includes the two compressed tables and adds their output.
			However it should rarely be used directly. In most case, the two tables can be more efficiently added in some pre-existing bit heap.
	*/
    class DiffCompressedTable  : public Table {
    public:
		/**
		 * The DiffCompressedTable constructor.
		 It is an exception in FloPoCo as there is no corresponding user interface for it, because passing the vector of content on the command line would be a pain.
		 The proper way to instanciate this component is with newUniqueInstance()
		 * @param[in] parentOp 	the parent operator in the component hierarchy
		 * @param[in] target 		the target device
		 * @param[in] values 		the values used to fill the table. Each value is a bit vector given as positive mpz_class.
		 * @param[in] name      a new name for the VHDL entity
		 * @param[in] wIn    		the with of the input in bits (optional, may be deduced from values)
		 * @param[in] wOut   		the with of the output in bits  (optional, may be deduced from values)
		 * @param[in] logicTable 1 if the table is intended to be implemented as logic; It is then shared
							-1 if it is intended to be implemented as embedded RAM;
								 0 (default): let the constructor decide, depending on the size and target
		 * @param[in] minIn			minimal input value, to which value[0] will be mapped (default 0)
		 * @param[in] maxIn			maximal input value (default: values.size()-1)
		 */
		DiffCompressedTable(OperatorPtr parentOp, Target* target, vector<mpz_class> _values, string name="",
					int _wIn = -1, int _wOut = -1, int _logicTable = 0, int _minIn = -1, int _maxIn = -1);



		/** Drop-in replacement for Table::newUniqueInstance
		 * @param[in] op            The Operator that will be the parent of this Table (usually "this")
		 * @param[in] actualInput   The actual input name
		 * @param[in] actualOutput  The actual input name
		 * @param[in] values        The vector of mpz_class values to be passed to the Table constructor
		 */
		static OperatorPtr newUniqueInstance(OperatorPtr op,
																				 string actualInput, string actualOutput,
																				 vector<mpz_class> values, string name,
																				 int wIn = -1, int wOut = -1,
																				 int logicTable=0);

		void report_compression_gain();


		DifferentialCompression diff_comp;
	};
}
#endif
