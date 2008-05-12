/*
 * Multiplexer for FloPoCo
 *
 * Author : Bogdan Pasca
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "Mux.hpp"

using namespace std;

/**
 * The Mux constructor
 * @param[in]		target	the target device
 * @param[in]		wIn			the width of the inputs which are to be multiplexed
 * @param[in]		n     	the number of inputs that need to be multiplexed
**/ 
Mux::Mux(Target* target, int wIn, int n):
	Operator(target), wIn(wIn), n(n) {
	ostringstream name;

	/* Set up the name of the entity */
	name <<"Mux_"<<n<<"to"<<1<<"_"<<wIn<<"bits";	
	unique_name=name.str();
	
	wAddr = int(ceil(log2(n)));
	/* Set up the I/O signals of of the entity */
	add_input ("Input", n*wIn );
	add_input ("Sel",  wAddr); //the select line of the multiplexer
	add_output("Output", wIn); 
	
	//handle the possible unavailable input parameters of the user
	if (target->is_pipelined())
		cerr<<"Warning: this operator is only combinational."<<endl;
	
  /* This operator is combinatorial.*/
	set_combinatorial();
}



/**
 * Mux destructor
 */
Mux::~Mux() {
}



/**
 * Method belonging to the Operator class overloaded by the Mux class
 * @param[in,out] o     the stream where the current architecture will be outputed to
 * @param[in]     name  the name of the entity corresponding to the architecture generated in this method
 **/
void Mux::output_vhdl(std::ostream& o, std::string name) {
	int i,k,zeroPadding;
	ostringstream doubleQuote;
  ostringstream singleQuote;
  ostringstream quote;
  ostringstream busOut;
  ostringstream wireOut;
  ostringstream interfaceOut;
  
  //define the single quote type and the double quote type 
  doubleQuote<<"\"";
  singleQuote<<"\'";
  //when the selector address widht is 1 bit, then single quotes are required for the case, otherwise double quotes must be used
  quote<<((wAddr==1)?singleQuote.str():doubleQuote.str());
  
  Licence(o,"Bogdan Pasca (2008)");
	Operator::StdLibs(o);
	output_vhdl_entity(o);
  new_architecture(o, name);
	output_vhdl_signal_declarations(o);
  begin_architecture(o);
		
	//define the process of the multiplexer
	o<<tab<<"process (Sel,Input)"<<endl;
	o<<tab<<"begin"<<endl;
  o<<tab<<"case Sel is"<<endl;
  
  //iterating throughout selections of the multiplexer 
  for (i=0;i<n;i++){
  	ostringstream binVal;
  	
  	//convert the decimal number i to binary
  	binary(binVal,i);
  	 	
  	//determine the number of zeros needed for extending the result of the conversion the a binary number on wAddr
  	zeroPadding = wAddr - binVal.str().length();
  	
  	//create the zero string
  	ostringstream zeros;
  	for(k=0;k<zeroPadding;k++)
  		zeros<<"0";
  	
  	//initialization
  	busOut.str("");
  	wireOut.str("");
  	interfaceOut.str("");
	
		//due to the differentiation between bus and wire, function of the input width, the assignment of the output is different
		busOut<<"Input("<<wIn*(i+1)-1<<" downto "<<wIn*i<<")";
		wireOut<<"Input("<<wIn*i<<")";
		interfaceOut<<((wIn==1)?wireOut.str():busOut.str());
   	
  	o<<tab<<"when "<<quote.str()<<zeros.str()<<binVal.str()<<quote.str()<<" => Output <="<<interfaceOut.str()<<";"<<endl;
  }
  //default case; this will be discarded by the synthetizer
  o<<tab<<"when others => Output <= "<<interfaceOut.str()<<";"<<endl;
  o<<"end case;"<<endl;
	o<<"end process;"<<endl;
  
	end_architecture(o);
} 


/** Converts a positive decimal number in binary format 
 *@param[in,out] o the stream where the string containing the binary representation of the decimal number is outputed
 *@param[in] number the decimal number to be converted to binary
*/
void Mux::binary(std::ostream& o, int number) {
	int remainder;
	
	if(number <= 1) {
		o << number;
	}

	remainder = number%2;
	binary(o, number >> 1);    
	o << remainder;
}

