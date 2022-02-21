#include "CompressionStrategyOptILP.hpp"
#include "PrimitiveComponents/RowAdder.hpp"
#include "PrimitiveComponents/Xilinx/XilinxFourToTwoCompressor.hpp"

using namespace std;
namespace flopoco {

    CompressionStrategyOptILP::CompressionStrategyOptILP(BitHeap* bitheap):CompressionStrategy(bitheap)
	{

	}

    void CompressionStrategyOptILP::solve()
{

#ifndef HAVE_SCALP
    throw "Error, TilingAndCompressionOptILP::solve() was called but FloPoCo was not built with ScaLP library";
#else
    cout << "using ILP solver " << bitheap->getOp()->getTarget()->getILPSolver() << endl;
    solver = new ScaLP::Solver(ScaLP::newSolverDynamic({bitheap->getOp()->getTarget()->getILPSolver(),"Gurobi","CPLEX","SCIP","LPSolve"}));
    solver->timeout = bitheap->getOp()->getTarget()->getILPTimeout();

    bitheap->final_add_height = 3;   //Select if dual or ternary adder is used in final stage of bitheap compression
    consider_final_adder = true;

    //remove_all_but_Adders();    //Simplify Modell for test purposes -- REMOVE WHEN PROBLEMS SOLVED ------------------------
    addFlipFlop();              //Add FF to list of compressors
    addRowAdder();

    //for debugging it might be better to order the compressors by efficiency
    orderCompressorsByCompressionEfficiency();

    ScaLP::status stat;
    s_max = 0;
    do{
        s_max++;
        //if(s_max > 4) break;
        solver->reset();
        cout << "constructing model with" << s_max << " stages..." << endl;
        constructProblem(s_max);

        // Try to solve
        cout << "starting solver, this might take a while..." << endl;
        solver->quiet = false;
        stat = solver->solve();

        // print results
        cerr << "The result is " << stat << endl;
        //cout << solver->getResult() << endl;
    } while(stat == ScaLP::status::INFEASIBLE);

    if(!consider_final_adder) s_max++;

    ScaLP::Result res = solver->getResult();

#endif
}

void CompressionStrategyOptILP::compressionAlgorithm() {
#ifndef HAVE_SCALP
		throw "Error, TilingAndCompressionOptILP::compressionAlgorithm() was called but FloPoCo was not built with ScaLP library";
#else
		//adds the Bits to stages and columns
		orderBitsByColumnAndStage();

		//populates bitAmount
		fillBitAmounts();

		//prints out how the inputBits of the bitheap looks like
		printBitAmounts();

        solve();

		//new solution
		CompressionStrategy::solution = BitHeapSolution();
		CompressionStrategy::solution.setSolutionStatus(BitheapSolutionStatus::OPTIMAL_PARTIAL);

		float compressor_cost = 0;
		vector<vector<int>> zeroInputsVector(s_max, vector<int>((int)wIn, 0));
        vector<vector<vector<int>>> row_adder(s_max, vector<vector<int>>((int)wIn, vector<int>(3*3, 0)));   //3 types of row adders that have 3 (L,M,R) elements (3x3=9)
		resizeBitAmount(s_max-1);
		ScaLP::Result res = solver->getResult();
		for(auto &p:res.values)
		{
			if(p.second > 0.5){     //parametrize all multipliers at a certain position, for which the solver returned 1 as solution, to flopoco solution structure
				std::string var_name = p.first->getName();
				//cout << var_name << "\t " << p.second << endl;
				//if(var_name.substr(0,1).compare("k") != 0) continue;
				switch(var_name.substr(0,1).at(0)) {
					case 'k':{      //decision variables 'k' for placed compressors
						int sta_id = stoi(var_name.substr(2, dpSt));
						int com_id = stoi(var_name.substr(2 + dpSt + 1, dpK));
						int col_id = stoi(var_name.substr(2 + dpSt + 1 + dpK + 1, dpC));
						cout << p.second << " compressor" <<  setw(2) << com_id << " stage " << sta_id << " column " <<  setw(3) << col_id
                             << " compressor type " << ((com_id<(int)possibleCompressors.size())?possibleCompressors[com_id]->getStringOfIO():"?") << endl;
                        if(possibleCompressors[com_id]->type == CompressorType::Variable){
                            for(int n = 0; n < (int)lrint(p.second);n++){
                                int index = (possibleCompressors[com_id]->subtype == subType::M)?1:0;
                                index += (possibleCompressors[com_id]->subtype == subType::L)?2:0;
                                index += 3*possibleCompressors[com_id]->rcType;
                                row_adder[sta_id][col_id][index] += 1;
                            }
                            break;
                        }
						compressor_cost += ((com_id < (int)possibleCompressors.size() && sta_id < s_max - 1)?possibleCompressors[com_id]->area:0) * p.second;
                        if (possibleCompressors[com_id] != flipflop && sta_id < s_max) {
							float instances = p.second;
							while (instances-- > 0.5) {
								CompressionStrategy::solution.addCompressor(sta_id, col_id, possibleCompressors[com_id]);
							}
						} else {
                            //cout << "skipped FF " << possibleCompressors[com_id]  << "==" << flipflop << " " << (possibleCompressors[com_id] == flipflop) << endl;
                        }
						break;
					}
					case 'Z':{          //compressor input bits that are set zero have to be recorded for VHDL-Generation
						int sta_id = stoi(var_name.substr(2, dpSt));
						int col_id = stoi(var_name.substr(2 + dpSt + 1, dpC));
						zeroInputsVector[sta_id][col_id] = (int)(-1);
						break;
					}
					default:            //Other decision variables are not needed for VHDL-Generation
						break;
				}
				for(int stage = 0; stage < s_max; stage++){
					CompressionStrategy::solution.setEmptyInputsByRemainingBits(stage, zeroInputsVector[stage]);
				}
			}
		}

		cout << "Total compressor LUT-cost: " << compressor_cost << endl;

        replace_row_adders(CompressionStrategy::solution, row_adder);

        //reports the area in LUT-equivalents
		printSolutionStatistics();
        drawBitHeap();
		//here the VHDL-Code for the compressors as well as the bits->compressors->bits are being written.
		applyAllCompressorsFromSolution();
#endif
}


#ifdef HAVE_SCALP
void CompressionStrategyOptILP::constructProblem(int s_max)
{
    cout << "constructing problem formulation..." << endl;

    //Assemble cost function, declare problem variables
    cout << "   assembling cost function, declaring problem variables..." << endl;
    ScaLP::Term obj;
    wIn = bitAmount[0].size();
    int ns = wS-1; dpS = 1;     //calc number of decimal places, for var names
    int nk = possibleCompressors.size()+4, nc = wIn + 1, nst = s_max; dpK = 1; dpC = 1; dpSt = 1;
    while (ns /= 10)
        dpS++;
    while (nk /= 10)
        dpK++;
    while (nc /= 10)
        dpC++;
    while (nst /= 10)
        dpSt++;

    vector<ScaLP::Term> bitsinColumn(wIn + 4);
    // add the Constraints
    cout << "   adding the constraints to problem formulation..." << endl;


    for(int i = 0; i < wIn; i++){               //Fill array for the bits initially available on Bitheap
        bitsinColumn[i].add(bitAmount[0][i]);
    }

    // BitHeap compression part of ILP formulation
    vector<vector<ScaLP::Variable>> bitsInColAndStage(s_max+1, vector<ScaLP::Variable>(bitsinColumn.size()+1));

    cout << "Available compressors :" << endl;
    for(unsigned e = 0; e < possibleCompressors.size(); e++){
        cout << possibleCompressors[e]->getStringOfIO() << " cost:"<< possibleCompressors[e]->area << endl;
    }

    for(int s = 0; s <= s_max; s++){
        vector<ScaLP::Term> bitsinNextColumn(wIn + 1);
        vector<ScaLP::Term> bitsinCurrentColumn(wIn + 1);
        vector<vector<ScaLP::Term>> rcdDependencies(wIn + 1, vector<ScaLP::Term>(3));                       //One term for every column and type of row-adder

        for(int c = 0; c <= wIn; c++){        //one bit more for carry of ripple carry adder

            if(s < s_max){
                for(unsigned e = 0; e < possibleCompressors.size(); e++){                                          //place every possible compressor on current position and the following that get covered by it, index extended by 3 for RCA
                    if(c == 0 && (possibleCompressors[e]->subtype == subType::M || possibleCompressors[e]->subtype == subType::L)) continue;
                    stringstream nvarName;
                    nvarName << "k_" << setfill('0') << setw(dpSt) << s << "_" << setfill('0') << setw(dpK) << e << "_" << setfill('0') << setw(dpC) << c;
                    ScaLP::Variable tempV = ScaLP::newIntegerVariable(nvarName.str(), 0, ScaLP::INF());
                    obj.add(tempV, possibleCompressors[e]->area );                    //append variable to cost function, for r.c.a.-area (cost) is 1 6LUT, FFs cost 0.5LUT
                    for(int ce = 0; ce < (int) possibleCompressors[e]->heights.size() && ce < (int)bitsinCurrentColumn.size() - (int)c; ce++){                                //Bits that can be removed by compressor e in stage s in column c for constraint C1
                        //cout << "take bits: " << possibleCompressors[e]->getHeightsAtColumn((unsigned) ce, false) << " c: " << c+ce << " " << nvarName.str() << endl;
                        bitsinCurrentColumn[c+ce].add(tempV, possibleCompressors[e]->heights[ce]);
                    }
                    for(int ce = 0; ce < (int) possibleCompressors[e]->outHeights.size() && ce < (int)bitsinNextColumn.size() - (int)c; ce++){   //Bits that are added by compressor e in stage s+1 in column c for constraint C2
                        //cout <<  "give bits: " << possibleCompressors[e]->getOutHeightsAtColumn((unsigned) ce, false) << " c: " << c+ce << " " << nvarName.str() << endl;
                        bitsinNextColumn[c+ce].add(tempV, possibleCompressors[e]->outHeights[ce]);
                    }
                    handleRowAdderDependencies(tempV,bitsinColumn, rcdDependencies, c, e);
                }
            }

            if(bitsInColAndStage[s][c] == nullptr){                                                                 //N_s_c: Bits that enter current compressor stage
                stringstream curBits;
                curBits << "N_" << s << "_" << c;
                //cout << curBits.str() << endl;
                bitsInColAndStage[s][c] = ScaLP::newIntegerVariable(curBits.str(), 0, ScaLP::INF());
            }
            if(s == 0 && bitsInColAndStage[s][c] != nullptr){
                C0_bithesp_input_bits(s, c, bitsinColumn, bitsInColAndStage);
            }
            if(s < s_max){
                C1_compressor_input_bits(s, c, bitsinCurrentColumn, bitsInColAndStage);
                C2_compressor_output_bits(s, c, bitsinNextColumn, bitsInColAndStage);
            }

            if(s_max == s){                                               //limitation of the BitHeap height to final_add_height in s_max-1 and 1 in stage s_max
                C3_limit_BH_height(s, c, bitsInColAndStage, consider_final_adder);
            }
            if(0 < c && s < s_max){                                         //define constraint for the stucture of the RCA
                C5_RCA_dependencies(s, c, rcdDependencies);
            }

        }
    }

    // Set the Objective
    cout << "   setting objective (minimize cost function)..." << endl;
    solver->setObjective(ScaLP::minimize(obj));

    // Write Linear Program to file for debug purposes
    cout << "   writing LP-file for debuging..." << endl;
    solver->writeLP("compression.lp");
}


