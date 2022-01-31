#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TableTest

#include <iostream>
#include <vector>

#include <gmpxx.h>
#include <boost/test/unit_test.hpp>

#include "Tables/DifferentialCompression.hpp"
#include "Targets/Kintex7.hpp"

using std::vector;

BOOST_AUTO_TEST_CASE(TestDifferentialCompressionLarge)
{
	constexpr size_t TABLE_WIN = 8;
	constexpr size_t TABLE_SIZE = 1 << TABLE_WIN;
	constexpr size_t TABLE_WOUT = 13;
	constexpr size_t TABLE_FOLD_WIDTH = 3;
	constexpr size_t FOLD_SIZE = 1 << TABLE_FOLD_WIDTH;
	vector<mpz_class> val(TABLE_SIZE);
	mpz_class offset{1 << (TABLE_WOUT - 1)};

	for (size_t i = 0 ; i < TABLE_SIZE ; i += FOLD_SIZE) {
		for (size_t j = 0 ; j < FOLD_SIZE ; j++) {
			val[i+j] = offset + j;
		}
	}

	BOOST_TEST_CHECKPOINT("Calling find_differential_compression");
	Kintex7 target{};
	auto diff_compress = DifferentialCompression::find_differential_compression(val, TABLE_WIN, TABLE_WOUT, &target);

	BOOST_TEST_CHECKPOINT("find_differential_compression returned");
	auto reconstructedTable = diff_compress.getInitialTable();
	for (size_t i = 0 ; i < (1 << (TABLE_WIN)) ; ++i) {
		BOOST_REQUIRE_MESSAGE(reconstructedTable[i] == val[i],
		"Error with reconstitution of table value " <<i << ": got " << reconstructedTable[i] << " instead of " << val[i]);
	}
}

BOOST_AUTO_TEST_CASE(TestDifferentialCompressionTrivial)
{
	constexpr size_t HIGH_IDX_WIDTH = 8;
	constexpr size_t LOW_IDX_WIDTH = 3;
	constexpr size_t WIN = HIGH_IDX_WIDTH + LOW_IDX_WIDTH;
	constexpr size_t LOW_ID_MASK = (size_t{1} << LOW_IDX_WIDTH) - 1;
	constexpr size_t TOTAL_SIZE = size_t{1} << WIN;
	constexpr size_t SHIFT = LOW_IDX_WIDTH + 2;
	constexpr size_t WOUT = SHIFT + HIGH_IDX_WIDTH;

	vector<mpz_class> table(TOTAL_SIZE);
	for (size_t i = 0 ; i < TOTAL_SIZE ; ++i) {
		size_t low_id = i & LOW_ID_MASK;
		size_t high_id = (i >> LOW_IDX_WIDTH) << SHIFT;
		mpz_class val{high_id};
		val |= low_id;
		table[i] = val;
	}

	BOOST_TEST_CHECKPOINT("Calling find_differential_compression");
	Kintex7 target{};
	auto diff_compress = DifferentialCompression::find_differential_compression(
				table,
				WIN,
				WOUT,
				&target
		);

	BOOST_REQUIRE_MESSAGE(diff_compress.subsamplingIndexSize == HIGH_IDX_WIDTH, "Unexpected diff index size");
	BOOST_REQUIRE_MESSAGE(diff_compress.diffWordSize == LOW_IDX_WIDTH, "Unexpected diff word size");
	BOOST_REQUIRE_MESSAGE(diff_compress.subsamplingWordSize == HIGH_IDX_WIDTH + SHIFT - LOW_IDX_WIDTH, "Unexpected subsamble word size");
	auto overlapp = diff_compress.diffWordSize + diff_compress.subsamplingWordSize - WOUT;
	BOOST_REQUIRE_MESSAGE(overlapp == 0, "Result compression overlapped while it is not necessary");
}
