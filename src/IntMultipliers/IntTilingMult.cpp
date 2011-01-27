/*
  Tilling Multiplier for FloPoCo
 
  Authors:  Sebastian Banescu, Radu Tudoran, Bogdan Pasca
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  CeCILL license, 2008-2010.

 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <math.h>
#include <string.h>
#include <limits.h>

#include <gmp.h>


#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "../IntMultiplier.hpp"
#include "IntTilingMult.hpp"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <math.h>
#include <locale>
#include <cfloat>

#include <stdio.h>
#include <mpfr.h>

using namespace std;

namespace flopoco{

	extern vector<Operator*> oplist;

#define DEBUGVHDL 0

	
	IntTilingMult:: IntTilingMult(Target* target, int wInX, int wInY, float ratio, int maxTimeInMinutes, bool interactive) :
		Operator(target), wInX(wInX), wInY(wInY), wOut(wInX + wInY),ratio(ratio), maxTimeInMinutes(maxTimeInMinutes-1){
 
		ostringstream name;
			srcFileName="IntTilingMultiplier";
			
			start = clock(); /* time management */
			
			name <<"IntMultiplier_"<<wInX<<"_"<<wInY<<"_uid"<<getNewUId();
			setName(name.str());
		
			setCopyrightString("Sebastian Banescu, Radu Tudoran, Bogdan Pasca 2009-2010");
	
			addInput ("X", wInX);
			addInput ("Y", wInY);
			addOutput("R", wOut); /* wOut = wInX + wInY */
		
			warningInfo();
		
			/* the board is extended horizontally and vertically. This is done
			in order to fit DSPs on the edges. */
			vn = wInX + 2*getExtraWidth(); 
			vm = wInY + 2*getExtraHeight();
			
			/* one of the important optimizations consist in removing the extra
			stripes on on top and on the bottom */	
			vnme = vn - getExtraWidth();		
			vmme = vm - getExtraHeight();
			

			
			REPORT( INFO, "Input board size is width="<<wInX<<" height="<<wInY);
			REPORT( INFO, "Extra width:="<<getExtraWidth()<<" extra height:="<<getExtraHeight());
			REPORT( INFO, "Extended board width="<<vnme<<" height="<<vmme);

			/* detailed info about the abstracted DSP block */
			int x,y;
			target->getDSPWidths(x,y);
			REPORT( INFO, "DSP block: width= "<< x <<" height=" << y);

			/* get an estimated number of DSPs needed to tile the board with */
			nrDSPs = estimateDSPs();
			REPORT( INFO, "Estimated DSPs = " <<nrDSPs);

			/* the maximum number of DSPs that can be chained on Virtex devices.
			The number is related with the number of register levels inside the
			DSPs. */
			if ( ( target_->getID() == "Virtex4") ||
			 ( target_->getID() == "Spartan3"))  // then the target is A Xilinx FPGA 
				nrOfShifts4Virtex=int(sqrt(nrDSPs));			
			else
				nrOfShifts4Virtex=20;
			
			//~ float tempDist =	 (movePercentage  * getExtraWidth() * getExtraWidth()) /4.0 + (movePercentage *getExtraHeight() * getExtraHeight()) /4.0;
			/* each time a tile is placed maxDist2Move is the maximum distance
			it may be placed from the already placed blocks */
			float tempDist =	0;
			maxDist2Move = (int) ( sqrt(tempDist) );
			REPORT( INFO, "maxDist2Move = "<<maxDist2Move);
		
			float const scale=100.0;
			costDSP = ( (1.0+scale) - scale * ratio );
			//~ cout<<"Cost DSP is "<<costDSP<<endl;
			costLUT = ( (1.0+scale) - scale * (1-ratio) ) /  ((float) target_->getEquivalenceSliceDSP() );
			//~ cout<<"Cost LUT is "<<costLUT<<endl;
		
		
			//the one
			
			if (interactive){
				cout << " DO you want to run the algorithm? (y/n)" << endl;
				string myc;
				cin >> myc;
				if ( myc.compare("y")!=0)
					exit(-1);
			}						
			runAlgorithm();		
			
			
			
			
			
			
		
			REPORT(INFO, "Estimated DSPs:= "<<nrDSPs);
			target->getDSPWidths(x,y);
			REPORT(INFO,"Width of DSP is := "<<x<<" Height of DSP is:="<<y);
			REPORT(INFO,"Extra width:= "<<getExtraWidth()<<" \nExtra height:="<<getExtraHeight());
			REPORT(INFO,"maxDist2Move:= "<<maxDist2Move);
	
		
			generateVHDLCode4CompleteTilling();
		
		
		
		
		
		/*
		int n=vn;
			int m=vm;
			int exw=getExtraWidth();
			int exh=getExtraHeight();
	
			mat = new int*[m];
			for(int i=0;i<m;i++)
			{
				mat[i] = new int [n];
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
			
			
			int w, h;
			target->getDSPWidths(w, h);
			
			cout<<w<<" "<<h<<endl;
			int shift = 17;
			
			
			nrDSPs=2;
			globalConfig = new DSP*[nrDSPs];
			globalConfig[0] = new DSP(shift, w, h);	
			globalConfig[0]->setNrOfPrimitiveDSPs(1);
			globalConfig[0]->rotate();
			globalConfig[0]->setTopRightCorner(exw, exh);
			globalConfig[0]->setBottomLeftCorner(exw+h-1, exh+w-1);
			
			
			globalConfig[1] = new DSP(shift, w, h);	
			globalConfig[1]->setNrOfPrimitiveDSPs(1);			
			//globalConfig[1]->rotate();
			globalConfig[1]->setTopRightCorner(exw , exh +h   );
			globalConfig[1]->setBottomLeftCorner(exw + w-1, exh +2*h-1);
			
			display(globalConfig);
			bindDSPs(globalConfig);
			*/
		
		
		//~ //globalConfig[0] = target->createDSP();
		//~ globalConfig[0]->setTopRightCorner(9,6);
		//~ globalConfig[0]->setBottomLeftCorner(32,39);
		
		//~ //globalConfig[1] = target->createDSP();
		//~ globalConfig[1]->rotate();
		//~ globalConfig[1]->setTopRightCorner(33,6);
		//~ globalConfig[1]->setBottomLeftCorner(66,29);
		
		//~ //globalConfig[2] = target->createDSP();
		
		//~ globalConfig[2]->setTopRightCorner(43,30);
		//~ globalConfig[2]->setBottomLeftCorner(66,63);
		
		//~ //globalConfig[3] = target->createDSP();
		//~ globalConfig[3]->rotate();
		//~ globalConfig[3]->setTopRightCorner(9,40);
		//~ globalConfig[3]->setBottomLeftCorner(42,63);
		
		
		//~ display(globalConfig);
		
		//~ compareCost();
		
		//~ display(bestConfig);
		
		//~ cout<<"Best cost is "<<bestCost<<endl;
		
		//~ globalConfig[1]->rotate();
		//~ replace(globalConfig,1);
		//~ display(globalConfig);
		
		//~ //START SIMULATED ANNEALING -------------
		//~ counterfirst =0 ;
		//~ int n,m;

		//~ n=vn;
		//~ m=vm;
	
		//~ mat = new int*[m];
		//~ for(int i=0;i<m;i++)
			//~ {
				//~ mat[i] = new int[n];
				//~ for(int j=0;j<n;j++)
					//~ mat[i][j]=0;
			//~ }
		
		//~ tempc= new DSP*[nrDSPs];
		//~ for(int ii=0;ii<nrDSPs;ii++)
			//~ tempc[ii]= new DSP();

		//~ /*we will try the algorithm with 2 values of nrDSPs	
		  //~ One will be the estimated value(nrDSPs) and the second one will be nrDSPs-1	
		//~ */
		//~ rot = new bool[nrDSPs];
		//~ for(int i =0;i<nrDSPs;i++)
			//~ rot[i]=false;
	
		//~ simulatedAnnealing();
		
		
		
		
		//~ move(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ replace(globalConfig,3);
		//~ display(globalConfig);Pas
		
		
	 
		//~ globalConfig[0]->setTopRightCorner(19,0);
		//~ globalConfig[0]->setBottomLeftCorner(35,16);
		//~ replace(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ display(globalConfig);
		//~ move(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ display(globalConfig);
		
		//~ do{
			
			//~ replace(globalConfig,1);
		//~ do{
			//~ replace(globalConfig,2);
		//~ while(move(globalConfig,2))
		//~ {
			//~ compareCost();
		//~ }
		//~ }while(move(globalConfig,1));
		
		//~ display(globalConfig);
		//~ }while(move(globalConfig,0));
		
		//~ display(bestConfig);
		
		
		//~ move(globalConfig,0);
		//~ replace(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ display(globalConfig);
		//~ move(globalConfig,1);
		//~ move(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ display(globalConfig);
		//~ move(globalConfig,2);
		//~ move(globalConfig,2);
		//~ display(globalConfig);
		
		//~ while(move(globalConfig,2));
		//~ display(globalConfig);
		//~ move(globalConfig,2);
		//~ display(globalConfig);
	 
		//~ ///experiment(bogdan)  ./flopoco -target=Virtex5 IntTilingMult 58 58 0.44
		//~ tempc= new DSP*[nrDSPs];
		//~ for(int ii=0;ii<nrDSPs;ii++)
		//~ tempc[ii]= new DSP();
		//~ int n,m;
		//~ int count=1;
	
		//~ n=vn;
		//~ m=vm;
		//~ mat = new int*[m];
		//~ for(int i=0;i<m;i++)
		//~ {
		//~ mat[i] = new int [n];
		//~ for(int j=0;j<n;j++)
		//~ mat[i][j]=0;
		//~ }
	
	
	
		//~ initTiling(bestConfig,nrDSPs);
		//~ initTiling(globalConfig,nrDSPs);
	
	
		//Test The Configuration
		//~ globalConfig[0] = target->createDSP();
		//~ globalConfig[0]->setTopRightCorner(2,2);
		//~ globalConfig[0]->setBottomLeftCorner(25,18);
		//~ globalConfig[1] = target->createDSP();
		//~ globalConfig[1]->setTopRightCorner(2,19);
		//~ globalConfig[1]->setBottomLeftCorner(25,35);
		//~ globalConfig[2] = target->createDSP();
		//~ globalConfig[2]->rotate();
		//~ globalConfig[2]->setTopRightCorner(2,36);
		//~ globalConfig[2]->setBottomLeftCorner(18,59);
		//~ globalConfig[3] = target->createDSP();
		//~ globalConfig[3]->rotate();
		//~ globalConfig[3]->setTopRightCorner(19,36);
		//~ globalConfig[3]->setBottomLeftCorner(35,59);
		
		//~ globalConfig[4] = target->createDSP();
		//~ globalConfig[4]->rotate();
		//~ globalConfig[4]->setTopRightCorner(26,2);
		//~ globalConfig[4]->setBottomLeftCorner(42,25);
		//~ globalConfig[5] = target->createDSP();
		//~ globalConfig[5]->rotate();
		//~ globalConfig[5]->setTopRightCorner(43,2);
		//~ globalConfig[5]->setBottomLeftCorner(59,25);
		//~ globalConfig[6] = target->createDSP();
		//~ globalConfig[6]->setTopRightCorner(36,26);
		//~ globalConfig[6]->setBottomLeftCorner(59,42);
		//~ globalConfig[7] = target->createDSP();
		//~ globalConfig[7]->setTopRightCorner(36,43);
		//~ globalConfig[7]->setBottomLeftCorner(59,59);
		
		//~ display(globalConfig);
		//~ cout<<endl<<"This is the target configuration"<<endl;
		//~ compareCost();
		//~ cout<<"The best score is "<<bestCost<<endl;
		//~ display(bestConfig);
	
	
	
		//~ bestCost = 333222;
		//~ compareCost();
	 
		//~ display(bestConfig);
	
		//~ initTiling(globalConfig,nrDSPs);
	
		 //~ globalConfig[0]->setTopRightCorner(7,5);
		 //~ globalConfig[0]->setBottomLeftCorner(23,28);
		 //~ replace(globalConfig,1);
		 //~ replace(globalConfig,2);
		 //~ replace(globalConfig,3);
		 //~ display(globalConfig);
		 
		//~ globalConfig[1]->setTopRightCorner(24,5);
		//~ globalConfig[1]->setBottomLeftCorner(40,28);
		//~ globalConfig[2]->setTopRightCorner(41,5);
		//~ globalConfig[2]->setBottomLeftCorner(64,21);
		//~ globalConfig[3]->setTopRightCorner(41,22);
		//~ globalConfig[3]->setBottomLeftCorner(64,38);
		//~ globalConfig[4]->setTopRightCorner(7,29);
		//~ globalConfig[4]->setBottomLeftCorner(30,45);
		//~ globalConfig[5]->setTopRightCorner(7,46);
		//~ globalConfig[5]->setBottomLeftCorner(30,62);
		//~ globalConfig[6]->setTopRightCorner(31,39);
		//~ globalConfig[6]->setBottomLeftCorner(47,62);
		//~ globalConfig[7]->setTopRightCorner(48,39);
		//~ globalConfig[7]->setBottomLeftCorner(64,62);
	
		//~ display(globalConfig);
	
	
		//~ cout<<checkFarness(globalConfig,2);
		//~ move(globalConfig,1);
		//~ display(globalConfig);
	
		//~ compareCost();
	
		//experiment pentru ./flopoco -target=Virtex5 IntTilingMult 18 34 0.35
		//~ tempc= new DSP*[nrDSPs];
		//~ for(int ii=0;ii<nrDSPs;ii++)
		//~ tempc[ii]= new DSP();
		//~ int n,m;
		//~ int count=1;
	
		//~ n=vn;
		//~ m=vm;
		//~ mat = new int*[m];
		//~ for(int i=0;i<m;i++)
		//~ {
		//~ mat[i] = new int [n];
		//~ for(int j=0;j<n;j++)
		//~ mat[i][j]=0;
		//~ }
	
	
	
		//~ initTiling(bestConfig,nrDSPs);
		//~ initTiling(globalConfig,nrDSPs);
	
		//~ globalConfig[0]->setTopRightCorner(6,4);
		//~ globalConfig[0]->setBottomLeftCorner(22,27);
		//~ display(globalConfig);
	
		//~ bestCost = 333222;
		//~ compareCost();
	
	
	
		//~ initTiling(globalConfig,nrDSPs);
	
		//~ globalConfig[0]->setTopRightCorner(4,4);
		//~ globalConfig[0]->setBottomLeftCorner(20,27);
		//~ display(globalConfig);
	
	
	
		//~ compareCost();
	 
		///////////////////////////////////////////
	
	
		//~ globalConfig[1]->setTopRightCorner(1,17);
		//~ globalConfig[1]->setBottomLeftCorner(17,33);
		//~ globalConfig[2]->setTopRightCorner(21,-12);
		//~ globalConfig[2]->setBottomLeftCorner(37,4);

		//~ display(globalConfig);
	
		//~ maxDist2Move=2;
	
		//~ //cout<<endl<<checkFarness(globalConfig,2) <<endl;
	
		//~ //cout<<move(globalConfig,2)<<endl;
		//~ cout<<move(globalConfig,0)<<endl;
		//~ display(globalConfig);
		//~ replace(globalConfig,1);
		//~ replace(globalConfig,2);
		//~ display(globalConfig);
		//~ cout<<move(globalConfig,2)<<endl;
	
		//~ display(globalConfig);
	
	



		//~ initTiling(bestConfig,nrDSPs);
		//~ bestCost=367.455;
	
		//~ initTiling(globalConfig,nrDSPs);
	
		//~ globalConfig[0]->setTopRightCorner(1,2);
		//~ globalConfig[0]->setBottomLeftCorner(9,10);
		//~ globalConfig[1]->setTopRightCorner(9,13);
		//~ globalConfig[1]->setBottomLeftCorner(17,21);
		//~ globalConfig[2]->setTopRightCorner(18,4);
		//~ globalConfig[2]->setBottomLeftCorner(26,12);
	
	
		//~ cout<<"Initial configuration"<<endl<<endl;
		//~ display(globalConfig);
	
		//~ maxDist2Move=2;
	
		//~ cout<<endl<<checkFarness(globalConfig,1) <<endl;
	
		//~ compareCost();
	
	
		
	
		
		//~ initTiling(globalConfig,nrDSPs);
	
		//~ globalConfig[0]->setTopRightCorner(2,15);
		//~ globalConfig[0]->setBottomLeftCorner(18,31);
		//~ globalConfig[1]->setTopRightCorner(19,15);
		//~ globalConfig[1]->setBottomLeftCorner(35,31);
	
		//~ int t;
		//~ cout<<"cost of partitions is "<<partitionOfGridSlices(globalConfig,t);
		//~ cout<<"Cost of obtained best is "<<computeCost(globalConfig)<<endl;
		//~ display(globalConfig);
	
		//~ cout<<endl<<endl;
	
		//~ initTiling(globalConfig,nrDSPs);
	
		//~ globalConfig[0]->setTopRightCorner(2,14);
		//~ globalConfig[0]->setBottomLeftCorner(18,30);
		//~ globalConfig[1]->setTopRightCorner(19,14);
		//~ globalConfig[1]->setBottomLeftCorner(35,30);
	
		//~ cout<<"cost of partitions is "<<partitionOfGridSlices(globalConfig,t);
		//~ cout<<"Cost of obtained best is "<<computeCost(globalConfig)<<endl;
		//~ display(globalConfig);
		
		
		

		/*
	
	
		  initTiling(globalConfig, 4);
		  for (int i=0; i<19; i++)
		  move(globalConfig, 2);
		  //replace(globalConfig, 1);
		  //move(globalConfig, 0);
		  //int x, y;
	
		  for (int i=0; i<4; i++)
		  {
		  globalConfig[i]->getTopRightCorner(x,y);
		  cout << "DSP block #" << i << " is positioned at top-right: (" << x << ", " << y << ")";
		  globalConfig[i]->getBottomLeftCorner(x,y);
		  cout << "and bottom-left: (" << x << ", " << y << ")" << endl;
		  }
		*/  	

	}
	
	IntTilingMult::~IntTilingMult() {
	}



	
	void IntTilingMult::generateVHDLCode4CompleteTilling(){
		
//		bestConfig = splitLargeBlocks(bestConfig, nrDSPs);
		bindDSPs(bestConfig);      
		int nrDSPOperands = multiplicationInDSPs(bestConfig);
		int nrSliceOperands = multiplicationInSlices(bestConfig);
		REPORT(DETAILED, "nr of DSP operands="<<nrDSPOperands);
		REPORT(DETAILED, "nr of softDSP operands="<<nrSliceOperands);
		map<string, double> inMap;
		
		for (int j=0; j<nrDSPOperands; j++)
			{
				ostringstream concatPartialProd;
				concatPartialProd  << "addOpDSP" << j;
				syncCycleFromSignal(concatPartialProd.str());
			}	
			
		for (int j=0; j<nrSliceOperands; j++)
			{
				ostringstream concatPartialProd;
				concatPartialProd  << "addOpSlice" << j;
				syncCycleFromSignal(concatPartialProd.str());
			}
		nextCycle();/////////////////////////////////////////////////////////	

		if (nrDSPOperands+nrSliceOperands>1){
			Operator* add;
//			if ( ( target_->getID() == "Virtex4") ||
//				 ( target_->getID() == "Spartan3"))  // then the target is A Xilinx FPGA 
//				add =  new IntNAdder(getTarget(), wInX+wInY, nrDSPOperands+nrSliceOperands, inMap);
//			else
				add =  new IntCompressorTree(getTarget(), wInX+wInY, nrDSPOperands+nrSliceOperands,inMap);
			REPORT(DEBUG, "Finished generating the compressor tree");
		
			oplist.push_back(add);


			for (int j=0; j<nrDSPOperands; j++)
				{
					ostringstream concatPartialProd;
					concatPartialProd  << "addOpDSP" << j;

					inPortMap (add, join("X",j) , concatPartialProd.str());
				}	
			REPORT(DEBUG, "Finished mapping the DSP Operands");
					
			for (int j=0; j<nrSliceOperands; j++)
				{
					ostringstream concatPartialProd;
					concatPartialProd  << "addOpSlice" << j;
					REPORT(DETAILED, "@ In Port Map Current Cycle is " << getCurrentCycle());
					inPortMap (add, join("X",j+nrDSPOperands) , concatPartialProd.str());
				}	
			REPORT(DEBUG, "Finished mapping the SLICE Operands");


//			if ( ( target_->getID() == "Virtex4") ||
//				 ( target_->getID() == "Spartan3"))  // then the target is A Xilinx FPGA 
//				inPortMapCst(add, "Cin", "'0'");
			outPortMap(add, "R", "addRes");
			vhdl << instance(add, "adder");

			syncCycleFromSignal("addRes");
			setCriticalPath( add->getOutputDelay("R") );
			outDelayMap["R"] = getCriticalPath();
			vhdl << tab << "R <= addRes;" << endl;
		}else{
			if (nrDSPOperands==1){
				syncCycleFromSignal("addOpDSP0");
				vhdl << tab << "R <= addOpDSP0;" << endl;
			}else{
				syncCycleFromSignal("addOpSlice0");
				vhdl << tab << "R <= addOpSlice0;" << endl;
			}
		}
			
	}
	
	
	

	void IntTilingMult::runAlgorithm()
	{
		
		int n,m;

		//~ n=wInX + 2* getExtraWidth();
		//~ m=wInY + 2* getExtraHeight();

		n=vn;
		m=vm;
	
		mat = new int*[m];
		for(int i=0;i<m;i++)
			{
				mat[i] = new int [n];
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
		
		//if the estimated number of DSPs is grather then 0 then we should run the algorithm
		if(nrDSPs>0){
			tempc= new DSP*[nrDSPs];
			for(int ii=0;ii<nrDSPs;ii++)
				tempc[ii]= new DSP();
			/*we will try the algorithm with 2 values of nrDSPs	
			One will be the estimated value(nrDSPs) and the second one will be nrDSPs-1	
			*/
			rot = new bool[nrDSPs];
			for(int i =0;i<nrDSPs;i++)
				rot[i]=false;
		
				//The second
				numberDSP4Overlap=nrDSPs;
				initTiling(globalConfig,nrDSPs);
		
		
	
			
		
			
		//this will initialize the bestConfig with the first configuration
		bestCost = FLT_MAX ;
		REPORT(INFO, "Max score is"<<bestCost);
		//bestConfig = (DSP**)malloc(nrDSPs * sizeof(DSP*));
		bestConfig = new DSP*[nrDSPs];
		for(int i=0;i<nrDSPs;i++)
			bestConfig[i]= new DSP();
		compareCost();
		//cout<<"New best score is: " << bestCost << endl;
	
		//display(bestConfig);
	
	
		//the best configuration should be consider initially the first one. So the bestConfig parameter will be initialized with global config and hence the bestCost will be initialized with the first cost
	
	
		//the one
		numberDSP4Overlap=nrDSPs;
		tilingAlgorithm(nrDSPs-1,nrDSPs-1,false,nrDSPs-1);
		bindDSPs(bestConfig);
		
		
		display(bestConfig);
		REPORT(INFO, "Best cost is "<<bestCost);
		
		
		/*
		globalConfig[2]->setTopRightCorner(2,26);
		globalConfig[2]->setBottomLeftCorner(25,59);
		globalConfig[1]->setTopRightCorner(36,2);
		globalConfig[1]->setBottomLeftCorner(59,35);
		globalConfig[3]->setTopRightCorner(26,36);
		globalConfig[3]->setBottomLeftCorner(59,59);
		bestCost = computeCost(globalConfig);
		display(globalConfig);
		*/
		
	
	
	
	

		//~ // After all configurations with the nrDSPs number of DSPs were evaluated then a new search is carryed with one DSP less
		//~ // After the initialization of the new configuration with nrDSPs-1, the cost must be evaluated and confrunted with the best score obtained so far.
	
		//~ if(nrDSPs-1>0)
		//~ {
		
	
		//~ for(int i =0;i<nrDSPs;i++)
		//~ rot[i]=false;
		
		//~ initTiling(globalConfig,nrDSPs -1);	
		//~ compareCost();
		//~ tilingAlgorithm(nrDSPs-2,nrDSPs-2,false);
		//~ }	
	
	
	
	
		//dealocari
	
		//~ for(int ii=0;ii<m;ii++)
			//~ delete[](mat[ii]);
	
		//~ delete[] (mat);
	
		//~ for(int ii=0;ii<nrDSPs;ii++)
			//~ free(tempc[ii]);
	
		//~ delete[] (tempc);
		
		}
		else
		{
			n=vn;
			m=vm;
	
			mat = new int*[m];
			for(int i=0;i<m;i++)
			{
				mat[i] = new int [n];
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
			tempc= new DSP*[0];
			bestConfig = new DSP*[1];
			globalConfig = new DSP*[1];
			tempc[0]=bestConfig[0]=globalConfig[0]=NULL;
			
			bestCost = FLT_MAX ;
			REPORT(INFO, "Max score is "<<bestCost);
			compareCost();
			REPORT(INFO, "New best score is "<<bestCost);
			display(bestConfig);
		}
	
	}

	
	/** The movement of the DSP blocks with values belonging to their widths and heights still needs to be done. Now it only runs with one type of move on one of the directions, which is not ok for the cases when the DSPs are not squares.
	 */
	void IntTilingMult::tilingAlgorithm(int i, int n,bool repl,int lastMovedDSP)
	{
		if(i==n)
			{
				
				if(repl==true) // if previous DSPs were moved this one needs to recompute all positions 
					{
						//cout<<" Pas 4_1 "<<i<<endl;
						if(replace(globalConfig,i)) // repostioned the DSP
							{
						//		cout<<"Pas 4_0_1 "<<i<<endl;
								compareCost();
								//display(globalConfig);
								finish = (clock() - start)/(CLOCKS_PER_SEC*60);
								if (finish > maxTimeInMinutes)
									return;
								rot[i]=false;
								tilingAlgorithm(i,n,false,lastMovedDSP);
								
							}
						else // could not reposition the DSP in the bounds of the tiling board
							{
						//		cout<<"Pas 4_5_1 "<<i<<endl;
								rot[i]=false;
								if( lastMovedDSP>=0) // go one level up the backtracking stack
								{	
									finish = (clock() - start)/(CLOCKS_PER_SEC*60);
									if (finish > maxTimeInMinutes)
										return;
									tilingAlgorithm(lastMovedDSP,n,false, lastMovedDSP);
									
								}
							}
					}
				else // the last DSP is being moved on the tiling board
					{
						//	cout<<"Pas __1 "<<i<<endl;
						if(move(globalConfig,i)) // successfuly moved the last block
							{
								//cout<<" Pas 1_1 "<<i<<endl;
								compareCost();
								finish = (clock() - start)/(CLOCKS_PER_SEC*60);
								if (finish > maxTimeInMinutes)
									return;
								tilingAlgorithm(i,n,repl,i);		//repl should be false
								
							}
						//~ else
						//~ if(move(globalConfig,i,DSPh,DSPw))
						//~ {
						//~ cout<<" Pas 1_1 "<<i<<endl;
						//~ compareCost();
						//~ tilingAlgorithm(i,n,repl,i);		//repl should be false
						//~ }
 						else // could not find a position for the last block
							{
								if(rot[i]==false && (globalConfig[i]->getMaxMultiplierWidth() != globalConfig[i]->getMaxMultiplierHeight() ))
									{ // if the DSP was not rotated and is not sqare then roteate it
										//display(globalConfig);
										//~ cout<<" Pas 2_1 "<<i<<endl;
										globalConfig[i]->rotate();
										//display(globalConfig);
										rot[i]=true;
										if(replace(globalConfig,i)) // the DSP could be repositioned
											{
												//display(globalConfig);
												compareCost();
												finish = (clock() - start)/(CLOCKS_PER_SEC*60);
												if (finish > maxTimeInMinutes)
													return;
												tilingAlgorithm(i,n,repl,i);		//repl should be false
												
											}
										else // go to the previous block 
											{
												if(i-1>=0)
												{	
													finish = (clock() - start)/(CLOCKS_PER_SEC*60);
													if (finish > maxTimeInMinutes)
														return;
													tilingAlgorithm(i-1,n,false,i);
													
												}
											}
									}
								else // the DSP was either rotated already or is square
									{
										//~ cout<<" Pas 3_1 "<<i<<endl;
										if(i-1>=0)
										{	
											finish = (clock() - start)/(CLOCKS_PER_SEC*60);
											if (finish > maxTimeInMinutes)
												return;
											tilingAlgorithm(i-1,n,repl,i);		//repl should be false
											
										}
									}
							}
					}
			}
		else // we are not at the last DSP
			{
				if(repl==true) // the previuos DSPs were successfuly repositioned
					{
					//	cout<<" Pas 4_2 "<<i<<endl;
						if(replace(globalConfig,i)) // the current DSP was successfuly repositioned
							{
								finish = (clock() - start)/(CLOCKS_PER_SEC*60);
								if (finish > maxTimeInMinutes)
										return;
								rot[i]=false;
								tilingAlgorithm(i+1,n,repl, lastMovedDSP);
							}
						else // the current DSP could not be repositioned
							{// go to the DSP block that was moved (not repostioned) the last time
								rot[i]=false;
								if( lastMovedDSP>=0) 
								{
									finish = (clock() - start)/(CLOCKS_PER_SEC*60);
									if (finish > maxTimeInMinutes)
										return;
									tilingAlgorithm( lastMovedDSP,n,false, lastMovedDSP);
								}
							}
			
		
					}
				else // the folling DSP could not be moved or repositioned 
					{	
						//~ if(i==0)
						//~ display(globalConfig);
						if(move(globalConfig,i)) // the current DSP was successfuly moved
							{
					//			cout<<"Pas 1_2 "<<i<<endl;
								if(i==0){
									REPORT(DETAILED, "DSP #1 has made another step!");
									//~ display(globalConfig);
									//~ cout<<endl<<endl<<endl;
				
								}
								finish = (clock() - start)/(CLOCKS_PER_SEC*60);
								if (finish > maxTimeInMinutes)
										return;
								tilingAlgorithm(i+1,n,true,i);
								
							}
						//~ if(counterfirst%100==0)
						//~ cout<<counterfirst<<"DSP #1 has made 100 steps!"<<endl;
						//~ display(globalConfig);
						//~ cout<<endl<<endl<<endl;
				
						//~ }
						//~ tilingAlgorithm(i+1,n,true,i);
						//~ }
						else // the current DSP was not moved successfuly
							{
								if(rot[i]==false && (globalConfig[i]->getMaxMultiplierWidth() != globalConfig[i]->getMaxMultiplierHeight() ))
									{// if the DSP was not rotated and is not sqare then roteate it
					//					cout<<" Pas 2_2 "<<i<<endl;
										globalConfig[i]->rotate();
										if(replace(globalConfig,i)) // the current DSP was successfuly repositioned
											{
												finish = (clock() - start)/(CLOCKS_PER_SEC*60);
												if (finish > maxTimeInMinutes)
													return;
												rot[i]=true;
												tilingAlgorithm(i+1,n,true,i);
												
											}
										else // the current DSP was not successfuly repositioned
											{
												if(i-1>=0)
												{
													finish = (clock() - start)/(CLOCKS_PER_SEC*60);
													if (finish > maxTimeInMinutes)
														return;
													tilingAlgorithm(i-1,n,repl,i);
												}
											}
									}
								else // the DSP is either square or has been already rotated
									{
					//					cout<<" Pas 3_2 "<<i<<endl;
										if(i-1>=0)
										{
											finish = (clock() - start)/(CLOCKS_PER_SEC*60);
											if (finish > maxTimeInMinutes)
												return;
											tilingAlgorithm(i-1,n,repl,i);		//repl should be false
										}
									}
							}
					}
			}

	
	}


	bool IntTilingMult::compareOccupation(DSP** config)
	{
		int totalSize = (vnme-getExtraWidth()) *(vmme-getExtraHeight());
		int sizeBest = totalSize;
		int sizeConfig = totalSize;
		//~ cout<<"Total size is "<<totalSize<<endl;
		int c1X,c2X,c1Y,c2Y;
		int nj,ni,njj,nii;
		int ii,jj,i,j;
		int n,m;
		//~ n=wInX + 2* getExtraWidth();
		//~ m=wInY + 2* getExtraHeight();
		n=vm;
		m=vm;
	
		int nmew = vnme;
		int ew = getExtraWidth();
		int mmeh = vmme;
		int eh = getExtraHeight();
	
		for(int ti=0;ti<nrDSPs;ti++)
			if(config[ti]!=NULL)
				{
			
			
					config[ti]->getTopRightCorner(c1X,c1Y);
					config[ti]->getBottomLeftCorner(c2X,c2Y);
					c1X=n-c1X-1;
					c2X=n-c2X-1;
					//fillMatrix(mat,n,m,c2X,c1Y,c1X,c2Y,count);
					j = c2X;
					i = c1Y;
					jj = c1X;
					ii = c2Y;
			
					//cout<<" New coordinates are ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<")"<<endl;
			
			
						
					if( j>= nmew || jj< ew || i >= mmeh || ii < eh)
						{
							nj=ni=njj=nii=0;
						}
					else
						{
							if( j < getExtraWidth() )
								nj = getExtraWidth() ;
							else
								nj = j;
							if( jj >= n - getExtraWidth() )
								njj = n -getExtraWidth() -1;
							else
								njj = jj;
							
							if( i < getExtraHeight() )
								ni = getExtraHeight() ;
							else
								ni = i;
							if( ii >= m - getExtraHeight() )
								nii = m -getExtraHeight() -1;
							else
								nii = ii;
							//costSlice +=target->getIntMultiplierCost(njj-nj+1,nii-ni+1);
							sizeConfig-=(njj-nj+1)*(nii-ni+1);
							
						}
			
				}
		
		//cout<<"Size config is"<<sizeConfig<<endl;
		//display(config);
		
		
		
		for(int ti=0;ti<nrDSPs;ti++)
			if(bestConfig[ti]!=NULL)
				{
			
			
					bestConfig[ti]->getTopRightCorner(c1X,c1Y);
					bestConfig[ti]->getBottomLeftCorner(c2X,c2Y);
					c1X=n-c1X-1;
					c2X=n-c2X-1;
					//fillMatrix(mat,n,m,c2X,c1Y,c1X,c2Y,count);
					j = c2X;
					i = c1Y;
					jj = c1X;
					ii = c2Y;
			
					//cout<<" New coordinates are ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<")"<<endl;
			
			
						
					if( j>= nmew || jj< ew || i >= mmeh || ii < eh)
						{
							nj=ni=njj=nii=0;
						}
					else
						{
							if( j < getExtraWidth() )
								nj = getExtraWidth() ;
							else
								nj = j;
							if( jj >= n - getExtraWidth() )
								njj = n -getExtraWidth() -1;
							else
								njj = jj;
							
							if( i < getExtraHeight() )
								ni = getExtraHeight() ;
							else
								ni = i;
							if( ii >= m - getExtraHeight() )
								nii = m -getExtraHeight() -1;
							else
								nii = ii;
							//costSlice +=target->getIntMultiplierCost(njj-nj+1,nii-ni+1);
							sizeBest-=(njj-nj+1)*(nii-ni+1);
							
						}
			
				}
		
		
		//cout<<"Size best is "<<sizeBest<<endl;
		//display(bestConfig);
		
		
		if(sizeBest >= sizeConfig)		
			return true;
		else
			return false;
	
	}

	void IntTilingMult::fillMatrix(int **&matrix,int lw,int lh,int topleftX,int topleftY,int botomrightX,int botomrightY,int value)
	{
		for(int j=topleftX;j<=botomrightX;j++)
			{
				for(int i=topleftY;i<=botomrightY;i++)
					{
						if(j>-1&&i>-1&&i<lh&&j<lw)
							matrix[i][j]=value;
					}
			}
	
	}


	void IntTilingMult::display(DSP** config)
	{
		ofstream fig;
		fig.open ("tiling.fig", ios::trunc);
		fig << "#FIG 3.2  Produced by xfig version 3.2.5a" << endl;
		fig << "Landscape" << endl;
		fig << "Center" << endl;
		fig << "Metric" << endl;
		fig << "A4      " << endl;
		fig << "100.00" << endl;
		fig << "Single" << endl;
		fig << "-2" << endl;
		fig << "1200 2" << endl;
	
	
		int **mat;
		int n,m;
		int count=1;
		//~ n=wInX + 2* getExtraWidth();
		//~ m=wInY + 2* getExtraHeight();
		n = vn;
		m= vm;
		REPORT(INFO, "real width"<<vn - 2* getExtraWidth()<<"real height"<<vm - 2* getExtraHeight());
		REPORT(INFO, "width "<<n<<"height "<<m);
		mat = new int*[m];
	
		int nmew = vnme;
		int ew = getExtraWidth();
		int mmeh = vmme;
		int eh = getExtraHeight();
		int nj,ni,njj,nii;
		for(int i=0;i<m;i++)
			{
				mat[i] = new int [n];
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
		for(int i=0;i<nrDSPs;i++)
			{
				int c1X,c2X,c1Y,c2Y;
			
				config[i]->getTopRightCorner(c1X,c1Y);
				config[i]->getBottomLeftCorner(c2X,c2Y);
				REPORT(INFO, "DSP #"<<i+1<<"has toprigh ("<<c1X<<","<<c1Y<<") and botomleft ("<<c2X<<","<<c2Y<<")");
				fig << " 2 2 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 5 " << endl;
				fig << "	  " << (-c2X+getExtraWidth()-1)*45 << " " << (c1Y-getExtraHeight())*45 
				         << " " << (-c1X+getExtraWidth())*45 << " " << (c1Y-getExtraHeight())*45 
				         << " " << (-c1X+getExtraWidth())*45 << " " << (c2Y-getExtraHeight()+1)*45 
				         << " " << (-c2X+getExtraWidth()-1)*45 << " " << (c2Y-getExtraHeight()+1)*45 
				         << " " << (-c2X+getExtraWidth()-1)*45 << " " << (c1Y-getExtraHeight())*45 << endl;
				
				c1X=n-c1X-1;
				c2X=n-c2X-1;
				//~ cout<<"new x1 "<<c1X<<" new x2 "<<c2X<<endl;
			
				fillMatrix(mat,n,m,c2X,c1Y,c1X,c2Y,count);
				count++;			
			}
	
		count++;
		for(int i=0;i<m;i++)
			{
				for(int j=0;j<n;j++)
					{
						if(mat[i][j]==0)
							{
								int ver =0;
								int ii=i,jj=j;
								while(ver<6&&(ii<m-1||jj<n-1))
									{
							
							
										if(ver<3)
											{
								
												if(ver==0||ver==1)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=2;							
													}
							
												if(ver==0||ver==2)
													jj++;
							
												if(jj>n-1)
													{
														jj=n-1;
														ver=1;
													}
								
								
							
												for(int k=ii,q=jj;k>i-1&&(ver==0||ver==2);k--)
													if(mat[k][q]!=0)
														{
															if(ver==0)
																ver=1;
															else
																ver=3;
															jj--;
														}
									
												for(int k=ii,q=jj;q>j-1&&(ver==0||ver==1);q--)
													if(mat[k][q]!=0)
														{
															if(ver==0)
																ver=2;
															else
																ver=3;
															ii--;
														}
								
							
											}
										else
											{
												if(ver==3||ver==5)
													jj++;
							
												if(jj>n-1)
													{
														jj=n-1;
														ver=4;
													}
								
								
												if(ver==3||ver==4)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=5;							
													}
							
								
															
												for(int k=ii,q=jj;q>j-1&&(ver==3||ver==4);q--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=5;
															else
																ver=6;
															ii--;
														}
								
												for(int k=ii,q=jj;k>i-1&&(ver==3||ver==5);k--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=4;
															else
																ver=6;
															jj--;
														}
												if(ver==5&&jj==n-1)
													ver=6;
												if(ver==4&&ii==m-1)
													ver=6;
										
															
								
											}
									}
						
						
					
						
						
								if( j>= nmew || jj< ew || i >= mmeh || ii < eh)
									{
										REPORT(INFO,"Partition number "<<count<<" is totally out of the real multiplication bounds. ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<")");
									}
								else
									{
										if( j < getExtraWidth() )
											nj = getExtraWidth() ;
										else
											nj = j;
										if( jj >= n - getExtraWidth() )
											njj = n -getExtraWidth() -1;
										else
											njj = jj;
							
										if( i < getExtraHeight() )
											ni = getExtraHeight() ;
										else
											ni = i;
										if( ii >= m - getExtraHeight() )
											nii = m -getExtraHeight() -1;
										else
											nii = ii;
										REPORT(INFO, "Partition number "<<count<<" with bounds. ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<") has now bounds ("<<nj<<" , "<<ni<<" , "<<njj<<" , "<<nii<<")");
									}
						
								REPORT(INFO,j<<" "<<i<<" "<<jj<<" "<<ii);
								fillMatrix(mat,n,m,j,i,jj,ii,count);
								count++;
						
							}
					}
			
			}
		
		
		
		char af;
		int afi;
		if (verbose>1){
			for(int i=0;i<m;i++)
			{
				if(i==getExtraHeight())
					cout<<endl;
				
				for(int j=0;j<n;j++)
				{
					if(j==getExtraWidth())
						cout<<" ";
					if(j==n-getExtraWidth())
						cout<<" ";
					
					if(mat[i][j]<10)
						afi=mat[i][j];
					else
						afi=mat[i][j]+7;
					af=(int)afi+48;
					cout<<af;
				}
				cout<<endl;
				if(i==m-getExtraHeight()-1)
					cout<<endl;
			}
		}
		for(int ii=0;ii<m;ii++)
		   delete [] (mat[ii]);
	
		delete[] (mat);
		
		

		fig << "		2 2 1 1 0 7 50 -1 -1 4.000 0 0 -1 0 0 5" << endl;
		fig << "	  " << (-wInX)*45 << " " << 0 
         << " " << 0 << " " << 0  
         << " " << 0 << " " << (wInY)*45 
         << " " << (-wInX)*45 << " " << (wInY)*45 
         << " " << (-wInX)*45 << " " << 0 << endl;

		
		fig.close();
	}



	int IntTilingMult::partitionOfGridSlices(DSP** config,int &partitions)
	{
		//~ cout<<"Incepe"<<endl;
		int costSlice=0;
	
		int n,m;
		int count=1;
		n=vn;
		m=vm;
	
		int nmew = vnme;
		int ew = getExtraWidth();
		int mmeh = vmme;
		int eh = getExtraHeight();
		int nj,ni,njj,nii;
		
		
		//~ cout<<"width "<<n<<"height "<<m<<endl;
	
		for(int i=0;i<m;i++)
			{
			
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
		for(int i=0;i<nrDSPs;i++)
			{
				int c1X,c2X,c1Y,c2Y;
			
				config[i]->getTopRightCorner(c1X,c1Y);
				config[i]->getBottomLeftCorner(c2X,c2Y);
				//~ cout<<"DSP #"<<i+1<<"has toprigh ("<<c1X<<","<<c1Y<<") and botomleft ("<<c2X<<","<<c2Y<<")"<<endl;
				c1X=n-c1X-1;
				c2X=n-c2X-1;
				//~ cout<<"new x1 "<<c1X<<" new x2 "<<c2X<<endl;
			
				fillMatrix(mat,n,m,c2X,c1Y,c1X,c2Y,count);
				count++;			
			}
		//partitions = count;
		partitions = 0;
	
		//~ cout<<"Partea 2"<<endl;
		
		for(int i=0;i<m;i++)
			{
				for(int j=0;j<n;j++)
					{
						if(mat[i][j]==0)
							{
								int ver =0;
								int ii=i,jj=j;
								while(ver<6&&(ii<m-1||jj<n-1))
									{
							
							
										if(ver<3)
											{
								
												if(ver==0||ver==1)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=2;							
													}
							
												if(ver==0||ver==2)
													jj++;
							
												if(jj>n-1)
													{
														jj=n-1;
														ver=1;
													}
												//~ if(count==3)
												//~ cout<<"P0  ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl;
								
								
							
												for(int k=ii,q=jj;k>i-1&&(ver==0||ver==2);k--)
													if(mat[k][q]!=0)
														{
															if(ver==0)
																ver=1;
															else
																ver=3;
															jj--;
														}
												//~ if(count==3)
												//~ {
												//~ cout<<"P1   ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl; 
												//~ }
									
												for(int k=ii,q=jj;q>j-1&&(ver==0||ver==1);q--)
													if(mat[k][q]!=0)
														{
															if(ver==0)
																ver=2;
															else
																ver=3;
															ii--;
														}
												//~ if(count==3)
												//~ {cout<<"P2  ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl;
												//~ cout<<endl;
												//~ }
							
											}
										else
											{
												if(ver==3||ver==5)
													jj++;
							
												if(jj>n-1)
													{
														jj=n-1;
														ver=4;
													}
								
												//~ if(count==3)
												//~ cout<<"P3  ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl;
								
												if(ver==3||ver==4)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=5;							
													}
							
								
												//~ if(count==3)
												//~ cout<<"P3  ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl;

								
												for(int k=ii,q=jj;q>j-1&&(ver==3||ver==4);q--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=5;
															else
																ver=6;
															ii--;
														}
												//~ if(count==3)
												//~ {
												//~ cout<<"P4   ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl; 
												//~ }
								
												for(int k=ii,q=jj;k>i-1&&(ver==3||ver==5);k--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=4;
															else
																ver=6;
															jj--;
														}
												if(ver==5&&jj==n-1)
													ver=6;
												if(ver==4&&ii==m-1)
													ver=6;
										
												//~ if(count==3)
												//~ {cout<<"P5  ii:="<<ii<<" jj:="<<jj<<" ver:="<<ver<<endl;
												//~ cout<<endl;
												//~ }
							
								
											}
									}
						
								//~ cout<<count<<endl;
						
					
												
						
								if( j>=nmew || jj< ew || i >= mmeh || ii < eh)
									{
							
									}
								else
									{
										if( j < ew )
											nj = ew ;
										else
											nj = j;
										if( jj >=nmew )
											njj = nmew -1;
										else
											njj = jj;
							
										if( i < eh )
											ni = eh ;
										else
											ni = i;
										if( ii >=mmeh)
											nii = mmeh -1;
										else
											nii = ii;
							
										partitions++;
										//cout << "IntMultiplierCost ("<<nj<<", "<<njj<<") ("<<ni<<", "<<nii<<") cost="<< target_->getIntMultiplierCost(njj-nj+1,nii-ni+1) << endl;
										costSlice += target_->getIntMultiplierCost(njj-nj+1,nii-ni+1);//(njj-nj+1)*(nii-ni+1);
							
							
														
									}
						
						
						
								fillMatrix(mat,n,m,j,i,jj,ii,count);
								count++;
						
							}
					}
			
			}
		
		//de verificat
		
		//cout<<"Count "<<count<<" Partitions "<<partitions<<endl;
		
		//partitions =count -partitions;
		 
		
		//~ char af;
		//~ int afi;
		//~ for(int i=0;i<m;i++)
		//~ {
		//~ for(int j=0;j<n;j++)
		//~ {
		//~ if(mat[i][j]<10)
		//~ afi=mat[i][j];
		//~ else
		//~ afi=mat[i][j]+7;
		//~ af=(int)afi+48;
		//~ cout<<af;
		//~ }
		//~ cout<<endl;
		//~ }
	
		//~ cout<<"gata"<<endl;
	
		//~ for(int ii=0;ii<m;ii++)
		//~ delete[](mat[ii]);
	
		//~ delete[] (mat);
	
		return costSlice;
	}	



	int IntTilingMult::bindDSPs4Virtex(DSP** &config)
	{
		int nrOfUsedDSPs=0;
		//~ int dx,dy;
		
		//~ for(int i=0;i<nrDSPs;i++){
			
			//~ config[i]->getTopRightCorner(dx,dy);
			//~ if(config[i]!=NULL )  // && (dx<=vnme && dy<vmme)  // with this condition in this if the algorithm will be encorege to try to compute with less dsps that the user has given
				//~ {
					//~ nrOfUsedDSPs++;
				//~ }
			//~ //countsShift[i]=0;	
			//~ }
		nrOfUsedDSPs = nrDSPs;	
			
		DSP* ref;
	
		sortDSPs(config);
		
			
		int itx,ity,jtx,jty,ibx,iby;//,jbx,jby;
		//int prev=0;
			
		//cout<<endl<<endl;	
		int count;
		
		for(int i=0;i<nrDSPs;i++)
			{
				
				if ((config[i]!=NULL) && (config[i]->getShiftIn()==NULL))
					{
						
						ref=config[i];
						count=ref->getNrOfPrimitiveDSPs();
						bool ver=true;
						int rw,rh;
						int sa;
						//while(ver==true&&ref->getShiftOut()==NULL && countsShift[prev] <nrOfShifts4Virtex-1)
						while(ver==true&&ref->getShiftOut()==NULL && count <nrOfShifts4Virtex)
							{
								ver=false;
								ref->getTopRightCorner(itx,ity);
								ref->getBottomLeftCorner(ibx,iby);
								rw=ref->getMaxMultiplierWidth();
								rh=ref->getMaxMultiplierHeight();
					
								for(int j=0;j<nrDSPs&&ver==false;j++)
									{
										if(config[j]!=NULL &&j!=i && ((count+ config[j]->getNrOfPrimitiveDSPs())<=nrOfShifts4Virtex))
											{
												config[j]->getTopRightCorner(jtx,jty);
												if((jtx<=vnme && jty<vmme))
												{
												
												sa = config[j]->getShiftAmount();
												//cout<<"Now considering taking(in left) dsp nr. "<<i<<" with tx:="<<itx<<" ty:="<<ity<<" bx:="<<ibx<<"by:="<<iby<<" with dsp nr. "<<j<<" with tx:="<<jtx<<" ty:="<<jty<<endl;
												//config[j]->getBottomLeftCorner(jbx,jby);
												if(rw!=34 && rh!=34)
												{
												if(jtx==ibx+1&&jty==ity&&rw==sa&&config[j]->getShiftIn()==NULL)
													{
														//cout<<"DSP #"<<i<<" bind with DSP# "<<j<<endl;
														ver=true;
														ref->setShiftOut(config[j]);
														config[j]->setShiftIn(ref);
														nrOfUsedDSPs--;
														ref=config[j];
														count+=ref->getNrOfPrimitiveDSPs();
														//~ countsShift[prev]++;
														//~ countsShift[j] = countsShift[prev];
														//~ prev = j;								
													}
												}
												else
												{
													//~ cout<<config[i]->getMaxMultiplierHeight()<<" "<<rh<<" ";
													//~ cout<<(config[i]->getMaxMultiplierHeight()% rh)<<endl;

													if( jtx==ibx+1 && sa!=0 && rw%sa==0 && ( (rw == 34 && jty==ity)   || ( rw==17 && jty==ity+sa   )  ))

													{
														//cout<<" case 1_2 DSP #"<<i<<" bind with DSP# "<<j<<endl;
														ver=true;
														ref->setShiftOut(config[j]);
														config[j]->setShiftIn(ref);
														nrOfUsedDSPs--;
														ref=config[j];
														count+=ref->getNrOfPrimitiveDSPs();
													}
													
												}
											}
											}
									}
					
								for(int j=0;j<nrDSPs&&ver==false;j++)
									{
										if(config[j]!=NULL &&j!=i&& count+ config[j]->getNrOfPrimitiveDSPs()<=nrOfShifts4Virtex)
											{
												config[j]->getTopRightCorner(jtx,jty);
												if((jtx<=vnme && jty<vmme))
												{
												sa = config[j]->getShiftAmount();
												//cout<<"Now considering taking(down) dsp nr. "<<i<<" with tx:="<<itx<<" ty:="<<ity<<" bx:="<<ibx<<"by:="<<iby<<" with dsp nr. "<<j<<" with tx:="<<jtx<<" ty:="<<jty<<endl;
												//config[j]->getBottomLeftCorner(jbx,jby);
												if(rw!=34 && rh!=34)
												{
												if(iby+1==jty&&itx==jtx&&rh==sa&&config[j]->getShiftIn()==NULL)
													{
														//cout<<"DSP #"<<i<<" bind with DSP# "<<j<<endl;
														ver=true;
														ref->setShiftOut(config[j]);
														config[j]->setShiftIn(ref);
														nrOfUsedDSPs--;
														ref=config[j];								
														count+=ref->getNrOfPrimitiveDSPs();
														//~ countsShift[prev]++;
														//~ countsShift[j] = countsShift[prev];
														//~ prev = j;
													}
												}
												else
												{
													//~ cout<<config[i]->getMaxMultiplierWidth()<<" "<<rw<<endl;
													//~ cout<<(config[i]->getMaxMultiplierWidth()% rw)<<endl;

													//&&  (config[j]->getMaxMultiplierWidth()% rw==0)
													if( iby+1==jty &&sa!=0&& rh% sa==0 && ( (rh == 34 && jtx==itx   )   || ( rw==17 && jtx==itx+sa)  ))

													{
														//cout<<"case 2_2 DSP #"<<i<<" bind with DSP# "<<j<<endl;
														ver=true;
														ref->setShiftOut(config[j]);
														config[j]->setShiftIn(ref);
														nrOfUsedDSPs--;
														ref=config[j];								
														count+=ref->getNrOfPrimitiveDSPs();
													}
												}
											}
											}						
									}					
							}
				
					}
			
			}
	
		//cout<<" nr de dspuri dupa bind "<<nrOfUsedDSPs<<endl;
		return nrOfUsedDSPs;
	
	}

	void IntTilingMult::sortDSPs(DSP** &config)
	{
		int ix,iy,jx,jy;
		DSP* temp;
		for(int i=0;i<nrDSPs-1;i++)
			{		
				for(int j=i+1;j<nrDSPs;j++)
					{
						config[i]->getTopRightCorner(ix,iy);
						config[j]->getTopRightCorner(jx,jy);
						if(iy>jy)
							{
								temp=config[i];
								config[i]=config[j];
								config[j]=temp;				
							}
						else
							if(iy==jy)
								{
									if(ix>jx)
										{
											temp=config[i];
											config[i]=config[j];
											config[j]=temp;						
										}
								}
					}
			}
	
	}

	int IntTilingMult::bindDSPs(DSP** &config)
	{
		if  ( (target_->getID() == "Virtex4") ||
		      (target_->getID() == "Virtex5") ||
		      (target_->getID() == "Spartan3"))  // then the target is a Xilinx FPGA
			{
				return bindDSPs4Virtex(config);
			}
		else // the target is Stratix
			{
				return bindDSPs4Stratix(config);
			}
	}


	void IntTilingMult::compareCost()
	{
		//~ cout<<"Inta la cost"<<endl;
	
		//~ DSP** tempc;
		
		//~ tempc= new DSP*[nrDSPs];
		//~ for(int ii=0;ii<nrDSPs;ii++)
		//~ tempc[ii]= new DSP();
		
		//memcpy(tempc,globalConfig,sizeof(DSP*) *nrDSPs );
		for(int ii=0;ii<nrDSPs;ii++)
			memcpy(tempc[ii],globalConfig[ii],sizeof(DSP) );
	
		//display(tempc);
		//~ cout<<"intra la display cost"<<endl;
		
		float temp = computeCost(tempc);
		
		/*
		display(globalConfig);
		cout<<"score temp is"<<temp<<" and current best is"<<bestCost<<endl;
		getchar();
		*/
		if(temp < bestCost)
			{
				//~ cout<<"Costul e mai bun la cel curent!Schimba"<<endl;
				
				//cout<<"Interchange! Score for current is"<<temp<<" and current best is"<<bestCost<<endl;
				
				//~ int c1X,c2X,c1Y,c2Y;
				//~ int n=wInX + 2* getExtraWidth();
				//~ tempc[0]->getTopRightCorner(c1X,c1Y);
				//~ tempc[0]->getBottomLeftCorner(c2X,c2Y);
				//~ cout<<"DSP #"<<1<<"has toprigh ("<<c1X<<","<<c1Y<<") and botomleft ("<<c2X<<","<<c2Y<<")"<<endl;
				//~ c1X=n-c1X-1;
				//~ c2X=n-c2X-1;
				//~ //cout<<"new x1 "<<c1X<<" new x2 "<<c2X<<endl;
				//~ cout<<"matrix DSP #"<<1<<"has topleft ("<<c2X<<","<<c1Y<<") and botomright ("<<c1X<<","<<c2Y<<")"<<endl;
		
		
		
				bestCost=temp;
				REPORT(INFO, "New best score is: " << bestCost);
				//memcpy(bestConfig,tempc,sizeof(DSP*) *nrDSPs );	
				for(int ii=0;ii<nrDSPs;ii++)
					memcpy(bestConfig[ii],globalConfig[ii],sizeof(DSP) );
				//display(bestConfig);
			}
		else
			if(temp == bestCost )
				{
					//cout<<"Cost egal!!!"<<endl;
					//cout<<"Rezult compare is"<<compareOccupation(tempc)<<endl;
					//~ cout<<"score temp is"<<temp<<" and current best is"<<bestCost<<endl;
					//display(bestConfig);
					if(compareOccupation(tempc)==true)
						{
							REPORT(INFO, "Interchange for equal cost. Now best has cost "<<temp);
							
							bestCost=temp;
			
							for(int ii=0;ii<nrDSPs;ii++)
								memcpy(bestConfig[ii],globalConfig[ii],sizeof(DSP) );
							//	display(bestConfig);
						}
				}
	
	
		//~ for(int ii=0;ii<nrDSPs;ii++)
		//~ free(tempc[ii]);
	
		//~ delete[] (tempc);
	
	}



	float IntTilingMult::computeCost(DSP** &config)
	{
	
		
		float acc=0.0;
		
		//costLUT = ( (1.0+scale) - scale * (1-ratio) ) /  ((float)100);
	
		//~ cout<<"Cost of a DSP is "<<costDSP<<endl<<"Cost of a Slice is "<<costLUT<<endl;
	
		//~ int nrOfUsedDSPs=0;
	
		//~ for(int i=0;i<nrDSPs;i++)
			//~ if(config[i]!=NULL)
				//~ {
					//~ acc+=costDSP;
					//~ nrOfUsedDSPs++;
				//~ }
		
		int nrOfUsedDSPs=nrDSPs;
		acc = ((float)nrDSPs)*costDSP;
	
		
		//~ cout<<"Number of used DSP blocks is "<<nrOfUsedDSPs<<endl;
		
		int partitions;
		float LUTs4Multiplication =  partitionOfGridSlices(config,partitions);
	
		//~ cout<<"Number of slices 4 multiplication of the rest is "<<LUTs4Multiplication<<endl;
		
		acc =((float)nrOfUsedDSPs)*costDSP + costLUT * LUTs4Multiplication;
		
		//~ cout<<"Number of partitions for LUTs is "<<partitions<<endl;
		nrOfUsedDSPs = bindDSPs(config);
		//~ cout<<"Number of operands coming from DSPs is "<<nrOfUsedDSPs<<endl;
		
	
		float LUTs4NAdder=((float)target_->getIntNAdderCost(wInX + wInY,nrOfUsedDSPs+partitions) );
		//float LUTs4NAdder=((float)200);
				
				
	
		//~ cout<<"LUTs used for last "<<nrOfUsedDSPs+partitions<<" adder are"<<LUTs4NAdder<<endl;
		
		acc +=  LUTs4NAdder* costLUT;	
		
		
		//~ Substracting the cost of different additions that can be done in DSPs(Virtex) or the cost of a DSP if they can be combined(Altera)
		
		
		return acc;
			
	}

	
	
	int IntTilingMult::estimateDSPs()
	{
		if (ratio > 1){
			REPORT(INFO, "Ratio is " << ratio << " and should be in [0,1]. Will saturate to 1");
			ratio = 1;
		}
			
		float t1,t2,t3,t4; /* meaningful vars ;) */
		int Xd, Yd;  /* the dimension of the multiplier on X and on Y */
		target_->getDSPWidths(Xd,Yd);
		bool fitMultiplicaication = false;

		int maxDSP, mDSP = target_->getNumberOfDSPs();
		int wInXt = wInX;
		int wInYt = wInY;
		if (wInY > wInX){
			wInYt = wInX;
			wInXt = wInY;
		}

		/* all trivial tiling possibilities */
		t1 = ((float) wInXt) / ((float) Xd);
		t2 = ((float) wInYt) / ((float) Yd);
		t3 = ((float) wInXt) / ((float) Yd);
		t4 = ((float) wInYt) / ((float) Xd);
		
		maxDSP = int ( max( ceil(t1)*ceil(t2), ceil(t3)*ceil(t4)) );
	
		if(maxDSP > mDSP){
			fitMultiplicaication = true;
			maxDSP = mDSP; //set the maximum number of DSPs to the multiplication size
		}
			
		if (ratio == 1){
			if (fitMultiplicaication){
				REPORT(INFO, "Warning!!! The number of existing DSPs on this FPGA is not enough to cover the whole multiplication!");
			}else{
				REPORT(INFO, "Warning!!! The minimum number of DSP that is neccessary to cover the whole addition will be used!");
			}
			return maxDSP;
		}else{	
						
//			float scaleDSPs = 1.0;
//			if(maxDSP>4)
//				scaleDSPs= ((float)maxDSP) / ((float)maxDSP-2);
//			float temp = ( float(target_->getIntMultiplierCost(wInX,wInY)) * ratio* scaleDSPs)  /   (float(target_->getEquivalenceSliceDSP())) ;
//			//float temp = ( float(target_->getIntMultiplierCost(wInX,wInY)) )  /   ((1.-ratio)*float(target_->getEquivalenceSliceDSP())) ;
//			cout<<"val calc "<<temp<<endl;
//			int i_tmp = int(ceil(temp));
//			cout<<" rounded "<<i_tmp<<endl;
//	
//			if(i_tmp > maxDSP){
//				if (fitMultiplicaication){
//					REPORT(INFO, "Warning!!! The number of estimated DSPs with respect with this ratio of preference is grather then the needed number of DSPs to perform this multiplication!");
//				}else{
//					REPORT(INFO, "Warning!!! The number of estimated DSPs with respect with this ratio of preference is grather then the total number of DSPs that exist on this board!");
//				}
//				
//				i_tmp = maxDSP;
//				//cout<<" final val is(the max dsps) "<<i_tmp<<endl;
//			}
//			return i_tmp ;

			return int(ceil((float)maxDSP * ratio));
		}
	}
	
	
	
	
	int  IntTilingMult::getExtraHeight()
	{

		
		int x,y;	
		target_->getDSPWidths(x,  y);
		float temp = ratio * 0.75 * ((float) y);
		return ((int)temp);
		//return 4;

	}

	
	int  IntTilingMult::getExtraWidth()
	{

		int x,y;	
		target_->getDSPWidths(x,y);
		float temp = ratio * 0.75 * ((float) x);
		return ((int)temp);
		//return 4;

	}





	void IntTilingMult::emulate(TestCase* tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		mpz_class svR = svX * svY;

		tc->addExpectedOutput("R", svR);
	}

	void IntTilingMult::buildStandardTestCases(TestCaseList* tcl){
	
	}


	int IntTilingMult::checkFarness(DSP** config,int index)
	{
		int xtr1, ytr1, xbl1, ybl1, xtr2, ytr2, xbl2, ybl2;
		//if we run the algorithm with one less DSP then we should verify against the DSP nrDSP-2
	
		if(index  < 1)
			return 0;
	
		config[index]->getTopRightCorner(xtr1, ytr1);
		config[index]->getBottomLeftCorner(xbl1, ybl1);
		int dist=0;
		bool ver=false;
		int sqrDist = maxDist2Move * maxDist2Move;
	
		dist=0;	
		for (int i=0; i<index; i++)
			if ((config[i] != NULL) )
				{
					config[i]->getTopRightCorner(xtr2, ytr2);
					config[i]->getBottomLeftCorner(xbl2, ybl2);
					if(xtr1 > xbl2+1 + maxDist2Move)
						dist++;			
				}
		if(dist == index)
			return 1;
	
		//cout<<"Sqr max is "<<sqrDist<<endl;
		for (int i=0; i<index; i++)
			if ((config[i] != NULL) )
				{
					dist=0;
					config[i]->getTopRightCorner(xtr2, ytr2);
					config[i]->getBottomLeftCorner(xbl2, ybl2);
					if(xtr1 > xbl2)
						dist +=(xtr1 - xbl2-1)*(xtr1 - xbl2-1);
					//~ cout<<"xtr1= "<<xtr1<<" xbl2= "<<xbl2<<" Dist = "<<dist<<"  ";
					if(ybl2 < ytr1)
						dist +=(1+ybl2-ytr1) * (1+ybl2-ytr1) ;
					else
						if(ybl1 < ytr2)
							{
								dist +=(1+ybl1 - ytr2) *(1+ybl1 - ytr2) ;
								ver =true;
							}
			
					//~ cout<<"The distance for DSP "<<i<<" is "<<dist<<endl;
					if(dist <= sqrDist)
						{
							//cout<<"Dist  was = "<<dist<<endl;
							return 0; //OK
					
						}			
				}
	
	
	
		if(!ver)
			return 2;
	
		return 3;
	
	}

	bool IntTilingMult::checkOverlap(DSP** config, int index)
	{	
		
		//return false;
		int x,y,w,h;
		h = config[index]->getMaxMultiplierHeight();
		w = config[index]->getMaxMultiplierWidth();
		int poscur = config[index]->getCurrentPosition();
		int posav = config[index]->getAvailablePositions();
		int minY,minX;
		config[index]->getTopRightCorner(minX,minY);
		//~ cout<<index<<" "<< minX<<" "<<minY<<" "<<poscur<<" "<<posav<<" ";
		minX+=w; 
		for(;poscur<posav;poscur++)
		{
			if(minY>config[index]->Ypositions[poscur])
			{
				minY=config[index]->Ypositions[poscur];
				minX=config[index]->Xpositions[poscur];
			}
		}
		//~ cout<<index<<" "<< minX<<" "<<minY<<" ";
		
		config[index]->getBottomLeftCorner(x,y);
		
		
		
		long area = (vn - minX+6) * (vm -minY+6) + w * (vm- y);

		//~ cout<<" area "<<area<<" dspArea "<<dsplimit<<" ";
		int dsplimittemp = (int)ceil( ((double)area) / dsplimit );
		//~ cout<<" limit "<<dsplimittemp<<" nrrest "<<(numberDSP4Overlap -index-1)<<endl;
		//I have added +1 to the number of dsps that could be place because when we work with combined dsps there exists the case when an are of a dsp is not added
		if( dsplimittemp+1 < (numberDSP4Overlap -index-1))

			return true;
		
		//return false;
		
		//not used now
		
		int xtr1, ytr1, xbl1, ybl1, xtr2, ytr2, xbl2, ybl2;
		//int nrdsp = sizeof(config)/sizeof(DSP*);
		config[index]->getTopRightCorner(xtr1, ytr1);
		config[index]->getBottomLeftCorner(xbl1, ybl1);
	

		
		bool a1 ;
		bool a2 ;
		bool a3 ;
		bool a4 ;
		bool a5 ;
		bool a6 ;
				
		bool b1 ;
		bool b2 ;
		bool b3 ;
		bool b4 ;
		bool b5 ;
		bool b6 ;
		
	
		if(verbose)
			cout << tab << tab << "checkOverlap: ref is block #" << index << ". Top-right is at (" << xtr1 << ", " << ytr1 << ") and Bottom-right is at (" << xbl1 << ", " << ybl1 << ")" << endl;
	
		for (int i=0; i<index; i++)
			if (config[i] != NULL)
				{
					config[i]->getTopRightCorner(xtr2, ytr2);
					config[i]->getBottomLeftCorner(xbl2, ybl2);
					//cout<<index<<" "<<i<<" "<<xbl1<<" "<<xtr2<<endl;
					if (((xbl1 < xbl2) && (ytr2 > ybl1)) || 	// config[index] is above and to the right of config[i]
						  ((xbl1 < xtr2)))// && (ybl1 < ybl2))) 							// config[index] is to the right of config[i]
						return true;
			
				
					if(verbose)
						cout << tab << tab << "checkOverlap: comparing with block #" << i << ". Top-right is at (" << xtr2 << ", " << ytr2 << ") and Bottom-right is at (" << xbl1 << ", " << ybl1 << ")" << endl;
			
					//~ if (((xtr2 <= xbl1) && (ytr2 <= ybl1) && (xtr2 >= xtr1) && (ytr2 >= ytr1)) || // config[index] overlaps the upper and/or right part(s) of config[i]
					//~ ((xbl2 <= xbl1) && (ybl2 <= ybl1) && (xbl2 >= xtr1) && (ybl2 >= ytr1)) || // config[index] overlaps the bottom and/or left part(s) of config[i]
					//~ ((xtr2 <= xbl1) && (ybl2 <= ybl1) && (xtr2 >= xtr1) && (ybl2 >= ytr1)) || // config[index] overlaps the upper and/or left part(s) of config[i]
					//~ ((xbl2 <= xbl1) && (ytr2 <= ybl1) && (xbl2 >= xtr1) && (ytr2 >= ytr1)) || // config[index] overlaps the bottom and/or right part(s) of config[i]
					//~ ((xbl2 >= xbl1) && (ybl2 <= ybl1) && (ytr2 >= ytr1) && (xtr2 <= xtr1)) || // config[index] overlaps the center part of config[i]
				
					//~ ((xtr1 <= xbl2) && (ytr1 <= ybl2) && (xtr1 >= xtr2) && (ytr1 >= ytr2)) || // config[i] overlaps the upper and/or right part(s) of config[index]
					//~ ((xbl1 <= xbl2) && (ybl1 <= ybl2) && (xbl1 >= xtr2) && (ybl1 >= ytr2)) || // config[i] overlaps the bottom and/or left part(s) of config[index]
					//~ ((xtr1 <= xbl2) && (ybl1 <= ybl2) && (xtr1 >= xtr2) && (ybl1 >= ytr2)) || // config[i] overlaps the upper and/or left part(s) of config[index]
					//~ ((xbl1 <= xbl2) && (ytr1 <= ybl2) && (xbl1 >= xtr2) && (ytr1 >= ytr2)) || // config[i] overlaps the bottom and/or right part(s) of config[index]
					//~ ((xbl1 >= xbl2) && (ybl1 <= ybl2) && (ytr1 >= ytr2) && (xtr1 <= xtr2))    // config[i] overlaps the center part of config[index]
					//~ )
					//~ return true;
					
					
					
					// the optimisation of the above if
					a1 = (xtr2 <= xbl1);
					a2 = (xtr2 >= xtr1);
					a3 = (xbl2 <= xbl1);
					a4 = (xbl2 >= xtr1);
					a5 = (xbl2 >= xbl1);
					a6 = (xtr1 >= xtr2);
				
					b1 = (ytr2 <= ybl1);
					b2 = (ytr2 >= ytr1);
					b3 = (ybl2 <= ybl1);
					b4 = (ybl2 >= ytr1);
					b5 = (ytr1 >= ytr2);
					b6 = (ybl1 <= ybl2);
				
					if ((((a1 && a2)||(a3 && a4)) && ((b1 && b2)||(b3 && b4))) || 
					    (((a4 && a6)||(a5 && a1)) && ((b6 && b1)||(b4 && b5))) || 
					    (((a5 && b3) && ( b2 && a6)) || ((a3 && b6) && (b5 && a2))))
						return true;
					
				
			
			
				}	
		if(verbose)
			cout << tab << tab << "checkOverlap: return false" << endl;	
		return false;
	}


	/**
		There is one case that is not resolved w=yet. For DSP with widths different then height the algorithm should move the dsp with both values
	*/

	bool IntTilingMult::move(DSP** config, int index)
	{
		int xtr1, ytr1, xbl1, ybl1;
		int w, h;
		//target->getDSPWidths(w,h);
		w= config[index]->getMaxMultiplierWidth();
		h= config[index]->getMaxMultiplierHeight();
	
		config[index]->getTopRightCorner(xtr1, ytr1);
		config[index]->getBottomLeftCorner(xbl1, ybl1);
	
		if(verbose)
			cout << tab << "replace : DSP #" << index << " width is " << w << ", height is " << h << endl; 
		/*
		 * Initialized in the constructor:
		 * vn = wInX+2*getExtraWidth(); width of the tiling grid including extensions
		 * vm = wInY+2*getExtraHeight(); height of the tiling grid including extensions
		 * */
		//~ int exh = getExtraHeight();
		//~ int exw = getExtraWidth();
		int pos; // index for list of positions of a DSP
		
		
		if(index==0) // the first DSP block can move freely on the tiling grid
		{
			return false;
			/*
			//if ((xtr1 > 0) && (ytr1 > 0) && (xbl1 < vn-1) && (ybl1 < vm-1))
					{// then the DSP block is placed outside the bounds 		
	
						//do{
							// move down one unit
							ytr1++;
							ybl1++;
			
							if (ytr1 > exh) // the DSP block has reached the bottom limit of the tiling grid
								{
									// move to top of grid and one unit to the left 
									xtr1++;
									xbl1++;
									
									ytr1 = exh -1; //0
									ybl1 = ytr1 + h-1;
										
									if (xtr1 > exw) // the DSP block has reached the left limit of the tiling grid
										return false;
								}						
							config[index]->setTopRightCorner(xtr1, ytr1);
							config[index]->setBottomLeftCorner(xbl1, ybl1);
						//}while (checkOverlap(config, index));
					}
					*/
		}
		else // all DSP blocks except the first one can move only in fixed positions
		{
						do{
							// move to next position
							pos = config[index]->pop();
							if(pos >= 0)
							{
									ytr1 = config[index]->Ypositions[pos];
									ybl1 = ytr1 + h-1;
									xtr1 = config[index]->Xpositions[pos];
									xbl1 = xtr1 + w-1;
							}
							else
								return false;
								
							//if ((ytr1 > gh) || (xtr1 > gw) || (ybl1 < exh) || (xbl1 < exw)) // the DSP block is out of the tiling grid
							//	continue;
														
							config[index]->setTopRightCorner(xtr1, ytr1);
							config[index]->setBottomLeftCorner(xbl1, ybl1);
						}while (checkOverlap(config, index));
		}
		
			
		/* set the current position of the DSP block within the tiling grid
		config[index]->setTopRightCorner(xtr1, ytr1);
		config[index]->setBottomLeftCorner(xbl1, ybl1);
		
		
		int f = checkFarness(config,index);	
		if (f == 0)
			return true;
		else if (f == 1)
			return false;
		else if (f  == 2)
			{
				// move to top of grid and one unit to the left 
				xtr1++;
				xbl1++;
				ytr1 = exh - h+1;
				ybl1 = ytr1 + h-1;
		
				if (xbl1 > vn-1) // the DSP block has reached the left limit of the tiling grid
					return false;
				config[index]->setTopRightCorner(xtr1, ytr1);
				config[index]->setBottomLeftCorner(xbl1, ybl1);
				move(config, index,w,h);
			}
		else if (f == 3)
			{
				move(config, index,w,h);	
			}
		return false;
		*/
		return true;
	}

	bool IntTilingMult::replace(DSP** config, int index)
	{
		//~ cout<<"DSP nr "<<index<<endl;
		int xtr1, ytr1, xbl1, ybl1;
		int w, h;
		string targetID = target_->getID();
		
		//target->getDSPWidths(w,h);
		w= config[index]->getMaxMultiplierWidth();
		h= config[index]->getMaxMultiplierHeight();
		config[index]->setPosition(0);
		
		if (index > 1)
		{// take all positions from the previous DSP
			int curpos =config[index-1]->getCurrentPosition();
			int avpos = config[index-1]->getAvailablePositions();
				memcpy(config[index]->Xpositions, config[index-1]->Xpositions + curpos, sizeof(int)*(avpos - curpos));	
				memcpy(config[index]->Ypositions, config[index-1]->Ypositions + curpos, sizeof(int)*(avpos - curpos));	
				config[index]->setPosition((avpos - curpos));
		}
		
		if (index > 0)
		{
			w= config[index-1]->getMaxMultiplierWidth();
			h= config[index-1]->getMaxMultiplierHeight();
			//int dif = abs(h-w);
			//int maxd = h;
			int mind = w;
			
			if (w > h)
			{
				mind = h;
			}
			int mindX = mind;
			int mindY = mind;
			

			if (targetID == "Virtex5")
			{ // align bottom-left corner of current with X-possition of previous to catch ideal case
				mindX = abs(w-h);
				mindY = h;
			}
			else if ((targetID == "StratixIV") || (targetID == "StratixIV"))
			{ // align on diagonal
				mindX = -w;
				mindY = h;	
			}
			//int positionDisplacementX[] = {0, dif, mind, maxd, w, w, w, w};
			int positionDisplacementX[] = {0, w, mindX};
			//int positionDisplacementY[] = {h, h, h, h, 0, dif, mind, maxd};
			int positionDisplacementY[] = {h, 0, mindY};

			int x,y,x1,y1, pos;
			config[index-1]->getTopRightCorner(x, y);
			
			//* Loop unrolling
			bool extraPosition = ((w!=h) || ((targetID == "StratixIV") || (targetID == "StratixIV")));
			for (int i=0; i<3; i++)
			{
				x1 =x+positionDisplacementX[i];
				y1 =y+positionDisplacementY[i];
				//if(  (x1<vn&&y1<vm)||(x1>vn&&y1>vm)  ) //allows dsp to get out of the board in the diagonal of the bottom left corner
				if (x1<vnme && y1<vmme && x1>0 && y1>0) 
					if((i!=2) || extraPosition)
					config[index]->push(x1, y1);
				
			}
			
			/*
			cout<<endl<<"index "<<index<<" ";
			config[index]->resetPosition();
			do
			{
				pos = config[index]->pop();
				if(pos>=0)
					cout<<" ("<<config[index]->Xpositions[pos]<<" , "<<config[index]->Ypositions[pos]<<")";	
			}while(pos>=0);
			cout<<endl;
			*/
			
			w= config[index]->getMaxMultiplierWidth();
			h= config[index]->getMaxMultiplierHeight();

			config[index]->resetPosition();
			
			do{// go to next position in list
				pos = config[index]->pop();
				if(pos >= 0)
				{
					ytr1 = config[index]->Ypositions[pos];
					ybl1 = ytr1 + h-1;
					xtr1 = config[index]->Xpositions[pos];
					xbl1 = xtr1 + w-1;
				}
				else
				{
					config[index]->setTopRightCorner(vn, vm);
					config[index]->setBottomLeftCorner(vn+w, vm+h);
					return false;
				}
				
				//~ cout<<index<<" (X,Y)="<<xtr1<<" "<<ytr1<<endl;
					
				//if ((ytr1 > gh) && (xtr1 > gw) && (ybl1 < exh) && (xbl1 < exw)) // the DSP block is out of the tiling grid
				//	continue;
											
				config[index]->setTopRightCorner(xtr1, ytr1);
				config[index]->setBottomLeftCorner(xbl1, ybl1);
			}while (checkOverlap(config, index));
				
			return true;
		}
		else
		{	
		// TODO if first DSP block	
		//xtr1 = ytr1 = 0;
		//xbl1 = w-1;
		//ybl1 = h-1;
		
		//cout<<"index"<<endl;
			
		int exh = getExtraHeight();
		int exw = getExtraWidth();
		
		//~ xtr1 = exw -1;
		//~ ytr1 = exh ;	
		//~ ybl1 = ytr1 + h-1;
		//~ xbl1 = xtr1 + w-1;
		xtr1 = exw ;
		ytr1 = exh ;	
		ybl1 = ytr1 + h-1;
		xbl1 = xtr1 + w-1;
			
		config[index]->setTopRightCorner(xtr1, ytr1);
		config[index]->setBottomLeftCorner(xbl1, ybl1);
		
		if(verbose)
			cout << tab << "replace : DSP width is " << w << ", height is " << h << endl; 
		// try yo place the DSP block inside the extended region of the tiling grid
		}
		return true;
	}

	void IntTilingMult::initTiling(DSP** &config, int dspCount)
	{
		int w,h; 
		target_->getDSPWidths(w, h);
		dsplimit = w*h;
		config = new DSP*[nrDSPs];
		//nrOfShifts4Virtex=4;
		//countsShift = new int[nrDSPs];
		for (int i=0; i<nrDSPs; i++)
		{
			config[i] = NULL;
		}
		for (int i=0; i<dspCount; i++)
		{
			if(verbose)
				cout << "initTiling : iteration #" << i << endl; 
			config[i] = target_->createDSP();						
			config[i]->setNrOfPrimitiveDSPs(1);
			
			config[i]->allocatePositions(3*i); // each DSP offers 8 positions
			if(!replace(config, i))
			{
				w=config[i]->getMaxMultiplierWidth();
				h= config[i]->getMaxMultiplierHeight();
				config[i]->setTopRightCorner(vn, vm);
				config[i]->setBottomLeftCorner(vn+w, vm+h);
			}
		}
		
	}
	
	void IntTilingMult::initTiling2(DSP** &config, int dspCount)
	{
		//nrOfShifts4Virtex =2; 
		int w, h;
		target_->getDSPWidths(w, h);
		int min = h;
		int mw,mh;
		mw = vn - 2*getExtraWidth();
		mh = vm - 2*getExtraHeight();
		if(w < h)
		{
			min = w;
			w = h;
			h = min;
		}	
		
		// allocate the maximum number of DSP objects
		config = new DSP*[nrDSPs];
		for (int i=0; i<nrDSPs; i++)
		{	
			config[i] = NULL;
		}
		
		/* NOTE: In case the multiplication is narrower than twice the minimum dimension of a DSP 
		 * then we do not form paris of multipliers
		 */
		if ((vm < 2*min) || (vn < 2*min))
		{
			for (int i=0; i<dspCount; i++)
			{
				config[i] = target_->createDSP();
				config[i]->setNrOfPrimitiveDSPs(1);
				config[i]->allocatePositions(3*i); // each DSP offers 3 positions
				if(!replace(config, i))
				{
					w=config[i]->getMaxMultiplierWidth();
					h= config[i]->getMaxMultiplierHeight();
					config[i]->setTopRightCorner(vn, vm);
					config[i]->setBottomLeftCorner(vn+w, vm+h);
				}
			}
			dsplimit = w*h;
			return; // force exit
		}
		dsplimit = w*h*2;
		// else form paris
		// verify if we have an even or odd nr of DSPs
		bool singleDSP = false;
		if (nrDSPs % 2 == 1)
			singleDSP = true;
		
		int nrDSPsprim = nrDSPs;
			
		// compute new number of DSPs

		numberDSP4Overlap = dspCount = nrDSPs = (int) ceil((double) dspCount/2);

		
		// set shift amount according to target
		int shift = 0;
		if ((target_->getID() == "Virtex4") || (target_->getID() == "Virtex5"))
			shift = 17;
		
		int start = 0; // starting position
		
		
		//~ cout<< "mw "<<mw<<" mh "<<mh<<" w "<<w<<" h "<<h<<"estimate dsps "<<nrDSPsprim<<endl;
		//~ cout<<"h*0.9 + h*4="<<h*0.9 + h*4<<endl;
		//~ cout<<"cond "<<(mw == h*4 || mh ==h*4)<<" tout "<<((mw == h*4 || mh ==h*4) ||(  (h*0.9 + h*4 <=mw) &&  mh>=2*w  ) || (   (h*0.9 + h*4 <=mh) &&  mw>=2*w  ))<<endl;
		//~ cout<<" area= "<<mw*mh<<" area covered by DSPs= "<<h*w*nrDSPsprim<<" condition "<<( mw*mh*0.8>= h*w*nrDSPsprim )<<endl;
		
		
		
		
		if (dspCount >= 16 &&
			((mw == h*4 || mh ==h*4) ||
			(  (h*0.9 + h*4 <=mw) &&  mh>=2*w  ) ||
			(   (h*0.9 + h*4 <=mh) &&  mw>=2*w  ) ||
			( mw*mh>= h*w*nrDSPsprim )
			)) 
		{ // we have more than 16 paris of multipliers we can group 4 such pairs into a super-block
			cout<<"A super DSP was created"<<endl;
			config[start] = new DSP(0, w*2, h*4);
			config[start]->setNrOfPrimitiveDSPs(4);
			config[start]->allocatePositions(3*start); // each DSP offers 3 positions
			if(!replace(config, start))
			{
				int w=config[start]->getMaxMultiplierWidth();
				int h= config[start]->getMaxMultiplierHeight();
				config[start]->setTopRightCorner(vn, vm);
				config[start]->setBottomLeftCorner(vn+w, vm+h);
			}
			start++;
			dspCount -= 3;

			numberDSP4Overlap = nrDSPs = dspCount;

			
			int i;
			for (i=start; i<3; i++)
			{
				config[i] = new DSP(shift, w, h*2);	
				config[i]->setNrOfPrimitiveDSPs(2);
				config[i]->allocatePositions(3*i); // each DSP offers 3 positions
				if(!replace(config, i))
				{
					int w=config[i]->getMaxMultiplierWidth();
					int h= config[i]->getMaxMultiplierHeight();
					config[i]->setTopRightCorner(vn, vm);
					config[i]->setBottomLeftCorner(vn+w, vm+h*2);
				}
			}
			start = i;	
		}

		/*NOTE: if the program entered the previous if clause it will also enter the following 
		 * 	if clause. This is the effect we want to obtain */
		if (dspCount >= 10 		&&
			((mw == h*4 || mh ==h*4) ||
			(  (h*0.9 + h*4 <=mw) &&  mh>=2*w  ) ||
			(   (h*0.9 + h*4 <=mh) &&  mw>=2*w  ) ||
			( mw*mh>= h*w*nrDSPsprim )
			)	
		)
		{ // we have more than 10 paris of multipliers we can group 4 such pairs into a super-block
			cout<<"A super DSP was created"<<endl;
			config[start] = new DSP(0, w*2, h*4);
			config[start]->setNrOfPrimitiveDSPs(4);
			config[start]->allocatePositions(3*start); // each DSP offers 3 positions
			dspCount -= 3;
			numberDSP4Overlap = nrDSPs = dspCount;
			if(!replace(config, start))
			{
				int w=config[start]->getMaxMultiplierWidth();
				int h= config[start]->getMaxMultiplierHeight();
				config[start]->setTopRightCorner(vn, vm);
				config[start]->setBottomLeftCorner(vn+w, vm+h);
			}
			start++;

		}
		
		// initialize all DSP paris except first one
		dspCount--;
		for (int i=start; i<dspCount; i++)
		{
			config[i] = new DSP(shift, w, h*2);	
			config[i]->setNrOfPrimitiveDSPs(2);
			config[i]->allocatePositions(3*i); // each DSP offers 3 positions
			if(!replace(config, i))
			{
				int w=config[i]->getMaxMultiplierWidth();
				int h= config[i]->getMaxMultiplierHeight();
				config[i]->setTopRightCorner(vn, vm);
				config[i]->setBottomLeftCorner(vn+w, vm+h*2);
			}
		}
		
		//initializa last DSP or DSP pair
		if (singleDSP) // then the last position is a single DSP 
		{ // allocate single DSP
			config[dspCount] = target_->createDSP();
			config[dspCount]->setNrOfPrimitiveDSPs(1);
			config[dspCount]->allocatePositions(3*dspCount); // each DSP offers 3 positions
			if(!replace(config, dspCount))
			{
				int w=config[dspCount]->getMaxMultiplierWidth();
				int h= config[dspCount]->getMaxMultiplierHeight();
				config[dspCount]->setTopRightCorner(vn, vm);
				config[dspCount]->setBottomLeftCorner(vn+w, vm+h);
			}
			
		}
		else // then the last position is a DSP pair
		{ // allocate DSP pair
			config[dspCount] = new DSP(shift, w, h*2);
			config[dspCount]->setNrOfPrimitiveDSPs(2);
			config[dspCount]->allocatePositions(3*dspCount); // each DSP offers 3 positions
			if(!replace(config, dspCount))
			{
				int w=config[dspCount]->getMaxMultiplierWidth();
				int h= config[dspCount]->getMaxMultiplierHeight();
				config[dspCount]->setTopRightCorner(vn, vm);
				config[dspCount]->setBottomLeftCorner(vn+w, vm+h*2);
			}
			
		}	
		
	}
	
	DSP** IntTilingMult::neighbour(DSP** config)
	{
		DSP** returnConfig;
		initTiling(returnConfig, nrDSPs);
		// select random DSP to move
		srand (time(NULL));
		int index = rand() % nrDSPs;
		// get multiplier width and height
		int w, h, pos;
		w= returnConfig[index]->getMaxMultiplierWidth();
		h= returnConfig[index]->getMaxMultiplierHeight();
		
		if (index == 0) // we have a special case for the first DSP
		{
			returnConfig[index]->rotate();
			index++;
		}
		
		// for all DSPs except the first one
		
		// nr of possible positions is 3 if multiplier is sqare and 6 if it is rectangular
		int nrPositions = 6;
		
		if (w == h)
			nrPositions = 3;
		
		// starting from the selected DSP move all susequent DSPs to random positions
		for (int i=index; i<nrDSPs; i++)
		{
			// move selected DSP in a random position
			pos = rand() % nrPositions;
			if (pos >= 3) // rotate the multiplier
			{
				config[i]->rotate();
				pos -=3;
			}
			if (replace(returnConfig, i))
			{	
				pos--; // one position was used on replace
				for (int j=0; j<pos; j++) // try to move DSP a number of times
					if (!move(returnConfig, i)) 
					{ // just replace if not posible to move	
						replace(returnConfig, i);
						break;
					}
			}
			else // cannot replace the DSP
				memcpy(returnConfig[i], config[i], sizeof(DSP)); // copy back the initial position
		}
		
		return returnConfig;	
	}
	
	float IntTilingMult::temp(float f)
	{
		return f;
	}
	
	float IntTilingMult::probability(float e, float enew, float t)
	{
		if (enew < e)
			return 1.0;
		else
		{
			float dif = enew-e;
			if (dif < 1)
				return dif*t;
			else
				return dif*t/enew;
		}
	}
	
	void IntTilingMult::simulatedAnnealing()
	{
		// Initial state
		initTiling(globalConfig, nrDSPs);
		int exw = getExtraWidth();
		int exh = getExtraHeight();
		int h = globalConfig[0]->getMaxMultiplierHeight();
		int w = globalConfig[0]->getMaxMultiplierWidth();
		globalConfig[0]->setTopRightCorner(exw, exh);
		globalConfig[0]->setBottomLeftCorner(exw+w-1, exh+h-1);
		for (int i=1; i<nrDSPs; i++)
			replace(globalConfig, i);
			
		display(globalConfig);
		float e = computeCost(globalConfig);// Initial energy.
		bestConfig = new DSP*[nrDSPs];// Allocate blocks
			
		for (int i=0; i<nrDSPs; i++)// Initial "best" solution
		{
			bestConfig[i] = new DSP();
			memcpy(bestConfig[i], globalConfig[i], sizeof(DSP)); 				
		}
		bestCost = e;// Initial "best" cost
		int k = 0;// Energy evaluation count.
		int kmax = 3*nrDSPs*nrDSPs;// Arbitrary limit
		DSP **snew;
		while (k < kmax)// While time left & not good enough:
		{
			// TODO deallocate previous snew;
			snew = neighbour(globalConfig);// Pick some neighbour.
			float enew = computeCost(snew);// Compute its energy.
			
			display(snew);
			
			if (enew < bestCost)// Is this a new best?
			{// Save 'new neighbour' to 'best found'.	
				for(int i=0; i<nrDSPs; i++)
					memcpy(bestConfig[i], snew[i], sizeof(DSP));
				bestCost = enew;                  
			}
			
			if (probability(e, enew, temp(k/kmax)) > rand())// Should we move to it?
			{// Yes, change state.	
				globalConfig = snew; 
				e = enew;         
			}                 
			k = k + 1; // One more evaluation done
		}
		display(bestConfig);
		// Return the best solution found.
		for(int i=0; i<nrDSPs; i++)
			memcpy(globalConfig[i], bestConfig[i], sizeof(DSP));
	}
	
	int IntTilingMult::bindDSPs4Stratix(DSP** config)
	{
		int nrDSPs_ = nrDSPs;
		int DSPcount = nrDSPs_;
	
		for (int i=0; i<nrDSPs_; i++)
			if (config[i] == NULL)
				DSPcount--;
		
			
		for (int i=0; i<DSPcount; i++)
			if (config[i]->getNumberOfAdders() < 3) // serach for an aligned DSP block that can be added with this one
				{
					int xtri, ytri, wx, wy, nrOp;
					bool bound;
					config[i]->getTopRightCorner(xtri, ytri);
					target_->getDSPWidths(wx,wy);
				
					if (verbose)
						cout << "bindDSP4Stratix: DSP #" << i << " has less than 3 operands. Top-right is at ( " << xtri << ", " << ytri << ") width is " << wx << endl;
					DSP** operands;
					DSP** operandsj;
					DSP** opsk;
					for (int j=0; j<DSPcount; j++)
						if (i != j)
							{
								nrOp = config[i]->getNumberOfAdders();
								if (nrOp == 3)
									break;
			
								operands = config[i]->getAdditionOperands();
								bound = false;
								// check if the DSP blocks are bound
								for (int k=0; k<nrOp; k++)
									if (operands[k] == config[j]) // the DSPs are allready bound
										{
											if (verbose)
												cout << "bindDSP4Stratix: DSP #" << j << " has #" << i << " as one of its operands allready." << endl;
											bound = true;
											break;
										}
				
								if (bound)
									continue;
								// if they are not bound
								int xtrj, ytrj, nrOpj;
								config[j]->getTopRightCorner(xtrj, ytrj);
								nrOpj = config[j]->getNumberOfAdders();
				
								if (verbose)
									cout << "bindDSP4Stratix:" << tab << " Checking against DSP #" << j << " Top-right is at ( " << xtrj << ", " << ytrj << ") width is " << wx << endl;
			
								if ((((xtrj-xtri == -wx) && (ytrj-ytri == wy)) || ((xtrj-xtri == wx) && (ytrj-ytri == -wy))) && // if they have the same alignment
									 (nrOpj + nrOp < 3)) // if the DSPs we want to bind have less than 3 other DSPs to add together
									{ // copy the operands from one another and bind them 
					
										if (verbose)
											cout << "bindDSP4Stratix : DSP #" << j << " together with #" << i << " have fewer than 3 operands. We can bind them." << endl;
										operandsj = config[j]->getAdditionOperands();
					
										for (int k=0; k<nrOp; k++)
											{
												operandsj[nrOpj+k] = operands[k];
												// each operand of congif[i] also gets bounded 
												int opcntk = operands[k]->getNumberOfAdders();
												operands[k]->setNumberOfAdders(nrOp+nrOpj+1);
												opsk = operands[k]->getAdditionOperands();
												for (int l=0; l < nrOpj; l++)
													opsk[l+opcntk] = operandsj[l];
												opsk[nrOpj+opcntk] = config[j];
												operands[k]->setAdditionOperands(opsk);
											}
										operandsj[nrOp+nrOpj] = config[i];
										config[j]->setAdditionOperands(operandsj);
										config[j]->setNumberOfAdders(nrOp+nrOpj+1);
					
										for (int k=0; k<nrOpj; k++)
											{
												operands[nrOp+k] = operandsj[k];
												// each operand of congif[j] also gets bounded 
												int opcntk = operandsj[k]->getNumberOfAdders();
												operandsj[k]->setNumberOfAdders(nrOp+nrOpj+1);
												opsk = operandsj[k]->getAdditionOperands();
												for (int l=0; l < nrOp; l++)
													opsk[l+opcntk] = operands[l];
												opsk[nrOp+opcntk] = config[i];
												operandsj[k]->setAdditionOperands(opsk);
											}
										operands[nrOp+nrOpj] = config[j];
										config[i]->setAdditionOperands(operands);
										config[i]->setNumberOfAdders(nrOp+nrOpj+1);
									}
							}
				}
		
		/* We now have:
		 * 	- pairs of DSP objects which have the number of operands set to 1
		 * 	- triplets of DSP objects which have the number of operands set to 2
		 * 	- quadruplets of DSP objects which have the number of operands set to 3 
		 * We keep a counter for each possible number of operands. */
		int pair[3] = {0, 0, 0};
	
	
		for (int i=0; i<DSPcount; i++)
			{
				if (verbose)
					cout << "bindDSP4Stratix : DSP #" << i << " has " << config[i]->getNumberOfAdders() << " adders" << endl;
				pair[config[i]->getNumberOfAdders()-1]++;
			}
		if (verbose)
			{
				cout << "bindDSP4Stratix : one " << pair[0] << endl;
				cout << "bindDSP4Stratix : two " << pair[1] << endl;
				cout << "bindDSP4Stratix : three " << pair[2] << endl;
			}
		return (DSPcount - pair[0]/2 - pair[1]*2/3 - pair[2]*3/4);
	}

	DSP** IntTilingMult::splitLargeBlocks(DSP** config, int &numberOfDSPs)
	{
		int h, w, dspH, dspW, tmp, nrDSPonHeight, nrDSPonWidth, shiftAmount, newNrDSPs=0;
		getTarget()->getDSPWidths(dspW, dspH);
		
		// count total number of DSPs
		for (int i=0; i<numberOfDSPs; i++)
		{
			h = config[i]->getMaxMultiplierHeight();
			w = config[i]->getMaxMultiplierWidth();
			
			if ((h % dspH) != 0) // match width and height
			{
				tmp = dspH;
				dspH = dspW;
				dspW = tmp;
			}
			
			nrDSPonHeight = h/dspH;
			nrDSPonWidth = w/dspW;
			
			newNrDSPs += (nrDSPonHeight*nrDSPonWidth);
		}
		
		DSP** returnConfig = new DSP*[newNrDSPs];
		int index = 0;
		int xtr, xbl, ytr, ybl;
		for (int i=0; i<numberOfDSPs; i++)
		{
			h = config[i]->getMaxMultiplierHeight();
			w = config[i]->getMaxMultiplierWidth();
			shiftAmount = config[i]->getShiftAmount();
			config[i]->getTopRightCorner(xtr, ytr);
			config[i]->getBottomLeftCorner(xbl, ybl);
			
			if ((h % dspH) != 0) // match width and height
			{
				tmp = dspH;
				dspH = dspW;
				dspW = tmp;
			}
			
			nrDSPonHeight = h/dspH;
			nrDSPonWidth = w/dspW;
			// create DSP blocks
			for (int j=0; j<nrDSPonHeight; j++)
				for (int k=0; k<nrDSPonWidth; k++)
				{
					returnConfig[index] = getTarget()->createDSP();
					returnConfig[index]->setTopRightCorner(xtr+k*dspW, ytr+j*dspH);
					returnConfig[index]->setBottomLeftCorner(xtr+(k+1)*dspW-1, ytr+(j+1)*dspH-1);
					returnConfig[index]->setMultiplierHeight(dspH);
					returnConfig[index]->setMultiplierWidth(dspW);
					// take care of shiftings between DSPs
					if (shiftAmount == dspH)
					{
						if (j > 0) 
						{
							returnConfig[index]->setShiftIn(returnConfig[index-(j*nrDSPonWidth)]);
							returnConfig[index-(j*nrDSPonWidth)]->setShiftOut(returnConfig[index]);	
						}
					}
					else if (shiftAmount == dspW)
					{
						if (k > 0)
						{
							returnConfig[index]->setShiftIn(returnConfig[index-1]);
							returnConfig[index-1]->setShiftOut(returnConfig[index]);
						}
					}
					index++;
				}
		}
		numberOfDSPs = newNrDSPs;
		return returnConfig;
	}


	void IntTilingMult::convertCoordinates(int &tx, int &ty, int &bx, int &by)
	{
		tx -= getExtraWidth();
		ty -= getExtraHeight();
		bx -= getExtraWidth();
		by -= getExtraHeight();

		if (bx>=wInX) bx = wInX-1;
		if (by>=wInY) by = wInY-1;
	}

	void IntTilingMult::convertCoordinatesKeepNeg(int &tx, int &ty, int &bx, int &by)
	{
		tx -= getExtraWidth();
		ty -= getExtraHeight();
		bx -= getExtraWidth();
		by -= getExtraHeight();
	}



	int IntTilingMult::multiplicationInDSPs(DSP** config)
	{
		int nrOp = 0;		 			// number of resulting adder operands
		int trx1, try1, blx1, bly1; 	// coordinates of the two corners of a DSP block 
		int fpadX, fpadY, bpadX, bpadY;	// zero padding on both axis
		int extW, extH;					// extra width and height of the tiling grid
		int multW, multH; 				// width and height of the multiplier the DSP block is using
		ostringstream xname, yname, mname, cname, sname;
		DSP** tempc = new DSP*[nrDSPs];	// buffer that with hold a copy of the global configuration
			
		memcpy(tempc, config, sizeof(DSP*) * nrDSPs );
	
		if ( ( target_->getID() == "Virtex4") ||
			 ( target_->getID() == "Virtex5") ||
			 ( target_->getID() == "Spartan3"))  // then the target is A Xilinx FPGA 
			{	
				for (int i=0; i<nrDSPs; i++)
					if (tempc[i] != NULL)
						{
							//~ cout << "At DSP#"<< i+1 << " tempc["<<i<<"]" << endl; 
							DSP* d = tempc[i];
							int j=0;
							int connected = 0;
							bool outside = false;
							
							while (d != NULL)
								{
									connected++;
									d = d->getShiftOut();
								}
							REPORT(DETAILED, "CONNECTED ================ " <<connected);
							d = tempc[i];
							
							
							while (d != NULL)
								{
									d->getTopRightCorner(trx1, try1);
									d->getBottomLeftCorner(blx1, bly1);
									extW = getExtraWidth();
									extH = getExtraHeight();
									
									fpadX = blx1-wInX-extW+1;
									//~ cout << "fpadX = " << fpadX << endl;
									fpadX = (fpadX<0)?0:fpadX;
									
									fpadY = bly1-wInY-extH+1;
									//~ cout << "fpadY = " << fpadY << endl;
									fpadY = (fpadY<0)?0:fpadY;
									
									bpadX = extW-trx1;
									bpadX = (bpadX<0)?0:bpadX;
									
									bpadY = extH-try1;
									bpadY = (bpadY<0)?0:bpadY;
									
									multW = blx1 - trx1 + 1;
									multH = bly1 - try1 + 1;

//									multW = d->getMultiplierWidth();
//									multH = d->getMultiplierHeight();
									
									int startX = blx1-fpadX-extW;
									int endX = trx1+bpadX-extW;
									int startY = bly1-fpadY-extH;
									int endY = try1+bpadY-extH;
									
									if ((startX < endX) || (startY < endY))
									{
										outside = true;
										break;
									}
										
									setCycle(0);
									
									xname.str("");
									xname << "x" << i << "_" << j;
									vhdl << tab << declare(xname.str(), multW+2, true) << " <= \"10\" & " << zg(fpadX,0) << " & X" << range(startX, endX) << " & " << zg(bpadX,0) << ";" << endl;
									
									yname.str("");
									yname << "y" << i << "_" << j;
									vhdl << tab << declare(yname.str(), multH+2, true) << " <= \"10\" & " << zg(fpadY,0) << " & Y" << range(startY, endY) << " & " << zg(bpadY,0) << ";" << endl;
				
									if ((d->getShiftIn() != NULL) && (j>0)) // multiply accumulate
										{
											mname.str("");
											mname << "pxy" << i;
											cname.str("");
											cname << "txy" << i << j;
											setCycle(j);
											vhdl << tab << declare(cname.str(), multW+multH+2) << " <= " << use(xname.str())<<range(multW,0) << " * " << use(yname.str())<<range(multH,0) << ";" << endl;
											vhdl << tab << declare(join(mname.str(),j), multW+multH+2) << " <= (" << use(cname.str())<<range(multW+multH+1,0) << ") + (" <<zg(d->getShiftAmount(),0)<< " &" << use(join(mname.str(), j-1)) << range(multW+multH+1, d->getShiftAmount()) << ");" << endl;	
											if (d->getShiftOut() == NULL) // concatenate the entire partial product
												{
													setCycle(connected);
													sname.seekp(ios_base::beg);
													//sname << zg(wInX-(blx1-fpadX-extW)+wInY-(bly1-fpadY-extH)-2, 0) << " & " << use(join(mname.str(),j)) << range(multW-fpadX + multH-fpadY-1, 0) << " & " << sname.str();
													int nrZeros = wInX-(blx1-extW)-fpadX + wInY-(bly1-extH)-fpadY-3;
													if (nrZeros < 0)
														sname << zg(wInX-(blx1-fpadX-extW)+wInY-(bly1-fpadY-extH)-2, 0) << " & " << use(join(mname.str(),j)) << range(multW-fpadX + multH-fpadY-1, 0) << " & " << sname.str();
//														sname << use(join(mname.str(),j)) << range(multW-fpadX + multH-fpadY-1, 0) << " & " << sname.str();

													else
														sname << zg(wInX-(blx1-extW) + wInY-(bly1-extH)-3, 0) << " & " << use(join(mname.str(),j)) << range(multW + multH, 0) << " & " << sname.str();
	
												}
											else // concatenate only the lower portion of the partial product
												{
													setCycle(connected);
													sname.seekp(ios_base::beg);
													sname << use(join(mname.str(),j)) << range(d->getShiftAmount()-1, 0) << " & " << sname.str();
												}
										}
									else // only multiplication
										{
											mname.str("");
											mname << "pxy" << i << j;
											vhdl << tab << declare(mname.str(), multW+multH+2) << " <= " << use(xname.str())<<range(multW,0) << " * " << use(yname.str())<<range(multH,0) << ";" << endl;
											sname.str("");
											if (d->getShiftOut() == NULL) // concatenate the entire partial product
												{
//													setCycle(connected);
													//sname << zg(wInX-(blx1-fpadX-extW)+wInY-(bly1-fpadY-extH)-2, 0) << " & " << use(mname.str()) << range(multH-fpadY+multW-fpadX-1, bpadX+bpadY)<< " & " << zg(trx1-extW,0) << " & " << zg(try1-extH,0) <<  ";" << endl;
													int nrZeros = wInX-(blx1-extW)-fpadX + wInY-(bly1-extH)-fpadY-2;
													if (nrZeros < 0)
														sname << zg(wInX-(blx1-fpadX-extW)+wInY-(bly1-fpadY-extH)-2, 0) << " & " << use(mname.str()) << range(multH-fpadY+multW-fpadX-1, 0)<< " & " << zg(trx1-extW-bpadX,0) << " & " << zg(try1-extH-bpadY,0) <<  ";" << endl;
													else
														sname << zg(wInX-(blx1-extW) + wInY-(bly1-extH)-2, 0) << " & " << use(mname.str()) << range(multW + multH-1, 0) << " & " << zg(trx1-extW-bpadX,0) << " & " << zg(try1-extH-bpadY,0) <<  ";" << endl;
	
												}
											else // concatenate only the lower portion of the partial product
												{
//													setCycle(connected);
													sname << use(mname.str()) << range(d->getShiftAmount()-1, bpadX+bpadY) << " & " << zg(trx1-extW,0) << " & " << zg(try1-extH,0) << ";" << endl;
												}
										}
				
									// erase d from the tempc buffer to avoid handleing it twice
									for (int k=i+1; k<nrDSPs; k++)
										{
											if ((tempc[k] != NULL) && (tempc[k] == d))
												{
													//~ cout << "tempc[" << k << "] deleted" << endl;
													tempc[k] = NULL;
													break;
												}
										}
				
				
									d = d->getShiftOut();
									j++;
								}
								
							if (outside) continue;	
							sname.seekp(ios_base::beg);
							sname << tab << declare(join("addOpDSP", nrOp),wInX+wInY) << " <= " << sname.str();
							vhdl << sname.str();
							nrOp++;		
						}
		
				return nrOp;
			}
		else // the target is Stratix
		{
				int boundDSPs;  				// number of bound DSPs in a group
				DSP** addOps;					// addition operands bound to a certain DSP
	
				for (int i=0; i<nrDSPs; i++)
					if (tempc[i] != NULL){
							cout << "At DSP#"<< i+1 << " tempc["<<i<<"]" << endl; 
							tempc[i]->getTopRightCorner(trx1, try1);
							tempc[i]->getBottomLeftCorner(blx1, bly1);
							convertCoordinates(trx1, try1, blx1, bly1);

							int trx2,try2,blx2,bly2;
							tempc[i]->getTopRightCorner(trx2, try2);
							tempc[i]->getBottomLeftCorner(blx2, bly2);
							convertCoordinatesKeepNeg(trx2, try2, blx2, bly2);
							multW = blx2-trx2+1;
							multH = bly2-try2+1;
							
							int maxX = blx1;
							int maxY = bly1;
			
							setCycle(0);
							vhdl << tab << declare(join("x",i,"_0"), multW, true) << " <= " << zg(blx2-blx1) << " & X"<<range(blx1, trx1) << ";" << endl;
							vhdl << tab << declare(join("y",i,"_0"), multH, true) << " <= " << zg(bly2-bly1) << " & Y"<<range(bly1, try1) << ";" << endl;

							boundDSPs = tempc[i]->getNumberOfAdders();
							int ext = 0;        // the number of carry bits of the addtion accumulation. 
							if (boundDSPs > 0){ // need to traverse the addition operands list and perform addtion
								ext = (boundDSPs>1)?2:1;
								REPORT(INFO, "boundDSPs = " << boundDSPs);
								nextCycle();
								mname.str("");
								mname << "mult_" << i << "_0";
								vhdl << tab << declare(mname.str(), multW+multH, true) << " <= " << join("x",i,"_0") << " * " << join("y",i,"_0") << ";" << endl;
								addOps = tempc[i]->getAdditionOperands();
			
								/* At most 4 operands */
								for (int j=0; j<3; j++)
									if (addOps[j] == NULL)
										cout << "addOps["<< j << "]=NULL" << endl;
									else
										cout << "addOps["<< j << "]=not null" << endl;
			
								for (int j=0; j<boundDSPs; j++){
										cout << "j = " << j << endl;
										// erase addOps[j] from the tempc buffer to avoid handleing it twice
										for (int k=i+1; k<nrDSPs; k++){
											if ((tempc[k] != NULL) && (tempc[k] == addOps[j])){
												REPORT( DETAILED, "tempc[" << k << "] deleted");
												tempc[k] = NULL;
												break;
											}
										}
				
										addOps[j]->getTopRightCorner(trx1, try1);
										addOps[j]->getBottomLeftCorner(blx1, bly1);
										convertCoordinates(trx1, try1, blx1, bly1);
										multW = addOps[j]->getMaxMultiplierWidth();
										multH = addOps[j]->getMaxMultiplierHeight();
										
										addOps[j]->getTopRightCorner(trx2, try2);
										addOps[j]->getBottomLeftCorner(blx2, bly2);
										convertCoordinatesKeepNeg(trx2, try2, blx2, bly2);

										multW = blx2-trx2+1;
										multH = bly2-try2+1;

										if ( bly2+blx2>maxX+maxY){
											maxY = bly1;
											maxX = blx1;
										}
										
										setCycle(0); ////////////////////////////////////
										vhdl << tab << declare(join("x",i,"_",j+1), multW, true) << " <= " << zg(blx2-blx1) << " & X"<<range(blx1, trx1) << ";" << endl;
										vhdl << tab << declare(join("y",i,"_",j+1), multH, true) << " <= " << zg(bly2-bly1) << " & Y"<<range(bly1, try1) << ";" << endl;


										nextCycle(); ////////////////////////////////////
										vhdl << tab << declare(join("mult_",i,"_",j+1), multW+multH, true) << " <= " << join("x",i,"_",j+1) << " * " << join("y",i,"_",j+1) << ";" << endl;
								}
			
								nextCycle();
								vhdl << tab << declare(join("addDSP", nrOp), multW+multH+ext, true) << " <= ";
			
								for (int j=0; j<boundDSPs; j++){
									vhdl << "(" << zg(ext,0) << " & " << join("mult_",i,"_",j) << ") + "; 
								}
								vhdl << "(" << zg(ext,0) << " & " << join("mult_",i,"_",boundDSPs) << ");" << endl; 
							}else{ // multiply the two terms and you're done
								nextCycle();
								vhdl << tab << declare(join("addDSP", nrOp), multW+multH, true) << " <= " << join("x",i,"_0") << " * " << join("y",i,"_0") << ";" << endl;
							}
							vhdl << tab << declare(join("addOpDSP", nrOp), wInX+wInY) << " <= " << zg(wInX+wInY-(maxX+maxY+2+ext),0) << " & " << join("addDSP", nrOp)<<range( (maxX+maxY-(trx2+try2)+2) +ext-1,0)  << " & " << zg(trx2+try2,0) << ";" << endl;
							nrOp++;
					}
				return nrOp;
		}
	}

	int IntTilingMult::multiplicationInSlices(DSP** config)
	{
		//~ cout<<"Incepe"<<endl;
		int partitions=0;
		int **mat;
		int n,m;
		int count=1;
		vector<SoftDSP*> softConfig;
		//~ n=wInX + 2* getExtraWidth();
		//~ m=wInY + 2* getExtraHeight();
		n=vn;
		m=vm;
		//~ cout<<"width "<<n<<"height "<<m<<endl;
		mat = new int*[m];
		for(int i=0;i<m;i++)
			{
				mat[i] = new int [n];
				for(int j=0;j<n;j++)
					mat[i][j]=0;
			}
		for(int i=0;i<nrDSPs;i++)
			{
				int c1X,c2X,c1Y,c2Y;
		
				config[i]->getTopRightCorner(c1X,c1Y);
				config[i]->getBottomLeftCorner(c2X,c2Y);
				//~ cout<<"DSP #"<<i+1<<"has toprigh ("<<c1X<<","<<c1Y<<") and botomleft ("<<c2X<<","<<c2Y<<")"<<endl;
				c1X=n-c1X-1;
				c2X=n-c2X-1;
				//~ cout<<"new x1 "<<c1X<<" new x2 "<<c2X<<endl;
		
				fillMatrix(mat,n,m,c2X,c1Y,c1X,c2Y,count);
				count++;			
			}
		partitions=0;
		
		for(int i=0;i<m;i++)
			{
				for(int j=0;j<n;j++)
					{
						if(mat[i][j]==0)
							{
								int ver =0;
								int ii=i,jj=j;
								while(ver<6&&(ii<m-1||jj<n-1))
									{
										if(ver<3)
											{
												if(ver==0||ver==1)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=2;							
													}
					
												if(ver==0||ver==2)
													jj++;
					
												if(jj>n-1)
													{
														jj=n-1;
														ver=1;
													}
					
												for(int k=ii,q=jj;k>i-1&&(ver==0||ver==2);k--)
													if(mat[k][q]!=0)
														{
															if(ver==0)
																ver=1;
															else
																ver=3;
															jj--;
														}
						
												for(int k=ii,q=jj;q>j-1&&(ver==0||ver==1);q--)
													if(mat[k][q]!=0)
														{
															if (ver==0)
																ver=2;
															else
																ver=3;
															ii--;
														}
											}
										else
											{
												if(ver==3||ver==5)
													jj++;
					
												if(jj>n-1)
													{
														jj=n-1;
														ver=4;
													}
						
												if(ver==3||ver==4)
													ii++;
												if(ii>m-1)
													{
														ii=m-1;
														ver=5;							
													}
					
												for(int k=ii,q=jj;q>j-1&&(ver==3||ver==4);q--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=5;
															else
																ver=6;
															ii--;
														}
						
												for(int k=ii,q=jj;k>i-1&&(ver==3||ver==5);k--)
													if(mat[k][q]!=0)
														{
															if(ver==3)
																ver=4;
															else
																ver=6;
															jj--;
														}
						
												if(ver==5&&jj==n-1)
													ver=6;
												if(ver==4&&ii==m-1)
													ver=6;
											}
									}
				
								int nj,ni,njj,nii;
								int extH = getExtraHeight();
								int extW = getExtraWidth();
				
				
								if( j >= n-extW || jj < extW || i >= m-extH || ii < extH)
									{
										REPORT(DETAILED, "Partition number "<<count<<" is totally out of the real multiplication bounds. ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<")");
									}
								else
									{
										if( j < extW )
											nj = extW ;
										else
											nj = j;
										if( jj >= n-extW )
											njj = n-extW -1;
										else
											njj = jj;
					
										if( i < extH )
											ni = extH;
										else
											ni = i;
										if( ii >= m-extH )
											nii = m-extH-1;
										else
											nii = ii;
										
										setCycle(0);
										SoftDSP *sdsp = new SoftDSP(wInX-nj+2*extW, nii+1, wInX-njj-2+2*extW, ni-1);
										softConfig.push_back(sdsp);
//										target_->setUseHardMultipliers(false);
										LogicIntMultiplier* mult =  new LogicIntMultiplier(target_, njj-nj+1, nii-ni+1); //unsigned
										ostringstream cname;
										cname << mult->getName() << "_" << partitions;
										mult->changeName(cname.str());
										oplist.push_back(mult);
										// TODO: compute width of x and y + corretc range for X and Y
										vhdl << tab << declare(join("x_",partitions), njj-nj+1, true) << " <= X" << range(wInX-nj-1+extW, wInX-njj-1+extW) << ";" << endl;
										inPortMap(mult, "X", join("x_",partitions));
										vhdl << tab << declare(join("y_",partitions), nii-ni+1, true) << " <= Y" << range(nii-extH, ni-extH) << ";" << endl;
										inPortMap(mult, "Y", join("y_",partitions));
					
										outPortMap(mult, "R", join("result", partitions));
					
										vhdl << instance(mult, join("Mult", partitions));

										syncCycleFromSignal(join("result", partitions));
										
										vhdl << tab << declare(join("addOpSlice", partitions), wInX+wInY) << " <= " << zg(wInX+wInY-(wInX-nj-1+extW+nii-extH)-2, 0) << " & " << join("result", partitions) << " & " << zg(wInX-njj-1+extW+ni-extH, 0) << ";" << endl;
										REPORT(DETAILED, "Partition number "<<count<<" with bounds. ("<<j<<" , "<<i<<" , "<<jj<<" , "<<ii<<") has now bounds ("<<nj<<" , "<<ni<<" , "<<njj<<" , "<<nii<<")");
										REPORT(DETAILED, "partitions " << partitions << " @ cycle " << getCurrentCycle());
										
										partitions++;
									}
				
								fillMatrix(mat,n,m,j,i,jj,ii,count);
								count++;
				
							}
					}
	
			}
	
		printConfiguration(bestConfig, softConfig);
		//de verificat
		
		//cout<<"Count "<<count<<" Partitions "<<partitions<<endl;
		
		//partitions =count -partitions;
		 
		
		//~ char af;
		//~ int afi;
		//~ for(int i=0;i<m;i++)
		//~ {
		//~ for(int j=0;j<n;j++)
		//~ {
		//~ if(mat[i][j]<10)
		//~ afi=mat[i][j];
		//~ else
		//~ afi=mat[i][j]+7;
		//~ af=(int)afi+48;
		//~ cout<<af;
		//~ }
		//~ cout<<endl;
		//~ }
	
		//~ cout<<"gata"<<endl;
		
		//~ for(int i=0;i<nrDSPs;i++)
		//~ {
			//~ if(config[i]->getShiftOut()!=NULL)
			//~ {
				//~ config[i]->getShiftOut()->getTopRightCorner(n,m);
				//~ cout<<"There is a shift connection from DSP# "<<i<<" to DSP with coordinates "<<n<<","<<m<<endl;
			//~ }
		//~ }
	
		for(int ii=0;ii<m;ii++)
			delete[](mat[ii]);
	
		delete[] (mat);
		
		return partitions;
	}	

		void IntTilingMult::outputVHDL(std::ostream& o, std::string name) {
		licence(o);
		o << "library ieee; " << endl;
		o << "use ieee.std_logic_1164.all;" << endl;
		o << "use ieee.std_logic_arith.all;" << endl;
		if  ( (target_->getID() == "Virtex4") ||
		      (target_->getID() == "Virtex5") ||
		      (target_->getID() == "Spartan3"))  // then the target is a Xilinx FPGA
			{
		o << "use ieee.std_logic_signed.all;" << endl;
		}else{
		o << "use ieee.std_logic_unsigned.all;" << endl;
		}
		
		o << "library work;" << endl;
		outputVHDLEntity(o);
		newArchitecture(o,name);
		o << buildVHDLComponentDeclarations();	
		o << buildVHDLSignalDeclarations();
		beginArchitecture(o);		
		o<<buildVHDLRegisters();
		o << vhdl.str();
		endArchitecture(o);
	}
	
	void IntTilingMult::printConfiguration(DSP** configuration, vector<SoftDSP*> softDSPs){
		ofstream fig;
		fig.open ("tiling.fig", ios::trunc);
		fig << "#FIG 3.2  Produced by xfig version 3.2.5a" << endl;
		fig << "Landscape" << endl;
		fig << "Center" << endl;
		fig << "Metric" << endl;
		fig << "A4      " << endl;
		fig << "100.00" << endl;
		fig << "Single" << endl;
		fig << "-2" << endl;
		fig << "1200 2" << endl;
	
		if (configuration!=NULL){
			int i=0;
			int xB,xT,yB,yT;
			for(i=0; i<nrDSPs; i++){
				configuration[i]->getTopRightCorner(xT,yT);
				configuration[i]->getBottomLeftCorner(xB,yB);
				REPORT(DETAILED, "HARD DSP Top right = " << xT << ", " << yT << " and bottom left = " << xB << ", " <<yB);
				fig << " 2 2 0 1 0 7 50 -1 -1 0.000 0 0 -1 0 0 5 " << endl;
				fig << "	  " << (-xB+getExtraWidth()-1)*45 << " " << (yT-getExtraHeight())*45 
				         << " " << (-xT+getExtraWidth())*45 << " " << (yT-getExtraHeight())*45 
				         << " " << (-xT+getExtraWidth())*45 << " " << (yB-getExtraHeight()+1)*45 
				         << " " << (-xB+getExtraWidth()-1)*45 << " " << (yB-getExtraHeight()+1)*45 
				         << " " << (-xB+getExtraWidth()-1)*45 << " " << (yT-getExtraHeight())*45 << endl;
				
				int dpx = (-(xT+xB)/2+getExtraWidth()-1)*45;
				int dpy = ((yB+yT)/2-getExtraHeight())*45;         
				REPORT(DETAILED, "x="<<dpx<<" y="<<dpy);
				fig << " 4 1 0 50 -1 0 12 0.0000 4 195 630 "<<dpx<<" "<<dpy<<" DSP"<<i<<"\\001" << endl;

				//annotations
				fig << " 4 1 0 50 -1 0 7 0.0000 4 195 630 "<<(-xB+getExtraWidth())*45<<" "<<-45<<" "<<xB-getExtraWidth()<<"\\001" << endl;
				fig << " 4 1 0 50 -1 0 7 0.0000 4 195 630 "<<(-xT+getExtraWidth()-1)*45<<" "<<-45<<" "<<xT-getExtraWidth()<<"\\001" << endl;
				fig << " 4 0 0 50 -1 0 7 0.0000 4 195 630 "<<45<<" "<<(yT-getExtraHeight()+2)*45<<" "<<yT-getExtraHeight()<<"\\001" << endl;
				fig << " 4 0 0 50 -1 0 7 0.0000 4 195 630 "<<45<<" "<<(yB-getExtraHeight()+1)*45<<" "<<yB-getExtraHeight()<<"\\001" << endl;


//				fig << "	  " << (-xB+getExtraWidth()-1)*45 << " " << (yT-getExtraHeight())*45 
//		         << " " << (-xT+getExtraWidth())*45 << " " << (yT-getExtraHeight())*45 
//		         << " " << (-xT+getExtraWidth())*45 << " " << (yB-getExtraHeight()+1)*45 
//		         << " " << (-xB+getExtraWidth()-1)*45 << " " << (yB-getExtraHeight()+1)*45 




			}
		}

		int xB,xT,yB,yT;
		for (unsigned k=0; k < softDSPs.size(); k++){
			softDSPs[k]->trim(vnme, vmme);
			softDSPs[k]->getTopRightCorner(xT,yT);
			softDSPs[k]->getBottomLeftCorner(xB,yB);
			
			REPORT(DETAILED,  "SOFT DSP Top right = " << xT << ", " << yT << " and bottom left = " << xB << ", " <<yB);
			fig << " 2 2 0 1 0 7 50 -1 19 0.000 0 0 -1 0 0 5 " << endl;
			fig << "	  " << (-xB+getExtraWidth()-1)*45 << " " << (yT-getExtraHeight())*45 
			<< " " << (-xT+getExtraWidth())*45 << " " << (yT-getExtraHeight())*45 
			<< " " << (-xT+getExtraWidth())*45 << " " << (yB-getExtraHeight()+1)*45 
			<< " " << (-xB+getExtraWidth()-1)*45 << " " << (yB-getExtraHeight()+1)*45 
			<< " " << (-xB+getExtraWidth()-1)*45 << " " << (yT-getExtraHeight())*45 << endl;
			int dpx = (-(xT+xB)/2+getExtraWidth()-1)*45;
			int dpy = ((yB+yT)/2-getExtraHeight())*45;         
			REPORT(DETAILED,  "x="<<dpx<<" y="<<dpy);
			fig << " 4 1 0 50 -1 0 12 0.0000 4 195 630 "<<dpx<<" "<<dpy<<" M"<<k<<"\\001" << endl;
			
			xT++;
			yT++;
			xB++;
			yB++;
			int tmp;
			tmp=xT;
			xT=xB;
			xB=tmp;
			tmp=yT;
			yT=yB;
			yB=tmp;
			
			//annotations
			fig << " 4 1 0 50 -1 0 7 0.0000 4 195 630 "<<(-xB+getExtraWidth())*45<<" "<<-45<<" "<<xB-getExtraWidth()<<"\\001" << endl;
			fig << " 4 1 0 50 -1 0 7 0.0000 4 195 630 "<<(-xT+getExtraWidth()-1)*45<<" "<<-45<<" "<<xT-getExtraWidth()<<"\\001" << endl;
			fig << " 4 0 0 50 -1 0 7 0.0000 4 195 630 "<<45<<" "<<(yT-getExtraHeight()+2)*45<<" "<<yT-getExtraHeight()<<"\\001" << endl;
			fig << " 4 0 0 50 -1 0 7 0.0000 4 195 630 "<<45<<" "<<(yB-getExtraHeight()+1)*45<<" "<<yB-getExtraHeight()<<"\\001" << endl;
			
		}
		
		fig << "		2 2 1 1 0 7 50 -1 -1 4.000 0 0 -1 0 0 5" << endl;
		fig << "	  " << (-wInX)*45 << " " << 0 
         << " " << 0 << " " << 0  
         << " " << 0 << " " << (wInY)*45 
         << " " << (-wInX)*45 << " " << (wInY)*45 
         << " " << (-wInX)*45 << " " << 0 << endl;

		//big X and Y
		fig << " 4 1 0 50 -1 0 16 0.0000 4 195 630 "<<-wInX/2*45<<" "<<-3*45<<" X\\001" << endl;
		fig << " 4 0 0 50 -1 0 16 0.0000 4 195 630 "<<3*45<<" "<<wInY/2*45<<" Y\\001" << endl;

		
		fig.close();
		int hh,ww,aa;
		getTarget()->getDSPWidths(hh,ww);
		aa = hh * ww;
		REPORT(INFO, " DSP area is: " << aa ); 
		
		
		if (configuration!=NULL){
			int i=0;
			int xB,xT,yB,yT;
			int underutilized = 0;
			int sunderutilized = 0;

			
			for(i=0; i<nrDSPs; i++){
				configuration[i]->getTopRightCorner(xT,yT);
				configuration[i]->getBottomLeftCorner(xB,yB);
				
				yT=yT-getExtraHeight();
				yB=yB-getExtraHeight();
				xT=xT-getExtraWidth();
				xB=xB-getExtraWidth();

				xB = min(xB, wInX-1);
				yB = min(yB, wInY-1);
				
				if ( float((xB-xT+1)*(yB-yT+1))< float(aa)) {
					if ( float((xB-xT+1)*(yB-yT+1))/float(aa) < 0.5 ){
						REPORT(INFO, "HARD DSP is SEVERELY under-utilized, just " << float((xB-xT+1)*(yB-yT+1))/float(aa) << "%");
						sunderutilized++;
					}else{
						REPORT(INFO, "HARD DSP utilized " << float((xB-xT+1)*(yB-yT+1))/float(aa) << "%");
						underutilized++;
					}
				}
			}
			REPORT(INFO, "********************************************************************************");
			REPORT(INFO, "*      underutilized = " << underutilized  + sunderutilized);
			REPORT(INFO, "*      suggested ratio = " << (float(nrDSPs)*ratio - float(sunderutilized)) / float(nrDSPs));
			REPORT(INFO, "********************************************************************************");
		}
		
		
	}

}