    void CompressionStrategyOptILP::C0_bithesp_input_bits(int s, int c, vector<ScaLP::Term> &bitsinColumn, vector<vector<ScaLP::Variable>> &bitsInColAndStage){
        stringstream consName0;
        consName0 << "C0_" << s << "_" << c;
        bitsinColumn[c].add(bitsInColAndStage[s][c], -1);      //Output bits from sub-multipliers
        ScaLP::Constraint c0Constraint = bitsinColumn[c] == 0;     //C0_s_c
        c0Constraint.name = consName0.str();
        solver->addConstraint(c0Constraint);
    }

    void CompressionStrategyOptILP::C1_compressor_input_bits(int s, int c, vector<ScaLP::Term> &bitsinCurrentColumn, vector<vector<ScaLP::Variable>> &bitsInColAndStage){
        stringstream consName1, zeroBits;
        consName1 << "C1_" << s << "_" << c;
        zeroBits << "Z_" << setfill('0') << setw(dpSt) << s << "_" << setfill('0') << setw(dpC) << c;
        //cout << zeroBits.str() << endl;
        bitsinCurrentColumn[c].add(ScaLP::newIntegerVariable(zeroBits.str(), 0, ScaLP::INF()), -1);      //Unused compressor input bits, that will be set zero
        bitsinCurrentColumn[c].add(bitsInColAndStage[s][c], -1);      //Bits arriving in current stage of the compressor tree
        ScaLP::Constraint c1Constraint = bitsinCurrentColumn[c] == 0;     //C1_s_c
        c1Constraint.name = consName1.str();
        solver->addConstraint(c1Constraint);
    }

