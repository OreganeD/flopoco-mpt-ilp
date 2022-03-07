#include "PrimitiveComponents/ASIC/ApproxCompressor_NAC18.hpp"
#include <string>

using namespace std;

namespace flopoco{

    ApproxCompressor_NAC18::ApproxCompressor_NAC18(Operator* parentOp, Target * target, vector<int> heights_) : Compressor(parentOp, target){
        this->heights = heights_;
        //compressors are supposed to be combinatorial
        setCombinatorial();
        setShared();

        //set name
        ostringstream o_name;
        o_name << "ApproxCompressor_NAC18_" << heights[0] << "_" << ((4<heights[0])?3:2);
        setNameWithFreqAndUID(o_name.str());
    
        if(heights[0] == 6){
            wIn=1;
            wOut=1;
            //heights = vector<int> {6};      //Input bits of the compressor in first column
            outHeights = vector<int> {3};   //Output bits of the compressor in first column
            declareIO_Signals();            //Declare the inputs an outputs
            for(int n = 9; n <= 20; n++){
                declare(0, join("n",n));	//to declare temporary n variables
            }
            for(int y = 0; y < outHeights[0]; y++){
                declare(target->logicDelay(), join("y",y));	//to declare temporary y variables to consider delays
                vhdl << tab << ((y == 0)?"R":join("R",y)) << "(0) <= " << join("y",y) << ";" << endl;
            }

            vhdl << tab << "n9   <= not(X0(4)) and not(X0(2));" << endl;
            vhdl << tab << "n10  <= not(X0(5)) or not(X0(1));" << endl;
            vhdl << tab << "n11  <= not(X0(3));" << endl;
            vhdl << tab << "n12  <= (not(n11) and not(X0(0))) or (not(X0(1)) and not(X0(5)));" << endl;
            vhdl << tab << "y0 <= not(n12) or not(n10) or not(n9);" << endl;
    
            vhdl << tab << "n14  <= not(X0(4)) or not(X0(2));" << endl;
            vhdl << tab << "n15  <= not(n14) or not(n10);" << endl;
            vhdl << tab << "n16  <= not(n15) or not(X0(0));" << endl;
            vhdl << tab << "n17  <= (not(X0(4)) and not(X0(2))) or (not(X0(1)) and not(X0(5)));" << endl;
            vhdl << tab << "y1 <= not(n17) or not(n16) or not(n11);" << endl;
    
            vhdl << tab << "n19  <= not(X0(0));" << endl;
            vhdl << tab << "n20  <= (not(X0(5)) and not(X0(1))) or not(X0(3));" << endl;
            vhdl << tab << "y2 <= not(n20) or not(n14) or not(n10) or not(n19);" << endl;
    
        } else {
            stringstream s;
            s << "Unsupported compressor with height: " << heights[0];
            THROWERROR(s.str());
        }

    }
	
	
    void ApproxCompressor_NAC18::declareIO_Signals(){
        //Create inputs of the compressor
        for(unsigned i=0;i<heights.size();i++){
            if(heights[heights.size()-i-1] > 0)
                addInput(join("X",i), heights[heights.size()-i-1]);
        }
        //Create outputs of the compressor
        for(int i=0;i<outHeights[0];i++){
            addOutput(((i == 0)?"R":join("R",i)),1);    //The first output has to be called R
        }
    }


    OperatorPtr ApproxCompressor_NAC18::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
    {
        string in;
        vector<int> heights;

        UserInterface::parseString(args, "columnHeights", &in);

        // tokenize the string, with ':' as a separator
        stringstream ss(in);
        while(ss.good())
        {
            string substr;

            getline(ss, substr, ',');
            heights.insert(heights.begin(), stoi(substr));
        }

        return new ApproxCompressor_NAC18(parentOp,target,heights);
    }

    void ApproxCompressor_NAC18::registerFactory(){
        UserInterface::add("ApproxCompressor_NAC18", // name
                           "Implements an Approximate Compressor \
                       Available Compressor sizes are: \
                       6; 5; 4; 3",
                           "Primitives", // categories
                           "",
                           "columnHeights(string): comma separated list of heights for the columns of the compressor, \
                                    in decreasing order of the weight. For example, columnHeights=\"6\" produces a (6:3) Compressor",
                           "",
                           ApproxCompressor_NAC18::parseArguments
                           ) ;
    }


    void ApproxCompressor_NAC18::emulate(TestCase *tc) {
        mpz_class sX;
        sX = tc->getInputValue(join("X", 0));

        if(heights[0] == 6){
            vector<mpz_class> x (heights[0]);
            for(int i = 0; i < heights[0]; i++){
                x[i] = ((sX>>i) & 1);               //individual input bits
            }
            mpz_class n9   = !(x[4]) && !(x[2]);
            mpz_class n10  = !(x[5]) || !(x[1]);
            mpz_class n11  = !(x[3]);
            mpz_class n12  = (!(n11) && !(x[0])) || (!(x[1]) && !(x[5]));
            mpz_class y0 = !(n12) || !(n10) || !(n9);

            mpz_class n14  = !(x[4]) || !(x[2]);
            mpz_class n15  = !(n14) || !(n10);
            mpz_class n16  = !(n15) || !(x[0]);
            mpz_class n17  = (!(x[4]) && !(x[2])) || (!(x[1]) && !(x[5]));
            mpz_class y1 = !(n17) || !(n16) || !(n11);

            mpz_class n19  = !(x[0]);
            mpz_class n20  = (!(x[5]) && !(x[1])) || !(x[3]);
            mpz_class y2 = !(n20) || !(n14) || !(n10) || !(n19);

            tc->addExpectedOutput("R", y0);
            tc->addExpectedOutput("R1", y1);
            tc->addExpectedOutput("R2", y2);
        } else {
            stringstream s;
            s << "Unsupported compressor with height: " << heights[0];
            THROWERROR(s.str());
        }

    }









}





