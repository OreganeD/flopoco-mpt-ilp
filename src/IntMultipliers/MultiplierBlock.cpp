#include "MultiplierBlock.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>	
#include <cstdlib>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "../Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAdder.hpp"
#include "IntMultiAdder.hpp"
#include "IntAddition/NewCompressorTree.hpp"
#include "IntAddition/PopCount.hpp"
#include "utils.hpp"
#include<vector>
#include<list>
using namespace std;


namespace flopoco{


		MultiplierBlock::MultiplierBlock(int wX, int wY, int topX, int topY, bool goToDSP_, int weightShift_, int cycle, MultiplierBlock* previous, MultiplierBlock* next) :
			wX(wX), wY(wY), topX(topX), topY(topY), goToDSP(goToDSP_), weightShift(weightShift_)
		{
			cycle=cycle;
			weight=topX+topY-weightShift;
			previous=NULL;
			next=NULL;
			
			
		}


		bool MultiplierBlock::operator>= (MultiplierBlock* b){
		if ((weight>=b->getWeight())) 
			return true;
		else
			return false;
	} 

		void MultiplierBlock::setSignalName(string name)
		{
			signalName=name;
		}

		void MultiplierBlock::setSignalLength(int length)
		{
			signalLength=length;
		}

		string MultiplierBlock::getSigName()
		{
			return signalName;
		}

		int MultiplierBlock::getSigLength()
		{
			return signalLength;
		}

		int MultiplierBlock::getwX()
		{
			return wX;
		}

		int MultiplierBlock::getwY()
		{
			return wY;
		}

		int MultiplierBlock::gettopX()
		{
			return topX;
		}

		int MultiplierBlock::gettopY()
		{
			return topY;
		}


		int MultiplierBlock::getbotX()
		{
			return topX+wX;
		}

		int MultiplierBlock::getbotY()
		{
			return topY+wY;
		}


		int MultiplierBlock::getCycle()
		{
			return cycle;
		}
	
		bool MultiplierBlock::getgoToDSP()
		{
			return goToDSP;
		}


		void MultiplierBlock::setNext(MultiplierBlock* b)
		{
			next=b;
			//b->previous=this;
		}

		MultiplierBlock* MultiplierBlock::getNext()
		{
			return next;
		}

		

		MultiplierBlock* MultiplierBlock::getPrevious()
		{
			return previous;
		}

		void MultiplierBlock::setPrevious(MultiplierBlock* b)
		{
			previous=b;
			//b->previous=this;
		}



		bool MultiplierBlock::canBeChained(MultiplierBlock* next)
		{
			//for now just the stupid chaining
			if(	((this->topX==next->gettopX()) &&
					(this->topY==next->gettopY()+17)) ||
				((this->topY==next->gettopY()) &&
					(this->topX==next->gettopX()+17)) )

				return true;
			else return false;

		}


        /** i don't think it will be used anymore here...
		void MultiplierBlock::generateVHDLforDSP(int nr,int i)
		{
		//	REPORT(DETAILED,"dsp");
			
			stringstream s;
			int topX=gettopX();
			int topY=gettopY();
			int botX=topX+getwX()-1;
			int botY=topY+getwY()-1;
		
			//op->vhdl << tab << op->declare(join("DSP",i,"_",nr), getwX()+getwY()) << " <= XX"<<range(botX,topX)<<" * YY"
			//<<range	(botY,topY)<<";"<<endl;

			s<<join("DSP",i,"_",nr);
			
			setSignalName(s.str());
			setSignalLength(getwX()+getwY());
			//REPORT(DETAILED,"dspout");
		}
		*/

	



     

}