    void CompressionStrategyOptILP::C2_compressor_output_bits(int s, int c, vector<ScaLP::Term> &bitsinNextColumn, vector<vector<ScaLP::Variable>> &bitsInColAndStage){
        stringstream consName2, nextBits;
        consName2 << "C2_" << s << "_" << c;
        //cout << consName2.str() << endl;
        if(bitsInColAndStage[s+1][c] == nullptr){
            nextBits << "N_" << s+1 << "_" << c;
            //cout << nextBits.str() << endl;
            bitsInColAndStage[s+1][c] = ScaLP::newIntegerVariable(nextBits.str(), 0, ScaLP::INF());
        }
        bitsinNextColumn[c].add(bitsInColAndStage[s+1][c], -1); //Output Bits of compressors to next stage
        ScaLP::Constraint c2Constraint = bitsinNextColumn[c] == 0;     //C2_s_c
        c2Constraint.name = consName2.str();
        solver->addConstraint(c2Constraint);
    }

    void CompressionStrategyOptILP::C3_limit_BH_height(int s, int c, vector<vector<ScaLP::Variable>> &bitsInColAndStage, bool consider_final_adder){
        stringstream consName3;
        consName3 << "C3_" << s << "_" << c;
        ScaLP::Constraint c3Constraint = bitsInColAndStage[s][c] <= ((consider_final_adder)?1:bitheap->final_add_height);     //C3_s_c
        c3Constraint.name = consName3.str();
        solver->addConstraint(c3Constraint);
        //cout << consName3.str() << " " << stage.str() << endl;
    }

