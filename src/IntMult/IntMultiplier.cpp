/*
  An integer multiplier for FloPoCo

  Authors:  Martin Kumm with parts of previous IntMultiplier of Bogdan Pasca, F de Dinechin, Matei Istoan, Kinga Illyes and Bogdan Popa spent

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2017.
  All rights reserved.
*/

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAddSubCmp/IntAdder.hpp"
#include "BaseMultiplierCollection.hpp"

#include "TilingStrategyOptimalILP.hpp"
#include "TilingStrategyBasicTiling.hpp"
#include "TilingStrategyGreedy.hpp"
#include "TilingStrategyXGreedy.hpp"
#include "TilingStrategyBeamSearch.hpp"
#include "TilingAndCompressionOptILP.hpp"

using namespace std;

namespace flopoco {


    IntMultiplier::IntMultiplier (Operator *parentOp, Target* target_, int wX_, int wY_, int wOut_, bool signedIO_, float dspOccupationThreshold, int maxDSP, bool superTiles, bool use2xk, bool useirregular, bool useLUT, bool useDSP, bool useKaratsuba, int beamRange, bool optiTrunc, bool minStages, bool squarer):
		Operator ( parentOp, target_ ),wX(wX_), wY(wY_), wOut(wOut_),signedIO(signedIO_), dspOccupationThreshold(dspOccupationThreshold), squarer(squarer) {
        srcFileName = "IntMultiplier";
        setCopyrightString("Martin Kumm, Florent de Dinechin, Kinga Illyes, Bogdan Popa, Bogdan Pasca, 2012");

        ostringstream name, trunc_info;
        name << "IntMultiplier";
        setNameWithFreqAndUID(name.str());

        // the addition operators need the ieee_std_signed/unsigned libraries
        useNumericStd();

        multiplierUid = parentOp->getNewUId();
        wFullP = prodsize(wX, wY, signedIO_, signedIO_);

        if (wOut == 0 || wFullP < wOut)
            wOut = wFullP;

        unsigned int guardBits = 0, keepBits = 0;
        mpz_class errorBudget = 0, centerErrConstant = 0;
        if(wOut < wFullP){  //check if multiplier is truncated
            if(signedIO_){
                computeTruncMultParamsMPZ(wFullP, wOut, guardBits, keepBits, errorBudget, centerErrConstant);
            }else{
                computeTruncMultParamsMPZunsigned(wFullP, wOut, guardBits, keepBits, errorBudget, centerErrConstant);
            }
            trunc_info << " faithfully rounded to wOut=" << wOut << " bits. Will use " << guardBits << " guard and " << keepBits <<
                        " keep bits. The error budget is " << errorBudget << " and the error re-centering constant " << centerErrConstant;
        }

        REPORT(INFO, "IntMultiplier(): Constructing a multiplier of size " <<
        wX << "x" << wY << ((wOut < wFullP)?trunc_info.str():"") << ", using a DSP threshold of " << dspOccupationThreshold << ".")

		string xname="X";
		string yname="Y";

		// Set up the IO signals
		addInput(xname, wX, true);
		addInput(yname, wY, true);
		addOutput("R", wOut, 2, true);

		// The larger of the two inputs
		vhdl << tab << declare(addUID("XX"), wX, true) << " <= " << xname << " ;" << endl;
		vhdl << tab << declare(addUID("YY"), wY, true) << " <= " << yname << " ;" << endl;

		if(target_->plainVHDL()) {
			vhdl << tab << declareFixPoint("XX",signedIO,-1, -wX) << " <= " << (signedIO?"signed":"unsigned") << "(" << xname <<");" << endl;
			vhdl << tab << declareFixPoint("YY",signedIO,-1, -wY) << " <= " << (signedIO?"signed":"unsigned") << "(" << yname <<");" << endl;
			vhdl << tab << declareFixPoint("RR",signedIO,-1, -wX-wY) << " <= XX*YY;" << endl;
			vhdl << tab << "R <= std_logic_vector(RR" << range(wX+wY-1, wX+wY-wOut) << ");" << endl;

			return;
		}

        BitHeap bitHeap(this, wFullP);
		//BitHeap bitHeap(this, wOut + guardBits);
        unsigned bitHeapLSBWeight = 0;

		REPORT(INFO, "Creating BaseMultiplierCollection")
		BaseMultiplierCollection baseMultiplierCollection(getTarget());
//		baseMultiplierCollection.print();

		MultiplierTileCollection multiplierTileCollection(getTarget(), &baseMultiplierCollection, wX, wY, superTiles, use2xk, useirregular, useLUT, useDSP, useKaratsuba, squarer);

		string tilingMethod = getTarget()->getTilingMethod();

		REPORT(INFO, "Creating TilingStrategy using tiling method " << tilingMethod)

		TilingStrategy* tilingStrategy;
		if(tilingMethod.compare("heuristicBasicTiling") == 0) {
			tilingStrategy = new TilingStrategyBasicTiling(
					wX,
					wY,
					wOut + guardBits,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
                    ((maxDSP<0)?(unsigned)INT_MAX:(unsigned)maxDSP)
			);
		}
		else if(tilingMethod.compare("heuristicGreedyTiling") == 0) {
			tilingStrategy = new TilingStrategyGreedy(
					wX,
					wY,
					wOut,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
					maxDSP,
					useirregular,
					use2xk,
					superTiles,
					useKaratsuba,
					multiplierTileCollection,
                    guardBits,
                    keepBits
			);
		}
		else if(tilingMethod.compare("heuristicXGreedyTiling") == 0) {
			tilingStrategy = new TilingStrategyXGreedy(
					wX,
					wY,
					wOut,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
					maxDSP,
					useirregular,
					use2xk,
					superTiles,
					useKaratsuba,
					multiplierTileCollection,
                    guardBits,
                    keepBits
			);
		}
		else if(tilingMethod.compare("heuristicBeamSearchTiling") == 0) {
			tilingStrategy = new TilingStrategyBeamSearch(
					wX,
					wY,
					wOut,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
					maxDSP,
					useirregular,
					use2xk,
					superTiles,
					useKaratsuba,
					multiplierTileCollection,
					beamRange,
                    guardBits,
                    keepBits
			);
		} else if(tilingMethod.compare("optimal") == 0){
			tilingStrategy = new TilingStrategyOptimalILP(
					wX,
					wY,
					wOut,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
					maxDSP,
					multiplierTileCollection,
                    guardBits,
                    keepBits,
                    errorBudget,
                    centerErrConstant,
                    optiTrunc,
                    squarer
			);

		}  else if(tilingMethod.compare("optimalTilingAndCompression") == 0){
			tilingStrategy = new TilingAndCompressionOptILP(
					wX,
					wY,
					wOut,
					signedIO,
					&baseMultiplierCollection,
					baseMultiplierCollection.getPreferedMultiplier(),
					dspOccupationThreshold,
					maxDSP,
					multiplierTileCollection,
					&bitHeap,
                    guardBits,
                    keepBits,
                    errorBudget,
                    centerErrConstant,
                    optiTrunc,
                    minStages
			);

		} else {
			THROWERROR("Tiling strategy " << tilingMethod << " unknown");
		}

		REPORT(DEBUG, "Solving tiling problem")
		tilingStrategy->solve();

		list<TilingStrategy::mult_tile_t> &solution = tilingStrategy->getSolution();
		if(signedIO)
            for(auto & tile : solution) {
                //resize DSPs to be aligned with left and bottom border of the tiled area to allow the correct handling of the sign
                tile.first = tile.first.shrinkFitDSP(tile.second.first,tile.second.second, wX, wY);
                //Set signedness of individual tiles according to their position
                tile.first = tile.first.setSignStatus(tile.second.first,tile.second.second, wX, wY, signedIO);
            }
        tilingStrategy->printSolution();
		auto solLen = solution.size();
		REPORT(DETAILED, "Found solution has " << solLen << " tiles")
		if (target_->generateFigures())
            createFigures(tilingStrategy);

		schedule();

       	if (guardBits > 0) {
            //Check truncated solution
            mpz_class actualTruncError = checkTruncationError(solution, guardBits, errorBudget, centerErrConstant, wX, wY, signedIO);
            cout << "calc min req weight is=" << prodsize(wX, wY, signedIO, signedIO) - (wOut + guardBits) << endl;
            bitHeapLSBWeight = (dynamic_cast<CompressionStrategy*>(tilingStrategy))?0:calcBitHeapLSB(solution, guardBits, errorBudget, centerErrConstant, actualTruncError);
            guardBits = wFullP - wOut - bitHeapLSBWeight; //To select result bits, because the dynamic ilp does not consider guardBits

		    //this is the rounding bit for a faithfully rounded truncated multiplier
			bitHeap.addConstantOneBit(static_cast<int>(guardBits) - 1);
			//these are the constant bits to recenter the average error around 0 and allow for more truncation error
            mpz_class colweight, bitstate;
            int i = wFullP - wOut - guardBits ;
            do{
                mpz_pow_ui(colweight.get_mpz_t(), mpz_class(2).get_mpz_t(), i);
                mpz_and(bitstate.get_mpz_t(), colweight.get_mpz_t(), centerErrConstant.get_mpz_t());
                if (bitstate) {
                    bitHeap.addConstantOneBit(i - (wFullP - wOut - guardBits));
                    REPORT(DEBUG,  "Adding constant bit with weight=" << i << " BitHeap col=" << i - (wFullP - wOut - guardBits) << "to recenter the truncation error at 0");
                    //cout << "height at pos " << i - (wFullP - wOut - guardBits) << ": " << bitHeap.getColumnHeight( i - (wFullP - wOut - guardBits)) << endl;
                }
                i++;
            } while(colweight <= centerErrConstant);
		}

        branchToBitheap(&bitHeap, solution, bitHeapLSBWeight);

		if (dynamic_cast<CompressionStrategy*>(tilingStrategy)) {
		    REPORT(DEBUG,  "Class is derived from CompressionStrategy, passing result for compressor tree.");
			bitHeap.startCompression(dynamic_cast<CompressionStrategy*>(tilingStrategy));
		} else {
			bitHeap.startCompression();
		}

		vhdl << tab << "R" << " <= " << bitHeap.getSumName() << range(wOut-1 + guardBits, guardBits) << ";" << endl;

		delete tilingStrategy;
	}

