/*

TODO

*/

#ifdef HAVE_PAGLIB

#include<ScaLP/Solver.h>
#include<ScaLP/Exception.h>
#include<ScaLP/SolverGurobi.h>
#include <strstream>
#include <cmath>

#include "WordLengthCalculator.hpp"

using namespace std;
using namespace PAGSuite;

namespace flopoco {
    map<pair<int, int>, vector<int> > WordLengthCalculator::optimizeTruncation()
    {
            struct edge_info { 
                pair<adder_graph_base_node_t*, adder_graph_base_node_t*> nodes; 
                int wEdge; 
                int64_t shift; 
                int64_t constant; 
            };
            vector<edge_info> edges;

            for(auto & it : adder_graph_.nodes_list)
            {
                // TODO : add here the appropriate checks for allowed nodes
                /*
                if(is_a<adder_subtractor_node_t>(*it)) {
                    std::cout << "Node type: A\n";
                } else if(is_a<register_node_t>(*it)) {
                    std::cout << "Node type: R\n";
                } else if(is_a<output_node_t>(*it)) {
                    std::cout << "Node type: O\n";
                } else if(is_a<input_node_t>(*it)){
                    std::cout << "Node type: I\n";
                } else {
                    std::cerr << "Node type: UNRECOGNIZED\n";
                }*/

                if(is_a<adder_subtractor_node_t>(*it)) {
                    adder_subtractor_node_t* currNode = (adder_subtractor_node_t*)it;
                    for(size_t i{0u}; i < currNode->inputs.size(); ++i)
                    {
                        edges.push_back({ make_pair(currNode->inputs[i], it), 
                                          computeWordSize(currNode->inputs[i], wIn_),
                                          currNode->input_shifts[i],
                                          currNode->inputs[i]->output_factor[0][0]
                                        });
                    }
                } else if(is_a<register_node_t>(*it) || is_a<output_node_t>(*it)) {
                    register_node_t* currNode = (register_node_t*)it;
                    edges.push_back( { make_pair(currNode->input, it),
                                       computeWordSize(currNode->input, wIn_),
                                       currNode->input_shift,
                                       currNode->input->output_factor[0][0]       
                    });
                }
                
            }  

            try {
                ScaLP::Solver s = ScaLP::Solver(ScaLP::newSolverGurobi());
                s.quiet = true;

                std::vector<ScaLP::Variable> truncPosition;
                std::vector<ScaLP::Variable> truncError;
                std::vector<ScaLP::Variable> edgeError;
                std::vector<std::vector<ScaLP::Variable>> truncBits;
                truncPosition.resize(edges.size());
                truncError.resize(edges.size());
                truncBits.resize(edges.size());
                edgeError.resize(edges.size());
                for(size_t i{0u}; i < edges.size(); ++i)
                {
                    ostrstream vN;
                    vN << "epsilon_" << (int)i;
                    edgeError[i] = ScaLP::newIntegerVariable(vN.str());
                }
                for(size_t i{0u}; i < edges.size(); ++i)
                {
                    ostrstream vN1;
                    vN1 << "t_" << i;
                    truncPosition[i] = ScaLP::newIntegerVariable(vN1.str());
                    ostrstream vN2;
                    vN2 << "et_" << i;
                    truncError[i] = ScaLP::newIntegerVariable(vN2.str());
                    // add the associated constraints
                    ScaLP::Term t1(0);
                    ScaLP::Term t2(0);
                    ScaLP::Term t3(0);
                    for(int j{0}; j <= edges[i].wEdge; ++j) {
                        ostrstream vN;
                        vN << "t_" << i << "_" << j;
                        truncBits[i].push_back(ScaLP::newBinaryVariable(vN.str()));
                        t1 += truncBits[i][j];
                        t2 += j * truncBits[i][j];
                        t3 += ((int)pow(2,j)-1) * truncBits[i][j];
                    }
                    s << ( t1 == 1 );
                    s << ( t2 - truncPosition[i] == 0 );
                    s << ( t3 - truncError[i] == 0 );
                    if(is_a<input_node_t>(*edges[i].nodes.first)) {
                        s << ( edgeError[i] - truncError[i] == 0 );
                    } else if(is_a<register_node_t>(*edges[i].nodes.first)) {
                        bool inEdgeNotFound = true;
                        for(size_t j{0u}; j < edges.size() && inEdgeNotFound; ++j) {
                            if(edges[j].nodes.second == edges[i].nodes.first) {
                                inEdgeNotFound = false;
                                s << ( edgeError[i] - edgeError[j] == 0 );
                                s << ( truncError[i] - truncError[j] == 0 );
                            }
                        }
                    } else if(is_a<adder_subtractor_node_t>(*edges[i].nodes.first)) {
                        bool inEdgeNotFound = true;
                        ScaLP::Term t(0);
                        size_t secondIt{0u};
                        for(size_t j{0u}; j < edges.size() && inEdgeNotFound; ++j) {
                            if(edges[j].nodes.second == edges[i].nodes.first) {
                                inEdgeNotFound = false;
                                secondIt = j;
                                t += pow(2, edges[j].shift) * edgeError[j];
                            }
                        }
                        inEdgeNotFound = true;
                        for(size_t j{secondIt+1u}; j < edges.size() && inEdgeNotFound; ++j) {
                            if(edges[j].nodes.second == edges[i].nodes.first) {
                                inEdgeNotFound = false;
                                t += pow(2, edges[j].shift) * edgeError[j];
                            }                        
                        }
                        t += (truncError[i] - edgeError[i]);
                        s << ( t == 0 );
                    }
                    if(is_a<output_node_t>(*edges[i].nodes.second)) {
                        s << ( pow(2, edges[i].shift) * edgeError[i] <= epsilon_ );
                    }
                }

                std::vector<ScaLP::Variable> faGain;
                std::vector<ScaLP::Variable> indicVar;
                std::vector<ScaLP::Term> shiftTerms;
                size_t adderCount{0u};
                for(auto &it : adder_graph_.nodes_list) {
                    if(is_a<adder_subtractor_node_t>(*it)) {
                        ostrstream vN1, vN2; 
                        vN1 << "gain_" << adderCount;
                        vN2 << "indic_" << adderCount;
                        faGain.push_back(ScaLP::newIntegerVariable(vN1.str()));
                        indicVar.push_back(ScaLP::newBinaryVariable(vN2.str()));
                        size_t secondIt{0u};
                        bool inEdgeNotFound = true;
                        for(size_t j{0u}; j < edges.size() && inEdgeNotFound; ++j) {
                            if(edges[j].nodes.second == it) {
                                inEdgeNotFound = false;
                                secondIt = j;
                                shiftTerms.push_back(truncPosition[j]+edges[j].shift);
                            }
                        }
                        inEdgeNotFound = true;
                        for(size_t j{secondIt+1u}; j < edges.size() && inEdgeNotFound; ++j) {
                            if(edges[j].nodes.second == it) {
                                inEdgeNotFound = false;
                                shiftTerms.push_back(truncPosition[j]+edges[j].shift);
                            }                        
                        }
                        s << ( faGain[adderCount] - shiftTerms[2*adderCount] >= 0 );
                        s << ( faGain[adderCount] - shiftTerms[2*adderCount+1] >= 0 );
                        s << ( indicVar[adderCount] == 0 then faGain[adderCount] - shiftTerms[2*adderCount] == 0 );
                        s << ( indicVar[adderCount] == 1 then faGain[adderCount] - shiftTerms[2*adderCount+1] == 0 );
                        ++adderCount;
                    }
                }

                // construct the objective
                ScaLP::Term t(0);
                for(int i{0}; i < adderCount; ++i) {
                    t += faGain[i];
                }
                ScaLP::Objective o = ScaLP::maximize(t);
                s.setObjective(o);

                //s.writeLP("flopoco.lp");
                ScaLP::status stat = s.solve();
                //std::cout << "The result is " << stat << std::endl;
                if(stat == ScaLP::status::OPTIMAL || stat == ScaLP::status::FEASIBLE)
                {
                    ScaLP::Result r = s.getResult();

                    //for(std::pair<const ScaLP::Variable, double>& p : r.values)
                    //{
                    //    std::cout << p.first << " = " << p.second << std::endl;
                    //}
                    //pair - constant + stage
                    //vector - inputs in the adder / nb of bits to truncate on those
                    // parse the adder graph nodes
                    vector<size_t> adderOutEdges;
                    for(size_t i{0u}; i < edges.size(); ++i) {
                        if(is_a<adder_subtractor_node_t>(*edges[i].nodes.first)) {
                            bool notFound = true;
                            for(size_t j{0u}; j < adderOutEdges.size() && notFound; ++j)
                            {
                                if(edges[adderOutEdges[j]].nodes.first == edges[i].nodes.first)
                                    notFound = false;
                            } 
                            if(notFound)
                                adderOutEdges.push_back(i);                           
                        }

                        if(is_a<register_node_t>(*edges[i].nodes.second)) {
                            truncationVal_[make_pair(edges[i].constant, edges[i].nodes.second->stage)].push_back((int)r.values[truncPosition[i]]);
                        }
                    }

                    for(size_t i{0u}; i < adderOutEdges.size(); ++i) {
                        for(size_t j{0u}; j < edges.size(); ++j) {
                                if(is_a<adder_subtractor_node_t>(*edges[j].nodes.second))
                                    if (edges[j].nodes.second == edges[adderOutEdges[i]].nodes.first)
                                        truncationVal_[make_pair(edges[adderOutEdges[i]].constant, edges[adderOutEdges[i]].nodes.first->stage)].push_back((int)r.values[truncPosition[j]]);
                            }
                    }
                }
            }
            catch(ScaLP::Exception &e)
            {
                std::cerr << "Error: " << e << std::endl;
            }

        return truncationVal_;
    }
}

#endif