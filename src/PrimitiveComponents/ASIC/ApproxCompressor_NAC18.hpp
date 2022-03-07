#pragma once

#include "Operator.hpp"
#include "utils.hpp"
#include "BitHeap/Compressor.hpp"

namespace flopoco
{
    class ApproxCompressor_NAC18 : public Compressor
	{
	public:

		/**
		 * A basic constructor
		 */
		ApproxCompressor_NAC18(Operator *parentOp, Target * target, vector<int> heights);

		/** Factory method */
		static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
		/** Register the factory */
		static void registerFactory();

	public:
        void emulate(TestCase *tc);

	private:
	    void declareIO_Signals(void);
	};
/*
    class BasicAproxCompr : public BasicCompressor
	{
	public:
	    BasicAproxCompr(Operator* parentOp_, Target * target, vector<int> heights);

		virtual Compressor* getCompressor();


	};*/
}

