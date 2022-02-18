#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "PrimitiveComponents/Xilinx/XilinxFourToTwoCompressor.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"

using namespace std;
namespace flopoco{

    XilinxFourToTwoCompressor::XilinxFourToTwoCompressor(Operator* parentOp, Target* target, int width, bool useLastColumn) : Compressor(parentOp, target)
    {
        setCopyrightString("Martin Kumm");

        //compressors are supposed to be combinatorial
        setCombinatorial();
        setShared();

        this->useLastColumn = useLastColumn;
        setWidth(width);

        for(int i = 0; i < heights.size(); i++){
            cout << heights[i] << ", ";
        }
        cout << endl;
        for(int i = 0; i < outHeights.size(); i++){
            cout << outHeights[i] << ", ";
        }
        cout << endl;

        ostringstream name;
        name << "Compressor_4_to_2_type" << useLastColumn << "_width_" << width;
        setNameWithFreqAndUID(name.str());

        for(unsigned i=0;i<heights.size();i++)
        {
            if(heights[heights.size()-i-1] > 0)
                addInput(join("X",i), heights[heights.size()-i-1]);
        }

        addOutput("R0", wOut);
        addOutput("R1", wOut);

        int needed_cc = ( width / 4 ) + ( width % 4 > 0 ? 1 : 0 ); //no. of required carry chains

        REPORT(DEBUG, "no of required carry-chains for width=" << width << " is " << needed_cc);

        declare( "cc_s", needed_cc * 4 );
        declare( "cc_di", needed_cc * 4 );
        declare( "cc_co", needed_cc * 4 );
        declare( "cc_o", needed_cc * 4 );

        //init unused carry-chain inputs to zero:
        if(needed_cc*4 > width)
        {
            vhdl << tab << "cc_s(" << needed_cc*4-1 << " downto " << width << ") <= (others => '0');" << endl;
            vhdl << tab << "cc_di(" << needed_cc*4-1 << " downto " << width << ") <= (others => '0');" << endl;
        }
        vhdl << tab << "cc_di(" << width-1 << " downto 0) <= ";

        if(useLastColumn)
            vhdl << "X" << width-1 << "(1)";
        else
            vhdl << "'0'";

        for(int i=1; i < width; i++)
        {
            vhdl << " & X" << width-i-1 << "(3)";
        }
        vhdl << ";" << endl;

        vhdl << endl;

        //create LUTs, except the last LUT:
        for(int i=0; i < width-1; i++)
        {
            //LUT content of the LUTs exept the last LUT:
            lut_op lutop_o6 = lut_in(0) ^ lut_in(1) ^ lut_in(2) ^ lut_in(3); //sum out of full adder xor input 3
            lut_op lutop_o5 = (lut_in(0) & lut_in(1)) | (lut_in(0) & lut_in(2)) | (lut_in(1) & lut_in(2)); //carry out of full adder
            lut_init lutop( lutop_o5, lutop_o6 );

            Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2(this,target);
            cur_lut->setGeneric( "init", lutop.get_hex(), 64 );

            inPortMap("i0",join("X",i) + of(0));
            inPortMap("i1",join("X",i) + of(1));
            inPortMap("i2",join("X",i) + of(2));
            inPortMap("i3",join("X",i) + of(3));
            inPortMapCst( "i4","'0'");
            inPortMapCst( "i5","'1'");

            outPortMap("o5","R1" + of(i+1));
            outPortMap("o6","cc_s" + of(i));

            //        vhdl << instance(join("lut",i)) << endl;
            vhdl << cur_lut->primitiveInstance(join("lut",i)) << endl;
            //UserInterface::addToGlobalOpList(cur_lut);
            //        addSubComponent(cur_lut);
        }

        if(useLastColumn)
        {
            //create last LUT:
            lut_op lutop_o6 = lut_in(0) ^ lut_in(3); //input 0 xor input 3
            lut_op lutop_o5 = lut_in(3); //identical with input 3
            lut_init lutop( lutop_o5, lutop_o6 );

            Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2(this,target);
            cur_lut->setGeneric( "init", lutop.get_hex(), 64 );

            inPortMap("i0",join("X",width-1) + of(0));
            inPortMapCst("i1","'0'");
            inPortMapCst("i2","'0'");
            inPortMap("i3",join("X",width-1) + of(1));
            inPortMapCst( "i4","'0'");
            inPortMapCst( "i5","'1'");
            outPortMap("o5","open");
            //        outPortMap("o5",declare("X"));
            outPortMap("o6","cc_s" + of(width-1));

            vhdl << cur_lut->primitiveInstance(join("lut",width-1)) << endl;
        }
        else
        {
            vhdl << "cc_s" + of(width-1) << " <= '0';" << endl;
        }

        for( int i = 0; i < needed_cc; i++ ) {
            Xilinx_CARRY4 *cur_cc = new Xilinx_CARRY4(this,target);

            inPortMapCst( "cyinit", "'0'" );
            if( i == 0 ) {
                inPortMapCst( "ci", "'0'" ); //carry-in can not be used as AX input is blocked!!
            } else {
                inPortMap( "ci", "cc_co" + of( i * 4 - 1 ) );
            }
            inPortMap( "di", "cc_di" + range( i * 4 + 3, i * 4 ) );
            inPortMap( "s", "cc_s" + range( i * 4 + 3, i * 4 ) );
            outPortMap( "co", "cc_co" + range( i * 4 + 3, i * 4 ));
            outPortMap( "o", "cc_o" + range( i * 4 + 3, i * 4 ));

            stringstream cc_name;
            cc_name << "cc_" << i;
            //        vhdl << instance(cur_cc,cc_name.str()) << endl;
            vhdl << cur_cc->primitiveInstance(cc_name.str()) << endl;
        }

        vhdl << endl;

        vhdl << tab << "R0 <= cc_co(" << width-1 << ") & cc_o(" << width-1 << " downto 0);" << endl;

        reverse(heights.begin(), heights.end());        //the order of the heights is reversed in the 4:2 compressor class
        reverse(outHeights.begin(), outHeights.end());
        heights.erase(heights.cend()-1);
        outHeights.erase(outHeights.end()-1);
        /*cout << "heights" << endl;
        for(int i = 0; i < heights.size(); i++){
            cout << heights[i] << "," ;
        }
        cout << endl;
        for(int i = 0; i < outHeights.size(); i++){
            cout << outHeights[i] << "," ;
        }
        cout << endl;*/
    }