	unsigned int IntMultiplier::computeGuardBits(unsigned int wX, unsigned int wY, unsigned int wOut)
	{
		unsigned int minW = (wX < wY) ? wX : wY;
		unsigned int maxW = (wX < wY) ? wY : wX;

		auto ps = prodsize(wX, wY, signedIO, signedIO);
		auto nbDontCare = ps - wOut;

		mpz_class errorBudget{1};
		errorBudget <<= (nbDontCare >= 1) ? nbDontCare - 1 : 0;
		errorBudget -= 1;

		REPORT(DEBUG, "computeGuardBits: error budget is " << errorBudget.get_str())

		unsigned int nbUnneeded;
		for (nbUnneeded = 1 ; nbUnneeded <= nbDontCare ; nbUnneeded += 1) {
			unsigned int bitAtCurCol;
			if (nbUnneeded < minW) {
				bitAtCurCol = nbUnneeded;
			} else if (nbUnneeded <= maxW) {
				bitAtCurCol = minW;
			} else {
				bitAtCurCol = ps -nbUnneeded;
			}
			REPORT(DETAILED, "computeGuardBits: Nb bit in column " << nbUnneeded << " : " << bitAtCurCol)
			mpz_class currbitErrorAmount{bitAtCurCol};
			currbitErrorAmount <<= (nbUnneeded - 1);

			REPORT(DETAILED, "computeGuardBits: Local error for column " << nbUnneeded << " : " << currbitErrorAmount.get_str())

			errorBudget -= currbitErrorAmount;
			REPORT(DETAILED, "computeGuardBits: New error budget: " << errorBudget.get_str())
			if(errorBudget < 0) {
				break;
			}
		}
		nbUnneeded -= 1;

		return nbDontCare - nbUnneeded;
	}

