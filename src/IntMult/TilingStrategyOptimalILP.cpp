#include "IntMult/TilingStrategyOptimalILP.hpp"

#include "BaseMultiplierLUT.hpp"
#include "MultiplierTileCollection.hpp"

using namespace std;
namespace flopoco {

TilingStrategyOptimalILP::TilingStrategyOptimalILP(
		unsigned int wX_,
		unsigned int wY_,
		unsigned int wOut_,
		bool signedIO_,
		BaseMultiplierCollection* bmc,
		base_multiplier_id_t prefered_multiplier,
		float occupation_threshold,
		size_t maxPrefMult,
        MultiplierTileCollection mtc_):TilingStrategy(
			wX_,
			wY_,
			wOut_,
			signedIO_,
			bmc),
		small_tile_mult_{1}, //Most compact LUT-Based multiplier
		numUsedMults_{0},
		max_pref_mult_ {maxPrefMult},
		occupation_threshold_{occupation_threshold},
		tiles{mtc_.MultTileCollection}
	{

	}

void TilingStrategyOptimalILP::solve()
{
/*
    BaseMultiplierCategory* value = MultiplierTileCollection::superTileSubtitution(tiles, 7,  0, 23, 23,  0, 24, 16, 47);
    if(value != nullptr)
        cout << value->getType() << endl;
    cout << value->getParametrisation().getMultType() << endl;
    cout << value->efficiency() << endl;
    cout << value->getArea() << endl;

    exit(1);

*/


#ifndef HAVE_SCALP
    throw "Error, TilingStrategyOptimalILP::solve() was called but FloPoCo was not built with ScaLP library";
#else
    solver = new ScaLP::Solver(ScaLP::newSolverDynamic({"Gurobi","CPLEX","SCIP","LPSolve"}));
	constructProblem();

    // Try to solve
    cout << "starting solver, this might take a while..." << endl;
    ScaLP::status stat = solver->solve();

    // print results
    cerr << "The result is " << stat << endl;
    //cerr << solver->getResult() << endl;
    ScaLP::Result res = solver->getResult();

    double total_cost = 0;
    for(auto &p:res.values)
    {
        if(p.second > 0.5){     //parametrize all multipliers at a certain position, for which the solver returned 1 as solution, to flopoco solution structure
            std::string var_name = p.first->getName();
            cout << "is true:  " << var_name.substr(2,dpS) << " " << var_name.substr(2+dpS,dpX) << " " << var_name.substr(2+dpS+dpX,dpY) << std::endl;
            int mult_id = stoi(var_name.substr(2,dpS));
            int x_negative = (var_name.substr(2+dpS,1).compare("m") == 0)?1:0;
            int m_x_pos = stoi(var_name.substr(2+dpS+x_negative,dpX)) * ((x_negative)?(-1):1);
            int y_negative = (var_name.substr(2+dpS+x_negative+dpX,1).compare("m") == 0)?1:0;
            int m_y_pos = stoi(var_name.substr(2+dpS+dpX+x_negative+y_negative,dpY)) * ((y_negative)?(-1):1);

            total_cost += (double)tiles[mult_id]->cost();
            auto coord = make_pair(m_x_pos, m_y_pos);
            solution.push_back(make_pair(tiles[mult_id]->getParametrisation().tryDSPExpand(m_x_pos, m_y_pos, wX, wY, signedIO), coord));

        }
    }
    cout << "Total cost:" << total_cost <<std::endl;

#endif
}

#ifdef HAVE_SCALP
void TilingStrategyOptimalILP::constructProblem()
{
    cout << "constructing problem formulation..." << endl;
    wS = tiles.size();


    //Assemble cost function, declare problem variables
    cout << "   assembling cost function, declaring problem variables..." << endl;
    ScaLP::Term obj;
    int x_neg = 0, y_neg = 0;
    for(int s = 0; s < wS; s++){
        x_neg = (x_neg < (int)tiles[s]->wX())?tiles[s]->wX() - 1:x_neg;
        y_neg = (y_neg < (int)tiles[s]->wY())?tiles[s]->wY() - 1:y_neg;
    }
    int nx = wX-1, ny = wY-1, ns = wS-1; dpX = 1; dpY = 1; dpS = 1; //calc number of decimal places, for var names
    nx = (x_neg > nx)?x_neg:nx;                                     //in case the extend in negative direction is larger
    ny = (y_neg > ny)?y_neg:ny;
    while (nx /= 10)
        dpX++;
    while (ny /= 10)
        dpY++;
    while (ns /= 10)
        dpS++;

    vector<vector<vector<ScaLP::Variable>>> solve_Vars(wS, vector<vector<ScaLP::Variable>>(wX+x_neg, vector<ScaLP::Variable>(wY+y_neg)));
    // add the Constraints
    cout << "   adding the constraints to problem formulation..." << endl;
    for(int y = 0; y < wY; y++){
        for(int x = 0; x < wX; x++){
            stringstream consName;
            consName << "p" << setfill('0') << setw(dpX) << x << setfill('0') << setw(dpY) << y;            //one constraint for every position in the area to be tiled
            ScaLP::Term pxyTerm;
            for(int s = 0; s < wS; s++){					//for every available tile...
                for(int ys = 0 - tiles[s]->wY() + 1; ys <= y; ys++){					//...check if the position x,y gets covered by tile s located at position (xs, ys) = (x-wtile..x, y-htile..y)
                    for(int xs = 0 - tiles[s]->wX() + 1; xs <= x; xs++){
                        if(occupation_threshold_ == 1.0 && ((wX - xs) < (int)tiles[s]->wX() || (wY - ys) < (int)tiles[s]->wY())) break;
                        if(tiles[s]->shape_contribution(x, y, xs, ys, wX, wY, signedIO) == true){
                            if(tiles[s]->shape_utilisation(xs, ys, wX, wY, signedIO) >=  occupation_threshold_ ){
                                if(solve_Vars[s][xs+x_neg][ys+y_neg] == nullptr){
                                    stringstream nvarName;
                                    nvarName << " d" << setfill('0') << setw(dpS) << s << ((xs < 0)?"m":"") << setfill('0') << setw(dpX) << ((xs<0)?-xs:xs) << ((ys < 0)?"m":"")<< setfill('0') << setw(dpY) << ((ys<0)?-ys:ys) ;
                                    //std::cout << nvarName.str() << endl;
                                    ScaLP::Variable tempV = ScaLP::newBinaryVariable(nvarName.str());
                                    solve_Vars[s][xs+x_neg][ys+y_neg] = tempV;
                                    obj.add(tempV, (double)tiles[s]->cost());    //append variable to cost function
                                }
                                pxyTerm.add(solve_Vars[s][xs+x_neg][ys+y_neg], 1);
                            }
                        }
                    }
                }
            }
            ScaLP::Constraint c1Constraint = pxyTerm == (bool)1;
            c1Constraint.name = consName.str();
            solver->addConstraint(c1Constraint);
        }
    }

    //limit use of shape n
    cout << "   adding the constraint to limit the use of DSP-Blocks to " << max_pref_mult_ << " instances..." << endl;
    int sn = 0;
    stringstream consName;
    consName << "lims" << sn;
    ScaLP::Term pxyTerm;
    for (int y = 0 - 24 + 1; y < wY; y++) {
        for (int x = 0 - 24 + 1; x < wX; x++) {
            if (solve_Vars[0][x + x_neg][y + y_neg] != nullptr)                     //build a loop that checks for DSPcost > 0 to realize a more generic realisation that covers DSPSuperBlocks, when the current behaviour to limit DSP blocks is no longer needed for debugging purposes
                pxyTerm.add(solve_Vars[0][x + x_neg][y + y_neg], 1);
            if (solve_Vars[1][x + x_neg][y + y_neg] != nullptr)
                pxyTerm.add(solve_Vars[1][x + x_neg][y + y_neg], 1);
        }
    }
    ScaLP::Constraint c1Constraint = pxyTerm <= max_pref_mult_;     //set max usage equ.
    c1Constraint.name = consName.str();
    solver->addConstraint(c1Constraint);

    // Set the Objective
    cout << "   setting objective (minimize cost function)..." << endl;
    solver->setObjective(ScaLP::minimize(obj));

    // Write Linear Program to file for debug purposes
    cout << "   writing LP-file for debuging..." << endl;
    solver->writeLP("tile.lp");
}


#endif

}   //end namespace flopoco
