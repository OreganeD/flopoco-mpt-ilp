#ifndef DIFFERENTIAL_COMPRESSION_HPP
#define DIFFERENTIAL_COMPRESSION_HPP

#include "Operator.hpp"
#include "TableCostModel.hpp"

namespace flopoco {
	class DifferentialCompression {
	public:
		vector<mpz_class> subsampling;
		vector<mpz_class> diffs;
		int subsamplingIndexSize;
		int subsamplingWordSize;
		int diffWordSize;
		int diffIndexSize; // also originalWin
		int originalWout;
		mpz_class originalCost;
		mpz_class subsamplingCost;
		mpz_class diffCost;

		/**
		 * Find a non-destructive compression of a table as a sum of a subsampling + offset
		 * @param[in] values       The values of the table to compress
		 * @param[in] wIn          The initial index width of the table to compress
		 * @param[in] wOut         The output word size of the table to compress
		 * @param[in] costModel    The cost function which will be optimised by the method
		 * @param[in] target       The target for which the optimisation is performed
		 * @return                 A DifferentialCompression containing the values and the parameters of the compression for this table
		 */
		static DifferentialCompression find_differential_compression(vector<mpz_class> const & values, int wIn, int wOut, Target * target, table_cost_function_t cost);

		/**
		 * Call find_differential_compression with default cost model
		 * @param[in] values       The values of the table to compress
		 * @param[in] wIn          The initial index width of the table to compress
		 * @param[in] wOut         The output word size of the table to compress
		 * @param[in] target       The target for which the optimisation is performed
		 * @return                 A DifferentialCompression containing the values and the parameters of the compression for this table
		 */
		static DifferentialCompression find_differential_compression(vector<mpz_class> const & values, int wIn, int wOut, Target * target);


		/**
		 * @brief Uncompress the table
		 * @return a vector of mpz_class corresponding to the stored values
		 */
		vector<mpz_class> getInitialTable() const;

		/**
		 * @brief Compute the size in bits of the subsampling table content
		 * @return the size as a size_t
		 */
		size_t subsamplingStorageSize() const;

		/**
		 * @brief Compute the size in bits of the diffs table content
		 * @return the size as a size_t
		 */
		size_t diffsStorageSize() const;

		/**
		 * @brief Compute the left shift, in bits of the subsampling table content WRT the diff table content
		 */
		size_t subsamplingShift() const;

		/**  Computes the parameters of the addition, then inserts the corresponding VHDL in Operator op. */
		void insertAdditionVHDL(OperatorPtr op, string actualOutputName, string subsamplingOutName, string diffOutputName);


		string report() const;

		/** This is a drop-in  replacement for Table::newUniqueInstance.
			It will instantiate two tables and an adder, therefore will be suboptimal if the table output goes to a bit heap
		 * @param[in] op            The Operator that will be the parent of this Table (usually "this")
		 * @param[in] actualInput   The actual input name
		 * @param[in] actualOutput  The actual input name
		 * @param[in] values        The vector of mpz_class values to be passed to the Table constructor
		 returns the report
		 */
		static string newUniqueInstance(OperatorPtr op,
																		string actualInput, string actualOutput,
																		vector<mpz_class> values, string name,
																		int wIn = -1, int wOut = -1,
																		int logicTable=0);
	};
}
#endif
