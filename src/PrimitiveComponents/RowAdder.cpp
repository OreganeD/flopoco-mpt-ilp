#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "PrimitiveComponents/RowAdder.hpp"
#include "IntAddSubCmp/IntAdder.hpp"


using namespace std;
namespace flopoco {

    RowAdder::RowAdder(Operator *parentOp, Target *target, vector<int> _heights, vector<int> _outHeights) : Compressor(
            parentOp, target) {
        setCopyrightString("Andreas Boettcher");

        outHeights = _outHeights;
        heights = _heights;

        //compressors are supposed to be combinatorial
        setCombinatorial();
        setShared();

        //createInputsAndOutputs();

        for(int i=heights.size()-1; i>=0; i--) {
            //no need to create a signal for columns of height 0
            if (heights[i] > 0) {
                addInput(join("X", i), heights[i]);
                cerr << " creating input " << join("X", i) << " width " << heights[i] << endl;
            }
        }
        addOutput("R", heights.size()+1);

        ostringstream name;
        name << "Row_Adder_" << heights.size() ;
        setNameWithFreqAndUID(name.str());
        cerr << "in-heights: " << heights.size() << " out-heights: " << outHeights.size() << endl;

        int adderUid = parentOp->getNewUId();
        ostringstream adderIn0, adderIn0Name, adderIn1, adderIn1Name, adderOutName, adderCin, adderCinName;
        //create the names for the inputs/output of the adder
        adderIn0Name << "adder" << adderUid << "_In0";
        adderIn1Name << "adder" << adderUid << "_In1";
        adderCinName << "adder" << adderUid << "_Cin";
        adderOutName << "adder" << adderUid << "_Out";

        adderIn0 << "\"0\"";
        adderIn1 << "\"0\"";
        adderCin << join("X",0) << "(2 downto 2)";
        for(int i = heights.size()-1; 0 <= i; i--){
            adderIn0 << " & " << join("X",i) << "(0)";
            adderIn1 << " & " << join("X",i) << "(1)";
        }
        vhdl << tab << declare(adderIn0Name.str(), heights.size()+1) << " <= " << adderIn0.str() << ";" << endl;
        vhdl << tab << declare(adderIn1Name.str(), heights.size()+1) << " <= " << adderIn1.str() << ";" << endl;
        vhdl << tab << declare(adderCinName.str(), 1) << " <= " << adderCin.str() << ";" << endl;

/*        cout << "wIn=" + to_string(heights.size()+1) << endl;
        cout << adderIn0.str() << endl;
        cout << adderIn1.str() << endl;
        cout << adderCin.str() << endl;
*/
/*        newInstance("IntAdder",
                                       "RowAdder",
                                       "wIn=" + to_string(heights.size()+1),
                                       "X=>"+ adderIn0Name.str()
                                       + ",Y=>"+adderIn1Name.str()
                                       + ",Cin=>" + adderCinName.str(),
                                       "R=>"+ adderOutName.str()   );
*/
/*        IntAdder *cur_cc = new IntAdder(this,target, heights.size()+1);
        inPortMap( "X", adderIn0Name.str());
        inPortMap( "Y", adderIn1Name.str());
        inPortMap( "Cin", adderCinName.str());
        outPortMap( "R", adderOutName.str());*/


        //vhdl << tab << "R <= " << declare(adderOutName.str(), heights.size()+1) << ";" << endl;

        //vhdl << tab << "R <= " << adderOutName.str() << ";" << endl;
        //cout << vhdl.str();

        vhdl << tab << declare(getTarget()->adderDelay(heights.size()+1),adderOutName.str(), heights.size()+1);
        vhdl << " <= " << adderIn0Name.str() << " + " << adderIn1Name.str() << " + " << adderCinName.str() << ";" << endl;
        vhdl << tab << "R <= " << adderOutName.str() << ";" << endl;

    }

    RowAdder::RowAdder(Operator *parentOp, Target *target, int wIn) : Compressor(
            parentOp, target) {
        calc_widths(wIn, heights, outHeights);
        RowAdder(parentOp, target, heights, outHeights);
    }

    void RowAdder::calc_widths(int wIn, vector<int> &heights, vector<int> &outHeights){
        vector<int> comp_inputs, comp_outputs;
        for(int i = 0; i <= wIn; i++){
            comp_outputs.push_back(1);
            if(i == 0) comp_inputs.push_back(3);
            if(0 < i && i < wIn) comp_inputs.push_back(2);
        }
        heights = comp_inputs;
        outHeights = comp_outputs;
    }

    vector<int> BasicRowAdder::calc_heights(int wIn){
        vector<int> comp_inputs;
        for(int i = 0; i <= wIn; i++){
            if(i == 0) comp_inputs.push_back(3);
            if(0 < i && i < wIn) comp_inputs.push_back(2);
        }
        return comp_inputs;
    }

    BasicRowAdder::BasicRowAdder(Operator* parentOp_, Target * target, int wIn) : BasicCompressor(parentOp_, target, calc_heights( wIn), 0, CompressorType::Variable, true)
    {
        area = wIn; //1 LUT per bit
        RowAdder::calc_widths(wIn, heights, outHeights);
        cout << heights.size() << " " << outHeights.size() << endl;
        rcType = 0;
    }

    Compressor* BasicRowAdder::getCompressor(unsigned int middleLength){
        if (middleLength >= 0) {
            area = middleLength + 2;
            RowAdder::calc_widths(middleLength+2, heights, outHeights);
        }

        compressor = new RowAdder(parentOp, target, heights, outHeights);
        return compressor;
    }

    RowAdder::~RowAdder() {
    }

    OperatorPtr RowAdder::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
        int wIn;
        UserInterface::parseInt(args, "wIn", &wIn);
        return new RowAdder(parentOp, target, wIn);
    }

    void RowAdder::registerFactory() {
        UserInterface::add("RowAdder", // name
                           "Row adder for cormpression.", // description, string
                           "Primitives", // category, from the list defined in UserInterface.cpp
                           "",
                           "wIn(int): input width of the row adder",
                           "",
                           RowAdder::parseArguments
        );
    }

}