    void IntMultiplier::computeTruncMultParamsMPZ(unsigned wFull, unsigned wOut, unsigned &g, unsigned &k, mpz_class &errorBudget, mpz_class &constant) const{
        unsigned l_P = wFull - wOut, l_ext = 0, t = 0;
        mpz_class colweight = 2, deltan = 0, deltap = 0, wlext = 1, wlextpe = 2, tbits = 0;
        mpz_pow_ui(errorBudget.get_mpz_t(), mpz_class(2).get_mpz_t(), l_P-1); //tiling error budget
        if(l_P == 0) return;
        mpz_class delta_n_new = deltan + additionalError_n(wX, wY, l_ext, t + 1, wFull, signedIO) * wlext;
        mpz_class delta_p_new = deltap + additionalError_p(wX, wY, l_ext, t + 1, wFull, signedIO) * wlext;
        mpz_class constant_new = calcErcConst(errorBudget, wlext, delta_p_new);
        //Try to remove whole diagonals without violating the error bound.
        while(delta_n_new - constant_new < errorBudget && delta_p_new + constant_new < errorBudget ){
            t++;
            deltan = delta_n_new;
            deltap = delta_p_new;
            constant = constant_new;
            //printf("l_ext=%2i, t=%2i, deltap=%lu, deltan=%lu, wlext=%lu, wlextpe=%lu, C=%lu\n", l_ext, t, deltap.get_ui(), deltan.get_ui(), wlext.get_ui(), wlextpe.get_ui(), constant.get_ui());
            if(widthOfDiagonalOfRect(wX, wY, l_ext+1, wFull) <= t){
                t = 0, l_ext++;
                wlext = wlextpe;
                mpz_pow_ui(wlextpe.get_mpz_t(), mpz_class(2).get_mpz_t(), l_ext+1);
            }
            delta_n_new = deltan + additionalError_n(wX, wY, l_ext, t + 1, wFull, signedIO) * wlext;
            delta_p_new = deltap + additionalError_p(wX, wY, l_ext, t + 1, wFull, signedIO) * wlext;
            constant_new = calcErcConst(errorBudget, wlext, delta_p_new, constant);
        }

        //cout << "posTruncError=" << deltap << " negTruncError=" << deltan << endl;

        g = l_P - l_ext;
        k = widthOfDiagonalOfRect(wX,wY,l_ext+1,wFull) - t;
        //printf("w=%2i, l_ext=%i, t=%i, g=%i, k=%i, ", wX, l_ext, t, g, k);
        //cout << "errorBudget=" << errorBudget << ", C=" << constant << endl;
    }

    mpz_class IntMultiplier::calcErcConst(mpz_class &errorBudget, mpz_class &wlext, mpz_class &deltap, mpz_class constant){
        if(constant == 0) return 0;
        constant = errorBudget - wlext;                                         //2^(l_P-1) - 2^(l_ext+1)
        mpz_class cMaxLim = -deltap + errorBudget;
        //cout << "C=" << constant << " cMaxLim=" << cMaxLim << endl;
        while(cMaxLim <= constant) constant -= wlext;
        return constant;
    }

    unsigned IntMultiplier::additionalError_n(unsigned wX, unsigned wY, unsigned col, unsigned t, unsigned wFull, bool signedIO){
        unsigned nvals = negValsInDiagonalOfRect(wX, wY, col + 1, wFull, signedIO);
        unsigned diagl = widthOfDiagonalOfRect(wX, wY, col + 1, wFull);
        if( !(nvals && ( (t == 1 && wX - 1 <= col) || (t == diagl && wY - 1 <= col)) ) && !(nvals == 2 && t == diagl) ) return 1;
        return 0;
    }

    unsigned IntMultiplier::additionalError_p(unsigned wX, unsigned wY, unsigned col, unsigned t, unsigned wFull, bool signedIO){
        unsigned nvals = negValsInDiagonalOfRect(wX, wY, col + 1, wFull, signedIO);
        unsigned diagl = widthOfDiagonalOfRect(wX, wY, col + 1, wFull);
        //cout << "nvals=" << nvals << " diag=" << diagl << " " << (nvals && ( t == 1 && wX - 1 <= col || t == diagl && wY - 1 <= col) ) << endl;
        if( nvals && ( (t == 1 && wX - 1 <= col) || (t == diagl && wY - 1 <= col)) ) return 1;
        if( nvals == 2 && t == diagl) return 1;
        return 0;
    }

    void IntMultiplier::computeTruncMultParamsMPZunsigned(unsigned wFull, unsigned wOut, unsigned &g, unsigned &k, mpz_class &errorBudget, mpz_class &constant) const{
        unsigned l_P = wFull - wOut, l_ext = 0, t = 0;
        mpz_class colweight = 2, dlow = 0, wlext = 1, wlextpe = 2, wlp;
        mpz_pow_ui(errorBudget.get_mpz_t(), mpz_class(2).get_mpz_t(), l_P-1); //tiling error budget
        mpz_pow_ui(wlp.get_mpz_t(), mpz_class(2).get_mpz_t(), l_P); //2^l_P
        if(l_P == 0) return;
        //Try to remove whole diagonals without violating the error bound.
        while( (t+1)*wlextpe + (dlow + widthOfDiagonalOfRect(wX,wY,l_ext+1,wFull)*wlext ) < wlp ){
            dlow += widthOfDiagonalOfRect(wX,wY,l_ext+1,wFull) * wlext;
            l_ext++;
            wlext = wlextpe;
            mpz_pow_ui(wlextpe.get_mpz_t(), mpz_class(2).get_mpz_t(), l_ext+1);
            //printf("l_ext=%2i, dlow=%i, wlext=%i, wlextpe=%i, C=%i\n", l_ext, dlow.get_ui(), wlext.get_ui(), wlextpe.get_ui(), constant.get_ui());
        }

        //printf("l_ext=%2i, dlow=%i, wlext=%i, wlextpe=%i, C=%i, \n ", l_ext, dlow.get_ui(), wlext.get_ui(), wlextpe.get_ui(), constant.get_ui());

        //Try to remove partial products from the next diagonal, until it would violate the error bound.
        while( (t+2)*wlext + dlow < wlp && t < widthOfDiagonalOfRect(wX,wY,l_ext+1,wFull)){
            t += 1;
        }

        constant = errorBudget - wlext;                                         //2^(l_P-1) - 2^(l_ext+1)
        g = l_P - l_ext;
        k = widthOfDiagonalOfRect(wX,wY,l_ext+1,wFull) - t;
        //printf("w=%2i, l_ext=%i, t=%i, g=%i, k=%i, ", wX, l_ext, t, g, k);
        //cout << "errorBudget=" << errorBudget << ", C=" << constant << endl;
    }

