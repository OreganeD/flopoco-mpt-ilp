/*
  A class to manage heaps of weighted bits in FloPoCo
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2012.
  All rights reserved.

*/
#ifndef __WEIGHTEDBIT_HPP
#define __WEIGHTEDBIT_HPP
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>	

//#include "Plotter.hpp"



namespace flopoco{



	class WeightedBit
		{
		public:
			/** constructor 
			    @param bh the parent bit heap */ 
			WeightedBit(int guid, int uid, int weight, int type, int cycle,  double criticalPath);

			/** destructor */ 
			~WeightedBit(){};
		

			/** return the cycle at which this bit is defined */
			int getCycle(){
				return cycle;
			};

			/** return the critical path of this bit */
			double getCriticalPath(int atCycle);

			/** returns the stage when this bit should be compressed */ 
			int computeStage(int stagesPerCycle, double elementaryTime);


			

			/** return the VHDL signal name of this bit */
			std::string getName(){
				return name;
			};

		
			/** ordering by availability time */
			bool operator< (const WeightedBit& b); 
			/** ordering by availability time */
			bool operator<= (const WeightedBit& b); 

			//void removePlottableBit();


		private:
			int cycle;  /**< The cycle at which the bit was created */
			double criticalPath; /**< The cycle at which the bit was created */

			int weight;
			int type;
			int uid;
			std::string name;
			std::string srcFileName;
			std::string uniqueName_;
			

		};

}
#endif