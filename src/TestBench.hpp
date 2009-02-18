#ifndef __TESTBENCH_HPP
#define __TESTBENCH_HPP

/**
 * Creates a TestBench, which tests a certain Operator.
 * The test cases are generated by the unit under test (UUT).
 */
class TestBench : public Operator
{
public:
	/**
	 * Creates a TestBench.
	 * @param target The target architecture
	 * @param op The operator which is the UUT
	 * @param n Number of tests
	 */
	TestBench(Target *target, Operator *op, int n);
	
	/** Destructor */
	~TestBench();
	
	/** Method belonging to the Operator class overloaded by the Wrapper class
	 * @param[in,out] o     the stream where the current architecture will be outputed to
	 * @param[in]     name  the name of the entity corresponding to the architecture generated in this method
	 **/
	void outputVHDL(ostream& o, string name);

private:
	Operator *op_; /**< The unit under test UUT */
	int       n_;   /**< The parameter from the constructor */
	TestCaseList tcl_; /**< Test case list */

};
#endif