    void CompressionStrategyOptILP::C5_RCA_dependencies(int s, int c, vector<vector<ScaLP::Term>> &rcdDependencies){
        for(int rcType = 0; rcType < 3; rcType++){
            stringstream consName5;
            consName5 << "C5" << char('a'+ rcType) << "_" << s << "_" << c;
            //cout << consName5.str() << rcdDependencies[c-1][rcType] << endl;
            ScaLP::Constraint c5Constraint = rcdDependencies[c-1][rcType] == 0;     //C5_s_c
            c5Constraint.name = consName5.str();
            solver->addConstraint(c5Constraint);
        }
    }

    void CompressionStrategyOptILP::handleRowAdderDependencies(const ScaLP::Variable &tempV, vector<ScaLP::Term> &bitsinColumn, vector<vector<ScaLP::Term>> &rcdDependencies, unsigned int c, unsigned int e) const {
        if(possibleCompressors[e]->type == CompressorType::Variable){
            if(0 < c && (possibleCompressors[e]->subtype == subType::M || possibleCompressors[e]->subtype == subType::L))   //Middle and left element of RCA are counted negative in eq. for relations between RCA elements (C5)
                rcdDependencies[c-1][possibleCompressors[e]->rcType].add(tempV, -1);
            if(c < bitsinColumn.size()-1 && (possibleCompressors[e]->subtype == subType::M || possibleCompressors[e]->subtype == subType::R))    //Middle and right element of RCA are counted positive in eq. for relations between RCA elements (C5)
                rcdDependencies[c][possibleCompressors[e]->rcType].add(tempV, 1);
            if(c == bitsinColumn.size() && possibleCompressors[e]->subtype == subType::L)                                       //only the left (and not the middle) element of the RCA should be put in the MSB column of the bitheap
                rcdDependencies[c][possibleCompressors[e]->rcType].add(tempV, 1);
        }
    }

    bool CompressionStrategyOptILP::addFlipFlop(){
        //BasicCompressor* flipflop;
        bool foundFlipflop = false;
        for(unsigned int e = 0; e < possibleCompressors.size(); e++){
            //inputs
            if(possibleCompressors[e]->getHeights() == 1 && possibleCompressors[e]->getHeightsAtColumn(0) == 1){
                if(possibleCompressors[e]->getOutHeights() == 1 && possibleCompressors[e]->getOutHeightsAtColumn(0) == 1){
                    foundFlipflop = true;
                    flipflop = possibleCompressors[e];
                    break;
                }
            }
        }

        if(!foundFlipflop){
            BasicCompressor *newCompressor = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {1}, 0.5, CompressorType::Gpc, true);
            possibleCompressors.push_back(newCompressor);
            flipflop = newCompressor;
        }

