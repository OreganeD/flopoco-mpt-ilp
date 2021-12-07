#include "MultiplierTileCollection.hpp"
#include "BaseMultiplierDSP.hpp"
#include "BaseMultiplierLUT.hpp"
#include "BaseMultiplierXilinx2xk.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"
#include "BaseMultiplierDSPKaratsuba.hpp"
#include "BaseSquarerLUT.hpp"

using namespace std;
namespace flopoco {

    MultiplierTileCollection::MultiplierTileCollection(Target *target, BaseMultiplierCollection *bmc, int mult_wX, int mult_wY, bool superTile, bool use2xk, bool useirregular, bool useLUT, bool useDSP, bool useKaratsuba, bool squarer):squarer{squarer} {
        //cout << bmc->size() << endl;
        int tilingWeights[4] = {1, -1, 2, -2};
        for(int w = 0; w < ((squarer)?4:1); w++){

            if(useDSP) {
                addBaseTile(target, new BaseMultiplierDSP(24, 17, 1), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierDSP(17, 24, 1), tilingWeights[w]);
            }

            if(useLUT) {
                addBaseTile(target, new BaseMultiplierLUT(3, 3), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierLUT(2, 3), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierLUT(3, 2), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierLUT(1, 2), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierLUT(2, 1), tilingWeights[w]);
                addBaseTile(target, new BaseMultiplierLUT(1, 1), tilingWeights[w]);
            }

            if(superTile){
                for(int i = 1; i <= 12; i++) {
                    addSuperTile(target, new BaseMultiplierDSPSuperTilesXilinx((BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE) i), tilingWeights[w]);
                }
            }

            if(use2xk){
                variableTileOffset = 4;
                for(int x = variableTileOffset; x <= mult_wX; x++) {
                    addVariableXTile(target, new BaseMultiplierXilinx2xk(x, 2), tilingWeights[w]);
                }

                for(int y = variableTileOffset; y <= mult_wY; y++) {
                    addVariableYTile(target, new BaseMultiplierXilinx2xk(2, y), tilingWeights[w]);
                }
            }

            if(useirregular){
                for(int i = 1; i <= 8; i++) {
                    addBaseTile(target, new BaseMultiplierIrregularLUTXilinx((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE) i), tilingWeights[w]);
                }
            }

            if(useKaratsuba) {
                unsigned int min = std::min((mult_wX - 16) / 48, (mult_wY - 24) / 48);
                for(unsigned int i = 0; i <= min; i++) {
                    addBaseTile(target, new BaseMultiplierDSPKaratsuba(i, 16, 24), tilingWeights[w]);
                }
            }

            if(squarer && tilingWeights[w] != (-2) && tilingWeights[w] != 2){
                for(int i = 2; i <= 6; i++) {
                    addBaseTile(target, new BaseSquarerLUT(i), tilingWeights[w]);
                }
            }
        }
/*        for(int i = 0; i < (int)bmc->size(); i++)
        {
            cout << bmc->getBaseMultiplier(i).getType() << endl;

            if( (bmc->getBaseMultiplier(i).getType().compare("BaseMultiplierDSPSuperTilesXilinx")) == 0){
                for(int i = 1; i <= 12; i++) {
                    MultTileCollection.push_back(
                            new BaseMultiplierDSPSuperTilesXilinx((BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE) i));
                }
            }

            if( (bmc->getBaseMultiplier(i).getType().compare("BaseMultiplierIrregularLUTXilinx")) == 0){
                for(int i = 1; i <= 8; i++) {
                    MultTileCollection.push_back(
                            new BaseMultiplierIrregularLUTXilinx((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE) i));
                }
            }

        }
*/
//        cout << MultTileCollection.size() << endl;
    }

    void  MultiplierTileCollection::addBaseTile(Target *target, BaseMultiplierCategory *mult, int tilingWeight) {
        mult->setTarget(target);
        mult->setTilingWeight(tilingWeight);
        MultTileCollection.push_back(mult);
        BaseTileCollection.push_back(mult);
    }

    void  MultiplierTileCollection::addSuperTile(Target *target,BaseMultiplierCategory *mult, int tilingWeight) {
        mult->setTarget(target);
        mult->setTilingWeight(tilingWeight);
        MultTileCollection.push_back(mult);
        SuperTileCollection.push_back(mult);
    }

    void  MultiplierTileCollection::addVariableXTile(Target *target,BaseMultiplierCategory *mult, int tilingWeight) {
        mult->setTarget(target);
        mult->setTilingWeight(tilingWeight);
        MultTileCollection.push_back(mult);
        VariableXTileCollection.push_back(mult);
    }

    void  MultiplierTileCollection::addVariableYTile(Target *target,BaseMultiplierCategory *mult, int tilingWeight) {
        mult->setTarget(target);
        mult->setTilingWeight(tilingWeight);
        MultTileCollection.push_back(mult);
        VariableYTileCollection.push_back(mult);
    }

    BaseMultiplierCategory* MultiplierTileCollection::superTileSubtitution(vector<BaseMultiplierCategory*> mtc, int rx1, int ry1, int lx1, int ly1, int rx2, int ry2, int lx2, int ly2){

        for(int i = 0; i < (int)mtc.size(); i++)
        {
            if(mtc[i]->getDSPCost() == 2){
                int id = mtc[i]->isSuperTile(rx1, ry1, lx1, ly1, rx2, ry2, lx2, ly2);
                if(id){
                    return mtc[i+id];
                }
            }
        }
        return nullptr;
    }

}