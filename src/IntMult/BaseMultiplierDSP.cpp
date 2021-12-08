#include "BaseMultiplierDSP.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"
#include "IntMult/DSPBlock.hpp"

namespace flopoco {

Operator* BaseMultiplierDSP::generateOperator(
		Operator *parentOp, 
		Target* target,
		Parametrization const &parameters) const
{
	//ToDo: no fliplr support anymore, find another solution!
	return new DSPBlock(
			parentOp, 
			target, 
            parameters.getMultXWordSize(),
			parameters.getMultYWordSize(),
			parameters.isSignedMultX(),
			parameters.isSignedMultY()
			); 
}

    double BaseMultiplierDSP::getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO){
        bool signedX = signedIO && (wMultX == x_anchor+(int)wX()+1 || wMultX == x_anchor+(int)wX());
        bool signedY = signedIO && (wMultY == y_anchor+(int)wY()+1 || wMultY == y_anchor+(int)wY());

        int x_min = ((x_anchor < 0)?0: x_anchor);
        int y_min = ((y_anchor < 0)?0: y_anchor);
        int lsb = x_min + y_min;

        int x_max = ((wMultX <= x_anchor + (int)wX() + (int)(signedX?1:0))?wMultX: x_anchor + (int)wX());
        int y_max = ((wMultY <= y_anchor + (int)wY() + (int)(signedY?1:0))?wMultY: y_anchor + (int)wY());
        int msb = x_max+y_max;

        int ws = (x_max-x_min==1)?y_max-y_min:((y_max-y_min==1)?x_max-x_min:msb - lsb);
        return ws*getBitHeapCompressionCostperBit();
    }

}   //end namespace flopoco

