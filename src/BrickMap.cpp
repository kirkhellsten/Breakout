#include "BrickMap.h"

BrickMap::BrickMap()
{

}

BrickMap::~BrickMap()
{
    //dtor
}

int BrickMap::getNumberOfActiveBricks() {

    int numOfActiveBricks = 0;

    for (int i = 0; i < bricks.size(); ++i) {

        for (int ii = 0; ii < bricks[i].size(); ++ii) {
            BRICKTYPE brickType = bricks[i][ii];
            if (brickType != BRICKNONE && brickType != UNBREAKABLEBRICK) {
                ++numOfActiveBricks;
            }
        }

    }
    return numOfActiveBricks;
}

void BrickMap::setBrickWhenHit(int row, int column) {

    BRICKTYPE brickType = bricks[row][column];

    if(find(ONEHIT_BRICKS.begin(), ONEHIT_BRICKS.end(), brickType) != ONEHIT_BRICKS.end()) {
        bricks[row][column] = BRICKNONE;
    }

    if(find(TWOHIT_BRICKS.begin(), TWOHIT_BRICKS.end(), brickType) != TWOHIT_BRICKS.end()) {
        bricks[row][column] = REDBRICK;
    }

}
