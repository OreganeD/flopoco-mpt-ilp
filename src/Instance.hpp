/*
 * An instance of an Operator.
 * Instance is to Operator what a VHDL architecture is to an entity
 */

#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vector>
#include <map>

#include "utils.hpp"

#include "Operator.hpp"

using namespace std;

namespace flopoco {

	class Instance {

	public:

		/**
		 * Instance constructor
		 */
		Instance(std::string name, Operator* op);

		/**
		 * Instance constructor
		 */
		Instance(std::string name, Operator* op, map<std::string, Signal*> inPortMap, map<std::string, Signal*> outPortMap);

		/**
		 * Instance constructor
		 */
		~Instance();


		/**
		 * Return the name of the operator
		 */
		std::string getName();

		/**
		 * Set the name of the operator
		 */
		void setName(std::string name);

		/**
		 * Return the the operator for this instance
		 */
		Operator* getOperator();

		/**
		 * Set the the operator for this instance
		 */
		void setOperator(Operator* op);

		/**
		 * Add a new input port connection
		 */
		void addInputPort(std::string portName, Signal* connectedSignal);

		/**
		 * Add a set of new input port connections
		 */
		void addInputPorts(map<std::string, Signal*> signals);

		/**
		 * Remove an input port connection
		 */
		void removeInputPort(std::string portName);

		/**
		 * Clear the input port connections
		 */
		void clearInputPorts();

		/**
		 * Add a new output port connection
		 */
		void addOutputPort(std::string portName, Signal* connectedSignal);

		/**
		 * Add a set of new output port connections
		 */
		void addOutputPorts(map<std::string, Signal*> signals);

		/**
		 * Remove an output port connection
		 */
		void removeOutputPort(std::string portName);

		/**
		 * Clear the output port connections
		 */
		void clearOutputPorts();


	protected:

		std::string name;                           /*< the name of the instance */
		Operator* op;                               /*< the operator for this instance */
		map<std::string, Signal*> inPortMap;        /*< the port mapping for the inputs of the operator */
		map<std::string, Signal*> outPortMap;       /*< the port mapping for the outputs of the operator */


	private:

	};

} //namespace

#endif /* INSTANCE_HPP */