    unsigned int IntMultiplier::widthOfDiagonalOfRect(unsigned wX, unsigned wY, unsigned col, unsigned wFull, bool signedIO){
        if(signedIO){ wX--; wY--; wFull = prodsize(wX, wY, signedIO, signedIO);}
        if(0 <= col && (int)col <= min((int)wX, (int)wY)){
            return col;
        } else if(min((int)wX,(int)wY) < (int)col && (int)col <= max((int)wX, (int)wY)){
            return min((int)wX,(int)wY);
        } else {
            return wFull-col;
        }
    }

    unsigned int IntMultiplier::negValsInDiagonalOfRect(unsigned wX, unsigned wY, unsigned col, unsigned wFull, bool signedIO){
        return widthOfDiagonalOfRect(wX,wY,col ,wFull) - widthOfDiagonalOfRect(wX,wY,col ,wFull, signedIO);
    }

    /**
     * @brief Compute the size required to store the untruncated product of inputs of a given width
     * @param wX size of the first input
     * @param wY size of the second input
     * @return the number of bits needed to store a product of I<wX> * I<WY>
     */
    unsigned int IntMultiplier::prodsize(unsigned int wX, unsigned int wY, bool signedX, bool signedY)
    {
        if(wX == 0 || wY == 0)
            return 0;

        if (wX == 1 && wY == 1) {
            return 1;
        }

        if(wX == 1 && !signedX)
            return wY;

        if(wY == 1 && !signedY)
            return wX;

        return wX + wY;
    }

	/**
	 * @brief getZeroMSB if the tile goes out of the multiplier,
	 *		  compute the number of extra zeros that should be inputed to the submultiplier as MSB
	 * @param anchor lsb of the tile
	 * @param tileWidth width of the tile
	 * @param multiplierLimit msb of the overall multiplier
	 * @return
	 */
	inline unsigned int getZeroMSB(int anchor, unsigned int tileWidth, int multiplierLimit)
	{
		unsigned int result = 0;
		if (static_cast<int>(tileWidth) + anchor > multiplierLimit) {
			result = static_cast<unsigned int>(
						static_cast<int>(tileWidth) + anchor - multiplierLimit
						);
		}
		return result;
	}

	/**
	 * @brief getInputSignal Perform the selection of the useful bit and pad if necessary the input signal to fit a certain format
	 * @param msbZeros number of zeros to add left of the selection
	 * @param selectSize number of bits to get from the inputSignal
	 * @param offset where to start taking bits from the input signal
	 * @param lsbZeros number of zeros to append to the selection for right padding
	 * @param signalName input signal name from which to select the bits
	 * @return the string corresponding to the signal selection and padding
	 */
	inline string getInputSignal(unsigned int msbZeros, unsigned int selectSize, unsigned int offset, unsigned int lsbZeros, string const & signalName)
	{
		stringstream s;
		if (msbZeros > 0)
			s << zg(static_cast<int>(msbZeros)) << " & ";

		s << signalName << range(
				 static_cast<int>(selectSize - 1 + offset),
				 static_cast<int>(offset)
			);

		if (lsbZeros > 0)
			s << " & " << zg(static_cast<int>(lsbZeros));

		return s.str();
	}

