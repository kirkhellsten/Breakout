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
            if (brickType != BRICKNONE) {
                ++numOfActiveBricks;
            }
        }

    }
    return numOfActiveBricks;
}
