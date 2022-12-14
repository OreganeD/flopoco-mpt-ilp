#pragma once
#include <memory>

#include "Target.hpp"
#include "Operator.hpp"

namespace flopoco {
	class BaseMultiplierCategory {
		public:
			// Get the range from this category
			int getMaxWordSizeLargeInputUnsigned() const {return maxWordSizeLargeInputUnsigned_;}
			int getMaxWordSizeSmallInputUnsigned() const {return maxWordSizeSmallInputUnsigned_;}

			/**
			 * @brief Compute the maximum size of the second input for a given configuration
			 * @param The first input size
			 * @param Is the first input signed ?
			 * @param Is the second input signed
			 * @return The maximum size for the second input, 0 if the configuration is not realisable
			 */
			int getMaxSecondWordSize (int firstW, bool isW1Signed, bool isW2signed) const;

			BaseMultiplierCategory(
                    int wX,
                    int wY,
                    bool isSignedX=false,
                    bool isSignedY=false,
                    int shape_para=-1,
					string type="undefined!",
					bool rectangular=true,
                    const vector<int> &output_weights = vector<int>()
				):  tile_param{parametrize(wX, wY, isSignedX, isSignedY, shape_para, type, rectangular, output_weights)},
				    maxWordSizeLargeInputUnsigned_{wX},
					maxWordSizeSmallInputUnsigned_{wY},
					deltaWidthUnsignedSigned_{0},
                    rectangular{rectangular},
                    type_{type},
                    output_weights{output_weights}
				{}

			int getDeltaWidthSigned() const { return deltaWidthUnsignedSigned_; }
			string getType() const {return type_;}
            double getBitHeapCompressionCostperBit();

            /**
             * @brief Returns true if the x-input supports signed when the multiplier is instantiated, every tile that
             * supports signed multiplication should implement this function so that the tiling heuristic can avoid
             * placing tiles without signed support at the bottom and left |_ edge of the area to be tiled.
             * @return true if there is signed support for the x-input
             */
            virtual bool signSupX(){return false;}

            /**
             * @brief Returns true if the y-input supports signed when the multiplier is instantiated, every tile that
             * supports signed multiplication should implement this function so that the tiling heuristic can avoid
             * placing tiles without signed support at the bottom and left |_ edge of the area to be tiled.
             * @return true if there is signed support for the y-input
             */
            virtual bool signSupY(){return false;}

			class Parametrization{
				public:
					Operator *generateOperator(
							Operator *parentOp,
							Target *target
						) const {
						return bmCat_->generateOperator(parentOp, target, *this);
					}

                    unsigned int getMultXWordSize() const {return wX_;}
                    unsigned int getMultYWordSize() const {return wY_;}
                    unsigned int getTileXWordSize() const {return (isFlippedXY_) ? wY_ : wX_;}
                    unsigned int getTileYWordSize() const {return (isFlippedXY_) ? wX_ : wY_;}
					unsigned int getOutWordSize() const;
					bool shapeValid(int x, int y) const;
                    int getRelativeResultLSBWeight() const;
                    int getRelativeResultMSBWeight() const;
                    bool isSignedMultX() const {return isSignedX_;}
                    bool isSignedMultY() const {return isSignedY_;}
					bool isFlippedXY() const {return isFlippedXY_;}
					bool isSquarer() const {return bmCat_->isSquarer();}
					int getShapePara() const {return shape_para_;}
				    string getMultType() const {return bmCat_->getType();}
                    Parametrization tryDSPExpand(int m_x_pos, int m_y_pos, int wX, int wY, bool signedIO);
                    Parametrization setSignStatus(int m_x_pos, int m_y_pos, int wX, int wY, bool signedIO);
                    Parametrization shrinkFitDSP(int m_x_pos, int m_y_pos, int wX, int wY);
                    vector<int> getOutputWeights(){return output_weights;}
                    bool crossesSquarerDiagonal(int anchor_x, int anchor_y) const;
                    void setTilingWeight(int);
                    int getTilingWeight(void);

            private:
					Parametrization(
							unsigned int  wX,
							unsigned int  wY,
							BaseMultiplierCategory const * multCategory,
							bool isSignedX=false,
							bool isSignedY=false,
							bool isFlippedXY=false,
							int shape_para=-1,
                            vector<int> output_weights = vector<int>(),
                            int tilingWeight=1
						):wX_{wX},
						wY_{wY},
						isSignedX_{isSignedX},
						isSignedY_{isSignedY},
						isFlippedXY_{isFlippedXY},
						shape_para_{shape_para},
						bmCat_{multCategory},
                        output_weights{output_weights},
                        tilingWeight{tilingWeight}{}

					unsigned int wX_;
					unsigned int wY_;
					bool isSignedX_;
					bool isSignedY_;
					bool isFlippedXY_;
					int shape_para_;
					BaseMultiplierCategory const * bmCat_;
                    vector<int> output_weights;
                    int tilingWeight = 1;
				friend BaseMultiplierCategory;
			};

            virtual int getDSPCost() const = 0;
            virtual double getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO);
            virtual int ownLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO);
            virtual unsigned int getArea() {return tile_param.wX_*tile_param.wY_;}
            virtual bool isVariable() const { return false; }
            virtual bool isIrregular() const { return false; }
            virtual bool isKaratsuba() const { return false; }
            virtual bool isSquarer() const { return false; }
            float efficiency() {return getArea()/cost();}
            float cost() {return getLUTCost(0, 0, 48, 48, false);}

			virtual bool shapeValid(Parametrization const & param, unsigned x, unsigned y) const;
            virtual bool shapeValid(int x, int y);
            bool shape_contribution(int x, int y, int shape_x, int shape_y, int wX, int wY, bool signedIO, bool squarer=false);
            virtual float shape_utilisation(int shape_x, int shape_y, int wX, int wY, bool signedIO);

            virtual int getRelativeResultLSBWeight(Parametrization const & param) const;
            virtual int getRelativeResultMSBWeight(Parametrization const & param) const;
            virtual int getRelativeResultMSBWeight(Parametrization const & param, bool isSignedX, bool isSignedY) const;

			virtual Operator* generateOperator(
					Operator *parentOp,
					Target* target,
					Parametrization const &parameters
				) const = 0;

            Parametrization parametrize(int wX, int wY, bool isSignedX, bool isSignedY, int shape_para =-1, string type="undefined!",
                                        bool rectangular=true,
                                        const vector<int> &output_weights = vector<int>()) const;
            Parametrization getParametrisation(void) {return tile_param;}
            unsigned wX(void) {return tile_param.wX_;}
            unsigned wY(void) {return tile_param.wY_;}
            int wX_DSPexpanded(int m_x_pos, int m_y_pos, int wX, int wY, bool signedIO);
            int wY_DSPexpanded(int m_x_pos, int m_y_pos, int wX, int wY, bool signedIO);
            virtual int isSuperTile(int rx1, int ry1, int lx1, int ly1, int rx2, int ry2, int lx2, int ly2) {return 0;}
            void setTarget(Target* target){ this->target = target;}
            void setTilingWeight(int weight) { tile_param.tilingWeight = weight;}

	    protected:
            Target* target = NULL;

		private:
            BaseMultiplierCategory::Parametrization tile_param;

			int maxWordSizeLargeInputUnsigned_;
			int maxWordSizeSmallInputUnsigned_;
			int deltaWidthUnsignedSigned_;
            const bool rectangular;
            string type_; /**< Name to identify the corresponding base multiplier in the solution (for debug only) */
            vector<int> output_weights;
    };

	typedef BaseMultiplierCategory::Parametrization BaseMultiplierParametrization;
}