	void IntMultiplier::branchToBitheap(BitHeap* bitheap, list<TilingStrategy::mult_tile_t> &solution, unsigned int bitheapLSBWeight)
	{
		size_t i = 0;
		stringstream oname, ofname;
		for(auto & tile : solution) {
			auto& parameters = tile.first;
			auto& anchor = tile.second;

			int xPos = anchor.first;
			int yPos = anchor.second;

			int LSBWeight = xPos + yPos + parameters.getRelativeResultLSBWeight();
			unsigned int outLSBWeight = (LSBWeight < 0)?0:static_cast<unsigned int>(LSBWeight);                         // calc result LSB weight corresponding to tile position
			unsigned int truncated = (outLSBWeight < bitheapLSBWeight) ? bitheapLSBWeight - outLSBWeight : 0;           // calc result LSBs to be ignored
			unsigned int bitHeapOffset = (outLSBWeight < bitheapLSBWeight) ? 0 : outLSBWeight - bitheapLSBWeight;       // calc bits between the tiles output LSB and the bitheaps LSB

			//unsigned int toSkip = lsbZerosXIn + lsbZerosYIn + truncated;                                                // calc LSB bits to be ignored in the tiles output
			unsigned int toSkip = ((LSBWeight < 0) ? static_cast<unsigned int>(-LSBWeight) : 0) + truncated;            // calc LSB bits to be ignored in the tiles output
			unsigned int tokeep = parameters.getRelativeResultMSBWeight() - toSkip - parameters.getRelativeResultLSBWeight()+1;                                     // the tiles MSBs that are actually used
			assert(tokeep > 0); //A tiling should not give a useless tile

			oname.str("");
			oname << "tile_" << i << "_output";
			realiseTile(tile, i, oname.str());

			ofname.str("");
			ofname << "tile_" << i << "_filtered_output";

			if(parameters.getOutputWeights().size()){
			    //The multiplier tile has more then one output signal
				for(unsigned i = 0; i < parameters.getOutputWeights().size(); i++){
					if(i){
						vhdl << declare(.0, ofname.str() + to_string(i), 41) << " <= " << "" << oname.str() + to_string(i) << "(40 downto 0)" << ";" << endl;
						getSignalByName(ofname.str() + to_string(i))->setIsSigned();
						bitheap->addSignal(ofname.str() + to_string(i), bitHeapOffset+parameters.getOutputWeights()[i] + ((squarer && tile.first.isSquarer())?0:1));
					} else {
						vhdl << declare(.0, ofname.str(), 41) << " <= " << "" << oname.str() << "(40 downto 0)" << ";" << endl;
						bitheap->addSignal(ofname.str(), bitHeapOffset+parameters.getOutputWeights()[i] + ((squarer && tile.first.isSquarer())?0:1));
					}
					cout << "output (" << i+1 << "/" << parameters.getOutputWeights().size() << "): " << ofname.str() + to_string(i) << " shift " << bitHeapOffset+parameters.getOutputWeights()[i] << endl;
				}
			} else {
                //The multiplier tile has only a single output signal
				unsigned int xInputLength = parameters.getTileXWordSize();
				unsigned int yInputLength = parameters.getTileYWordSize();
				bool xIsSigned = parameters.isSignedMultX();
				bool yIsSigned = parameters.isSignedMultY();

				if(tile.first.isSquarer()){xIsSigned=false; yIsSigned=false;}   //result of squarer tile is always unsigned

				bool bothOne = (xInputLength == 1) && (yInputLength == 1);
				bool signedCase = (bothOne and (xIsSigned != yIsSigned)) or ((not bothOne) and (xIsSigned or yIsSigned));

				vhdl << tab << declareFixPoint(.0, ofname.str(), signedCase, tokeep-1, 0) << " <= " << ((signedCase) ? "" : "un") << "signed(" << oname.str() <<
						range(toSkip + tokeep - 1, toSkip) << ");" << endl;

				//squarers can have tile counted twice to exploit the symmetries, and hence might require a left shift by one bit or have tiles to be considered negative to compensate for overlap.
				if(tile.first.getTilingWeight() == 1 || tile.first.getTilingWeight() == 2){
				    bitheap->addSignal(ofname.str(), bitHeapOffset + ((tile.first.getTilingWeight() == 2)?1:0) );
				} else {
				    bitheap->subtractSignal(ofname.str(), bitHeapOffset + ((tile.first.getTilingWeight() == -2)?1:0) );
				}
			}
			i += 1;
		}
	}

	Operator* IntMultiplier::realiseTile(TilingStrategy::mult_tile_t & tile, size_t idx, string output_name)
	{
		auto& parameters = tile.first;
		auto& anchor = tile.second;
		int xPos = anchor.first;
		int yPos = anchor.second;

		unsigned int xInputLength = parameters.getTileXWordSize();
		unsigned int yInputLength = parameters.getTileYWordSize();
		//unsigned int outputLength = parameters.getOutWordSize();

		unsigned int lsbZerosXIn = (xPos < 0) ? static_cast<unsigned int>(-xPos) : 0;
		unsigned int lsbZerosYIn = (yPos < 0) ? static_cast<unsigned int>(-yPos) : 0;

		unsigned int msbZerosXIN = getZeroMSB(xPos, xInputLength, wX);
		unsigned int msbZerosYIn = getZeroMSB(yPos, yInputLength, wY);

		unsigned int selectSizeX = xInputLength - (msbZerosXIN + lsbZerosXIn);
		unsigned int selectSizeY = yInputLength - (msbZerosYIn + lsbZerosYIn);

		unsigned int effectiveAnchorX = (xPos < 0) ? 0 : static_cast<unsigned int>(xPos);
		unsigned int effectiveAnchorY = (yPos < 0) ? 0 : static_cast<unsigned int>(yPos);

		auto tileXSig = getInputSignal(msbZerosXIN, selectSizeX, effectiveAnchorX, lsbZerosXIn, "X");
		auto tileYSig = getInputSignal(msbZerosYIn, selectSizeY, effectiveAnchorY, lsbZerosYIn, ((squarer && !tile.first.isSquarer()) ?"X":"Y"));

		stringstream nameX, nameY, nameOutput;
		nameX << "tile_" << idx << "_X";
		nameY << "tile_" << idx << "_Y";

		nameOutput << "tile_" << idx << "_Out";

		vhdl << tab << declare(0., nameX.str(), xInputLength) << " <= " << tileXSig << ";" << endl;
		if(!tile.first.isSquarer()) vhdl << tab << declare(0., nameY.str(), yInputLength) << " <= " << tileYSig << ";" << endl;

		string multIn1SigName = nameX.str();
		string multIn2SigName = nameY.str();

		if (parameters.isFlippedXY())
			std::swap(multIn1SigName, multIn2SigName);

		nameOutput.str("");
		nameOutput << "tile_" << idx << "_mult";

		inPortMap("X", multIn1SigName);
		if(!tile.first.isSquarer()) inPortMap("Y", multIn2SigName);
		outPortMap("R", output_name);
		for(unsigned i = 1; i < parameters.getOutputWeights().size(); i++){
			outPortMap("R" + to_string(i), output_name + to_string(i));
		}

		auto mult = parameters.generateOperator(this, getTarget());

		vhdl << instance(mult, nameOutput.str(), false) <<endl;
		return mult;
	}

	string IntMultiplier::addUID(string name, int blockUID)
	{
		ostringstream s;

		s << name << "_m" << multiplierUid;
		if(blockUID != -1)
			s << "b" << blockUID;
		return s.str() ;
	}