        return !foundFlipflop;
    }

    void CompressionStrategyOptILP::addRowAdder(){
        {
            BasicCompressor *rightElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {3}, vector<int> {1}, 1, CompressorType::Variable, subType::R, 0);
            possibleCompressors.push_back(rightElement);
            BasicCompressor *middleElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {2}, vector<int> {1}, 1, CompressorType::Variable, subType::M, 0);
            possibleCompressors.push_back(middleElement);
            BasicCompressor *leftElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {0}, vector<int> {1}, 1, CompressorType::Variable, subType::L, 0);
            possibleCompressors.push_back(leftElement);
        }{
            BasicCompressor *rightElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {4}, vector<int> {1}, 1, CompressorType::Variable, subType::R, 1);
            possibleCompressors.push_back(rightElement);
            BasicCompressor *middleElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {3}, vector<int> {1}, 1, CompressorType::Variable, subType::M, 1);
            possibleCompressors.push_back(middleElement);
            BasicCompressor *leftElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {1}, vector<int> {1,1}, 1, CompressorType::Variable, subType::L, 1);
            possibleCompressors.push_back(leftElement);
        }{
            BasicCompressor *rightElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {4}, vector<int> {1}, 1, CompressorType::Variable, subType::R, 2);
            possibleCompressors.push_back(rightElement);
            BasicCompressor *middleElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {4}, vector<int> {2}, 1, CompressorType::Variable,  subType::M, 2);
            possibleCompressors.push_back(middleElement);
            BasicCompressor *leftElement = new BasicCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), vector<int> {0}, vector<int> {2}, 1, CompressorType::Variable, subType::L,2);
            possibleCompressors.push_back(leftElement);
        }
    }

    void CompressionStrategyOptILP::remove_all_but_Adders(){
        for(int e = possibleCompressors.size()-1; 0 <= e; e--){
            cout << possibleCompressors[e]->getStringOfIO();
            if(possibleCompressors[e]->getHeights() != 1 || !(possibleCompressors[e]->getHeightsAtColumn(0) == 2 || possibleCompressors[e]->getHeightsAtColumn(0) == 3)){
                possibleCompressors.erase(next(possibleCompressors.begin(), e));
            }
        }
    }

    void CompressionStrategyOptILP::resizeBitAmount(unsigned int stages){

        stages++;	//we need also one stage for the outputbits

        unsigned int columns = bitAmount[bitAmount.size() - 1].size();
        //we need one stage more for the
        while(bitAmount.size() < stages){
            bitAmount.resize(bitAmount.size() + 1);
            bitAmount[bitAmount.size() - 1].resize(columns, 0);
        }
    }

    void CompressionStrategyOptILP::printIOHeights(void){
        for(int j=0; j < (int)possibleCompressors.size(); j++){
            cout << "Compressor input heights: ";
            for(int i = possibleCompressors[j]->getHeights()-1; 0 < i; i--){
                cout << possibleCompressors[j]->getHeightsAtColumn(i);
            }
            cout << endl << "Compressor output heights: ";
            for(int o = possibleCompressors[j]->getOutHeights()-1; 0 < o; o--){
                cout << possibleCompressors[j]->getOutHeightsAtColumn(o);
            }
            cout << endl;
        }

    }

    void CompressionStrategyOptILP::drawBitHeap(){
        vector<vector<int>> bitsOnBitHeap(s_max+1, vector<int>((int)wIn, 0));
        vector<int> colWidth((int)wIn, 1);
        ScaLP::Result res = solver->getResult();
        for(auto &p:res.values) {
            if (p.second >
                0.5) {     //parametrize all multipliers at a certain position, for which the solver returned 1 as solution, to flopoco solution structure
                std::string var_name = p.first->getName();
                //if(var_name.substr(0,1).compare("k") != 0) continue;
                switch (var_name.substr(0, 1).at(0)) {
                    case 'p': {          //constant bits from sign extension of negative congruent pseudo-compressors
                        //int sta_id = stoi(var_name.substr(2, dpSt));
                        //int col_id = stoi(var_name.substr(2 + dpSt + 1, dpC));
                        //bitsOnBitHeap[sta_id][col_id] += 1;
                        cout << var_name << "\t " << p.second << endl;
                        break;
                    }
                    case 'N': {          //bits present in a particular column an stage of BitHeap
                        int sta_id = stoi(var_name.substr(2, dpSt));
                        int col_id = stoi(var_name.substr(2 + dpSt + 1, dpC));
                        if(sta_id < s_max+1 && col_id < wIn)
                            bitsOnBitHeap[sta_id][col_id] = p.second;
                        if(pow(10,colWidth[col_id]) <= p.second) colWidth[col_id]++;
                        //cout << var_name << "\t " << p.second << endl;
                        break;
                    }
                    default:            //Other decision variables are not needed for VHDL-Generation
                        break;
                }
            }
        }
        for(int c = bitsOnBitHeap[0].size()-1; 0 <= c; c--){
            cout << setw(colWidth[c]) << ((c%4==0)?'|':' ');
        }
        cout << endl;
        for(unsigned s = 0; s < bitsOnBitHeap.size(); s++){
            for(int c = bitsOnBitHeap[0].size()-1; 0 <= c; c--){
                cout << setw(colWidth[c]) << bitsOnBitHeap[s][c];
            }
            cout << endl;
        }
    }

    void CompressionStrategyOptILP::replace_row_adders(BitHeapSolution &solution, vector<vector<vector<int>>> &row_adders){
        cout << solution.getSolutionStatus() << endl;
        for(int rcType = 0; rcType < 3; rcType++){
            for(unsigned s = 0; s < row_adders.size(); s++){
                for(unsigned c = 0; c < row_adders[0].size(); c++){
                    //cout << "at stage " << s << " col " << c << " " << row_adders[s][c][0] << row_adders[s][c][1] << row_adders[s][c][2] << endl;
                    int ci; bool adder_started;
                    while(0 < row_adders[s][c][0+3*rcType]){
                        ci = 0;
                        adder_started = false;
                        while(0 < row_adders[s][c+ci][0+3*rcType] || 0 < row_adders[s][c+ci][1+3*rcType] || 0 < row_adders[s][c+ci][2+3*rcType]){
                            if(0 < row_adders[s][c+ci][0+3*rcType] && adder_started == false){
                                row_adders[s][c+ci][0+3*rcType]--;
                                adder_started = true;
                            } else {
                                if(adder_started || 0 < row_adders[s][c+ci][1+3*rcType] || 0 < row_adders[s][c+ci][2+3*rcType]){
                                    if(0 < row_adders[s][c+ci][1+3*rcType]){
                                        row_adders[s][c+ci][1+3*rcType]--;
                                    } else {
                                        row_adders[s][c+ci][2+3*rcType]--;
                                        adder_started = false;
                                        switch(rcType){
                                            case 0:{
                                                cout << "RCA row adder in stage " << s <<  " from col " << c << " to " << c+ci << " width " << ci+1 << endl;
                                                BasicCompressor *newCompressor = new BasicRowAdder(bitheap->getOp(), bitheap->getOp()->getTarget(), ci+1);
                                                //possibleCompressors.push_back(newCompressor);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << endl;
                                                solution.addCompressor(s, c, newCompressor, ci-2);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << " " << solution.getCompressorsAtPosition(s, c)[0].first->outHeights.size() << endl;
                                                cout << "ok" << endl;
                                                break;
                                            }
                                            case 1:{
                                                cout << "ternary row adder in stage " << s <<  " from col " << c << " to " << c+ci << " width " << ci+1 << endl;
                                                BasicCompressor *newCompressor = new BasicRowAdder(bitheap->getOp(), bitheap->getOp()->getTarget(), ci+1, 3);
                                                //possibleCompressors.push_back(newCompressor);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << endl;
                                                solution.addCompressor(s, c, newCompressor, ci-2);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << " " << solution.getCompressorsAtPosition(s, c)[0].first->outHeights.size() << endl;
                                                cout << "ok" << endl;
                                                break;
                                            }
                                            case 2:{
                                                cout << "4:2 row adder in stage " << s <<  " from col " << c << " to " << c+ci << " width " << ci+1 << endl;
                                                BasicCompressor *newCompressor = new BasicXilinxFourToTwoCompressor(bitheap->getOp(), bitheap->getOp()->getTarget(), ci+1);
                                                //possibleCompressors.push_back(newCompressor);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << endl;
                                                solution.addCompressor(s, c, newCompressor, ci-2);
                                                cout << solution.getCompressorsAtPosition(s, c).size() << " " << solution.getCompressorsAtPosition(s, c)[0].first->outHeights.size() << endl;
                                                cout << "ok" << endl;
                                                break;
                                            }
                                            default:
                                                break;
                                        }
                                        break;
                                    }
                                }
                            }
                            ci++;
                        }

                    }
                }
            }
        }
    }

#endif


}   //end namespace flopoco