    XilinxFourToTwoCompressor::~XilinxFourToTwoCompressor()
    {
    }

    void XilinxFourToTwoCompressor::setWidth(int width)
    {
        this->width = width;

        //adjust size of basic compressor to the size of a variable column compressor of this specific size:
        heights.resize(width);

        //no of bits at MSB position
        if(useLastColumn)
            heights[0] = 2;
        else
            heights[0] = 0;

        for(int i=1; i < width-1; i++)
        {
            heights[i] = 4;
        }
        heights[width-1] = 4;

        wOut=width+1;

        outHeights.resize(wOut);
        if(useLastColumn)
            outHeights[0] = 1; //there is one output at MSB
            else
                outHeights[0] = 0; //there is no output at MSB

                for(int i=1; i < wOut-1; i++)
                {
                    outHeights[i] = 2;
                }
                outHeights[wOut-1] = 1; //there is one output at LSB
    }


    OperatorPtr XilinxFourToTwoCompressor::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args )
    {
        if( target->getVendor() != "Xilinx" )
            throw std::runtime_error( "Can't build xilinx primitive on non xilinx target" );

        int wOut;
        bool useLastColumn;
        UserInterface::parseInt(args,"wOut",&wOut );
        UserInterface::parseBoolean(args,"useLastColumn",&useLastColumn );

        return new XilinxFourToTwoCompressor(parentOp, target, wOut, useLastColumn);
    }

    void XilinxFourToTwoCompressor::registerFactory()
    {
        UserInterface::add( "XilinxFourToTwoCompressor", // name
                            "An efficient 4:2 compressor build of xilinx primitives.", // description, string
                            "Primitives", // category, from the list defined in UserInterface.cpp
                            "",
                            "wOut(int): The wordsize of the 4:2 compressor;\
                            useLastColumn(bool)=0: if the 4:2 compressor should additonally compress two bits in the last column, this should be set to true;",
                            "",
                            XilinxFourToTwoCompressor::parseArguments
                            );
    }


    void BasicXilinxFourToTwoCompressor::calc_widths(int wIn, vector<int> &heights, vector<int> &outHeights){
        vector<int> comp_inputs, comp_outputs;
        comp_inputs.resize(wIn); comp_outputs.resize(wIn+1);
        for(int i = 0; i <= wIn; i++){
            if(i == 0) {
                comp_outputs[i] = 1;
            } else {
                comp_outputs[i] = 2;
            }
            if(i == wIn){
                //comp_inputs[i] = 0;
            } else {
                comp_inputs[i] = 4;
            }
        }
        heights = comp_inputs;
        outHeights = comp_outputs;
    }

    vector<int> BasicXilinxFourToTwoCompressor::calc_heights(int wIn){
        vector<int> comp_inputs, comp_outputs;
        calc_widths(wIn, comp_inputs, comp_outputs);
        return comp_inputs;
    }

    BasicXilinxFourToTwoCompressor::BasicXilinxFourToTwoCompressor(Operator* parentOp_, Target * target, int wIn) : BasicCompressor(parentOp_, target, calc_heights( wIn), 0, CompressorType::Variable, true), wIn(wIn)
    {
        //cout << "wIn= " << wIn << endl;
        area = wIn; //1 LUT per bit
        calc_widths(wIn, heights, outHeights);
        /*cout << heights.size() << " " << outHeights.size() << endl;
        for(int i = 0; i < heights.size(); i++){
            cout << heights[i] << ", ";
        }
        cout << endl;
        for(int i = 0; i < outHeights.size(); i++){
            cout << outHeights[i] << ", ";
        }
        cout << endl;*/
        rcType = 2;
    }

    Compressor* BasicXilinxFourToTwoCompressor::getCompressor(unsigned int middleLength){
        if (middleLength >= 0) {
            area = middleLength + 2;
            calc_widths(middleLength+2, heights, outHeights);
        }
        cout << "here " << wIn << endl;
        compressor = new XilinxFourToTwoCompressor(parentOp, target, wIn, false);
        return compressor;
    }

}