	unsigned int IntMultiplier::getOutputLengthNonZeros(
			BaseMultiplierParametrization const & parameter,
			unsigned int xPos,
			unsigned int yPos,
			unsigned int totalOffset
		)
	{
//        unsigned long long int sum = 0;
		mpfr_t sum;
		mpfr_t two;

		mpfr_inits(sum, two, NULL);

		mpfr_set_si(sum, 1, GMP_RNDN); //init sum to 1 as at the end the log(sum+1) has to be computed
		mpfr_set_si(two, 2, GMP_RNDN);

/*
	cout << "sum=";
	mpfr_out_str(stdout, 10, 0, sum, MPFR_RNDD);
	cout << endl;
	flush(cout);
*/

		for(unsigned int i = 0; i < parameter.getTileXWordSize(); i++){
			for(unsigned int j = 0; j < parameter.getTileYWordSize(); j++){
				if(i + xPos >= totalOffset && j + yPos >= totalOffset){
					if(i + xPos < (wX + totalOffset) && j + yPos < (wY + totalOffset)){
						if(parameter.shapeValid(i,j)){
							//sum += one << (i + j);
							mpfr_t r;
							mpfr_t e;
							mpfr_inits(r, e, NULL);
							mpfr_set_si(e, i+j, GMP_RNDD);
							mpfr_pow(r, two, e, GMP_RNDD);
							mpfr_add(sum, sum, r, GMP_RNDD);
//                            cout << " added 2^" << (i+j) << endl;
						}
						else{
							//cout << "not true " << i << " " << j << endl;
						}
					}
					else{
						//cout << "upper borders " << endl;
					}
				}
				else{
					//cout << "lower borders" << endl;
				}
			}
		}

		mpfr_t length;
		mpfr_inits(length, NULL);
		mpfr_log2(length, sum, GMP_RNDU);
		mpfr_ceil(length,length);

		unsigned long length_ul = mpfr_get_ui(length, GMP_RNDD);
//        cout << " outputLength has a length of " << length_ul << endl;
		return (int) length_ul;
	}

	/*this function computes the amount of zeros at the lsb position (mode 0). This is done by checking for every output bit position, if there is a) an input of the current BaseMultiplier, and b) an input of the IntMultiplier. if a or b or both are false, then there is a zero at this position and we check the next position. otherwise we break. If mode==1, only condition a) is checked */
	/*if mode is 2, only the offset for the bitHeap is computed. this is done by counting how many diagonals have a position, where is an IntMultiplierInput but not a BaseMultiplier input. */
	unsigned int IntMultiplier::getLSBZeros(
			BaseMultiplierParametrization const & param,
			unsigned int xPos,
			unsigned int yPos,
			unsigned int totalOffset,
			int mode
		){
		//cout << "mode is " << mode << endl;
		unsigned int max = min(int(param.getMultXWordSize()), int(param.getMultYWordSize()));
		unsigned int zeros = 0;
		unsigned int mode2Zeros = 0;
		bool startCountingMode2 = false;

		for(unsigned int i = 0; i < max; i++){
			unsigned int steps = i + 1; //for the lowest bit make one step (pos 0,0), second lowest 2 steps (pos 0,1 and 1,0) ...
			//the relevant positions for every outputbit lie on a diogonal line.


			for(unsigned int j = 0; j < steps; j++){
				bool bmHasInput = false;
				bool intMultiplierHasBit = false;
				if(param.shapeValid(j, steps - (j + 1))){
					bmHasInput = true;
				}
				if(bmHasInput && mode == 1){
					return zeros;   //only checking condition a)
				}
				//x in range?
				if((xPos + j >= totalOffset && xPos + j < (wX + totalOffset))){
					//y in range?
					if((yPos + (steps - (j + 1))) >= totalOffset && (yPos + (steps - (j + 1))) < (wY + totalOffset)){
						intMultiplierHasBit = true;
					}
				}
				if(mode == 2 && yPos + xPos + (steps - 1) >= 2 * totalOffset){
					startCountingMode2 = true;
				}
				//cout << "position " << j << "," << (steps - (j + 1)) << " has " << bmHasInput << " and " << intMultiplierHasBit << endl;
				if(bmHasInput && intMultiplierHasBit){
					//this output gets some values. So finished computation and return
					if(mode == 0){
						return zeros;
					}
					else{	//doesnt matter if startCountingMode2 is true or not. mode2Zeros are being incremented at the end of the diagonal
						return mode2Zeros;
					}

				}
			}

			zeros++;
			if(startCountingMode2 == true){
				mode2Zeros++;
			}
		}
		if(mode != 2){
			return zeros;       //if reached, that would mean the the bm shares no area with IntMultiplier
		}
		else{
			return mode2Zeros;
		}
	}

	void IntMultiplier::emulate (TestCase* tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svR;
        if(squarer) svY = svX;
//		cerr << "X : " << svX.get_str(10) << " (" << svX.get_str(2) << ")" << endl;
//		cerr << "Y : " << svY.get_str(10) << " (" << svY.get_str(2) << ")" << endl;


		if(signedIO)
		{
			// Manage signed digits
			mpz_class big1 = (mpz_class{1} << (wX));
			mpz_class big1P = (mpz_class{1} << (wX-1));
			mpz_class big2 = (mpz_class{1} << (wY));
			mpz_class big2P = (mpz_class{1} << (wY-1));

			if(svX >= big1P) {
				svX -= big1;
				//cerr << "X is neg. Interpreted value : " << svX.get_str(10) << endl;
			}

			if(svY >= big2P) {
				svY -= big2;
				//cerr << "Y is neg. Interpreted value : " << svY.get_str(10) << endl;
			}
		}
		svR = svX * svY;
		//cerr << "Computed product : " << svR.get_str() << endl;

		if(negate)
			svR = -svR;

		// manage two's complement at output
		if(svR < 0) {
			svR += (mpz_class(1) << wFullP);
			//cerr << "Prod is neg : encoded value : " << svR.get_str(2) << endl;
		}

		if(wOut>=wFullP) {
			tc->addExpectedOutput("R", svR);
			//cerr << "Exact result is required" << endl;
		}
		else {
			// there is truncation, so this mult should be faithful
			// TODO : this is not faithful but almost faithful :
			// currently only test if error <= ulp, faithful is < ulp
			mpz_class svRtrunc = (svR >> (wFullP-wOut));
			tc->addExpectedOutput("R", svRtrunc);
			if ((svRtrunc << (wFullP - wOut)) != svR) {
				svRtrunc++;
				svRtrunc &= (mpz_class(1) << (wOut)) -1;
				tc->addExpectedOutput("R", svRtrunc);
			}
		}
	}


