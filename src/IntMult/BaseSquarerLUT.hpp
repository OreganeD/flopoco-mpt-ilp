#include "Operator.hpp"
#include "BaseMultiplierCategory.hpp"

namespace flopoco
{

    class BaseSquarerLUT : public BaseMultiplierCategory
    {
            public:

                BaseSquarerLUT(
                        int wIn
                        ) : BaseMultiplierCategory{
                    wIn,
                    wIn,
                    false,
                    false,
                    -1,
                    "BaseSquarer_" + to_string(wIn) + "x" + to_string(wIn)
                }{
                    this->wIn = wIn;
                }

                bool isIrregular() const override { return true;}
                int getDSPCost() const final {return 0;}
                double getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO) override;
                int ownLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO) override;
                unsigned getArea(void) override;

                bool signSupX() override {return true;}
                bool signSupY() override {return true;}
                bool isSquarer() const override { return true; }
                bool shapeValid(int x, int y) override;
                bool shapeValid(Parametrization const & param, unsigned x, unsigned y) const override;
                int getRelativeResultMSBWeight(Parametrization const& param) const override;

                Operator *generateOperator(Operator *parentOp, Target *target, Parametrization const & params) const final;

                /** Factory method */
                static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target , vector<string> &args);
                /** Register the factory */
                static void registerFactory();
    private:
        int wIn, wR;

    };

    class BaseSquarerLUTOp : public Operator {
    public:
        BaseSquarerLUTOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int wIn);

        void emulate(TestCase* tc);
        static TestList unitTest(int index);

    private:
        int wIn;
        bool isSigned;
        mpz_class function(int xy);
    };

}