	void IntMultiplier::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		mpz_class x, y;

		// 1*1
		x = mpz_class(1);
		y = mpz_class(1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// -1 * -1
		x = (mpz_class(1) << wX) -1;
		y = (mpz_class(1) << wY) -1;
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// The product of the two max negative values overflows the signed multiplier
		x = mpz_class(1) << (wX -1);
		y = mpz_class(1) << (wY -1);
		tc = new TestCase(this);
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);
	}





	OperatorPtr IntMultiplier::parseArguments(OperatorPtr parentOp, Target *target, std::vector<std::string> &args) {
		int wX,wY, wOut, maxDSP;
		bool signedIO,superTile, use2xk, useirregular, useLUT, useDSP, useKaratsuba, optiTrunc, minStages, squarer;
		double dspOccupationThreshold=0.0;
		int beamRange = 0;

		UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
		UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);
		UserInterface::parsePositiveInt(args, "wOut", &wOut);
		UserInterface::parseBoolean(args, "signedIO", &signedIO);
		UserInterface::parseBoolean(args, "superTile", &superTile);
		UserInterface::parseBoolean(args, "use2xk", &use2xk);
		UserInterface::parseBoolean(args, "useirregular", &useirregular);
		UserInterface::parseBoolean(args, "useLUT", &useLUT);
		UserInterface::parseBoolean(args, "useDSP", &useDSP);
		UserInterface::parseBoolean(args, "useKaratsuba", &useKaratsuba);
		UserInterface::parseFloat(args, "dspThreshold", &dspOccupationThreshold);
		UserInterface::parseInt(args, "maxDSP", &maxDSP);
        UserInterface::parseBoolean(args, "optiTrunc", &optiTrunc);
        UserInterface::parseBoolean(args, "minStages", &minStages);
		UserInterface::parsePositiveInt(args, "beamRange", &beamRange);
		UserInterface::parseBoolean(args, "squarer", &squarer);

		return new IntMultiplier(parentOp, target, wX, wY, wOut, signedIO, dspOccupationThreshold, maxDSP, superTile, use2xk, useirregular, useLUT, useDSP, useKaratsuba, beamRange, optiTrunc, minStages, squarer);
	}



	void IntMultiplier::registerFactory(){
		UserInterface::add("IntMultiplier", // name
											 "A pipelined integer multiplier.  Also uses the global options: tiling, ilpSolver, etc",
											 "BasicInteger", // category
											 "", // see also
											 "wX(int): size of input X; wY(int): size of input Y;\
						wOut(int)=0: size of the output if you want a truncated multiplier. 0 for full multiplier;\
						signedIO(bool)=false: inputs and outputs can be signed or unsigned;\
						maxDSP(int)=-1: limits the number of DSP-Tiles used in Multiplier;\
						use2xk(bool)=false: if true, attempts to use the 2xk-LUT-Multiplier with relatively high efficiency;\
						useirregular(bool)=false: if true, attempts to use the irregular-LUT-Multipliers with higher area/lut efficiency than the rectangular versions;\
						useLUT(bool)=true: if true, attempts to use the LUT-Multipliers for tiling;\
						useDSP(bool)=true: if true, attempts to use the DSP-Multipliers for tiling;\
						useKaratsuba(bool)=false: if true, attempts to use rectangular Karatsuba for tiling;\
						superTile(bool)=false: if true, attempts to use the DSP adders to chain sub-multipliers. This may entail lower logic consumption, but higher latency.;\
						dspThreshold(real)=0.0: threshold of relative occupation ratio of a DSP multiplier to be used or not;\
                        optiTrunc(bool)=true: if true, considers the Truncation error dynamicly, instead of defining a hard border for tiling, like in th ARITH paper;\
                        minStages(bool)=true: if true, minimizes stages in combined opt. of tiling an comp., otherwise try to find a sol. with less LUTs and more stages;\
						beamRange(int)=3: range for beam search;\
                        squarer(bool)=false: generate squarer", // This string will be parsed
											 "", // no particular extra doc needed
											IntMultiplier::parseArguments,
											IntMultiplier::unitTest
											 ) ;
	}


	TestList IntMultiplier::unitTest(int )
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		list<pair<int,int>> wordSizes = {{1,1},{2,2},{3,3},{4,4},{8,8},{16,8},{8,16},{13,17},{24,24},{27,41},{35,35},{53,53},{64,64},{10,99},{99,10},{100,100}};

		for (auto wordSizePair : wordSizes)
			{
				int wX = wordSizePair.first;
				int wY = wordSizePair.second;
				for(int sign=0; sign < 2; sign++)
					{
						paramList.push_back(make_pair("wX", to_string(wX)));
						paramList.push_back(make_pair("wY", to_string(wY)));
						paramList.push_back(make_pair("signedIO", sign ? "true" : "false"));
#if 0		// commented by Florent: better leave the default if you don't actually test various values

						paramList.push_back(make_pair("dspThreshold", to_string(1.0)));
						paramList.push_back(make_pair("maxDSP", to_string(0)));
#endif

						testStateList.push_back(paramList);

						// same parameters, but truncate (to the size of the smallest input for no particular reason)
						paramList.push_back(make_pair("wOut", to_string(wX)));
						testStateList.push_back(paramList);

						paramList.clear();
					}
			}

		return testStateList;
	}

    mpz_class IntMultiplier::checkTruncationError(list<TilingStrategy::mult_tile_t> &solution, unsigned guardBits, const mpz_class& errorBudget, const mpz_class& constant, int wX, int wY, bool signedIO) {
        std::vector<std::vector<int>> mulAreaI(wX, std::vector<int>(wY,0));

        for(auto & tile : solution) {
            auto &parameters = tile.first;
            auto &anchor = tile.second;
            int xPos = anchor.first;
            int yPos = anchor.second;

            for(int x = (0 <= xPos)?xPos:0; x < (int)((xPos + (int)parameters.getTileXWordSize() < wX)?xPos + parameters.getTileXWordSize():wX); x++){
                for(int y = (0 <= yPos)?yPos:0; y < (int)((yPos + (int)parameters.getTileYWordSize() < wY)?yPos + parameters.getTileYWordSize():wY); y++){
                    if( parameters.shapeValid(x-xPos,y-yPos) || parameters.isSquarer() ){
                        if(1 < std::abs(parameters.getTilingWeight())){
                            mulAreaI[x][y] = mulAreaI[x][y] + ((0 <= parameters.getTilingWeight())?1:(-1));
                            mulAreaI[y][x] = mulAreaI[y][x] + ((0 <= parameters.getTilingWeight())?1:(-1));
                        } else {
                            mulAreaI[x][y] = mulAreaI[x][y] + parameters.getTilingWeight();
                            if(mulAreaI[x][y] > 1 && mulAreaI[y][x] == 0){
                                mulAreaI[x][y]--;
                                mulAreaI[y][x]++;
                            }
                        }
                    }
                }
            }
        }

        mpz_class negTruncError, posTruncError, maxErr = errorBudget+constant;
        negTruncError = mpz_class(0); posTruncError = mpz_class(0);
        for(int y = 0; y < (int)wY; y++){
            for(int x = (int)wX-1; 0 <= x; x--){
                if(mulAreaI[x][y] != 1){
                    // partial product at left and bottom |_ edge count negative in signed case (but not at corner)
                    if(signedIO && ((wX-1 == x) != (wY-1 ==y))){
                        // when they are missing they contribute with a positive error
                        posTruncError += (mpz_class(1)<<(x+y));
                    } else {
                        negTruncError += (mpz_class(1)<<(x+y));
                    }
                }
                cout << ((mulAreaI[x][y] == 1) ? 1 : 0);
            }
            cout << endl;
        }

        if(max(negTruncError,posTruncError) <= maxErr){
            cout << "OK: actual truncation error=" << max(negTruncError,posTruncError) << " is smaller than the max. permissible error=" << maxErr << " by " << maxErr-max(negTruncError,posTruncError) << "." << endl;
        } else {
            cout << "ERROR: actual truncation error=" << max(negTruncError,posTruncError) << " is larger than the max. permissible error=" << maxErr << " by " << max(negTruncError,posTruncError)-maxErr << "." << endl;
        }
        return max(negTruncError,posTruncError);
	}

    int IntMultiplier::calcBitHeapLSB(list<TilingStrategy::mult_tile_t> &solution, unsigned guardBits, const mpz_class& errorBudget, const mpz_class& constant, const mpz_class& actualTruncError) const{
	    int col=0, nBits = 0;
	    std::vector<std::vector<int>> mulAreaI(wX, std::vector<int>(wY,0));

	    for(auto & tile : solution) {
	        auto &parameters = tile.first;
	        auto &anchor = tile.second;
	        int xPos = anchor.first;
	        int yPos = anchor.second;

	        for(int x = (0 <= xPos)?xPos:0; x < (int)((xPos + parameters.getTileXWordSize() < wX)?xPos + parameters.getTileXWordSize():wX); x++){
	            for(int y = (0 <= yPos)?yPos:0; y < (int)((yPos + parameters.getTileYWordSize() < wY)?yPos + parameters.getTileYWordSize():wY); y++){
	                if( parameters.shapeValid(x-xPos,y-yPos) || parameters.isSquarer() ){
	                    if(1 < std::abs(parameters.getTilingWeight())){
	                        mulAreaI[x][y] = mulAreaI[x][y] + ((0 <= parameters.getTilingWeight())?1:(-1));
	                        mulAreaI[y][x] = mulAreaI[y][x] + ((0 <= parameters.getTilingWeight())?1:(-1));
	                    } else {
	                        mulAreaI[x][y] = mulAreaI[x][y] + parameters.getTilingWeight();
	                        if(mulAreaI[x][y] > 1 && mulAreaI[y][x] == 0){
	                            mulAreaI[x][y]--;
	                            mulAreaI[y][x]++;
	                        }
	                    }
	                }
	            }
	        }
	    }

        mpz_class error, weight;
        do{
            nBits = 0;
            error = 0;
            cout << " min weight=" << col << endl;
            mpz_pow_ui(weight.get_mpz_t(), mpz_class(2).get_mpz_t(), col);
            for(int x = 0; x <= col && x < (int)wX; x++){
                for(int y = 0; x+y <= col && y < (int)wY; y++){
                    error += (mulAreaI[x][y] == 1)?weight:0;
                    nBits += (mulAreaI[x][y] == 1)?1:0;
                }
            }
            cout << "trying to prune " << nBits << " bits with weight " << col << " error is "  << error << " additional permissible error is " << errorBudget + constant - actualTruncError << endl;
            col++;
        } while(actualTruncError + error < errorBudget + constant || 0 == error);
        col--;
        cout << "min req weight is=" << col << endl;
        return col;
    }

    void IntMultiplier::createFigures(TilingStrategy *tilingStrategy) const {
	    ofstream texfile;
	    texfile.open("multiplier_tiling.tex");
	    if ((texfile.rdstate() & ofstream::failbit) != 0) {
	        cerr << "Error when opening multiplier_tiling.tex file for output. Will not print tiling configuration."
	        << endl;
	    } else {
	        tilingStrategy->printSolutionTeX(texfile, wOut, false);
	        texfile.close();
	    }

	    texfile.open("multiplier_tiling.svg");
	    if ((texfile.rdstate() & ofstream::failbit) != 0) {
	        cerr << "Error when opening multiplier_tiling.svg file for output. Will not print tiling configuration."
	        << endl;
	    } else {
	        tilingStrategy->printSolutionSVG(texfile, wOut, false);
	        texfile.close();
	    }

	    texfile.open("multiplier_shape.tex");
	    if ((texfile.rdstate() & ofstream::failbit) != 0) {
	        cerr << "Error when opening multiplier_shape.tex file for output. Will not print tiling configuration."
	        << endl;
	    } else {
	        tilingStrategy->printSolutionTeX(texfile, wOut, true);
	        texfile.close();
	    }
	}

}
