#ifndef BRICKMAP_H
#define BRICKMAP_H

#include <vector>
#include <algorithm>

using namespace std;

enum BRICKTYPE { BRICKNONE, REDBRICK, BLUEBRICK, GREENBRICK, ORANGEBRICK,
                 WHITEBRICK, MEDIUMBRICK, HARDBRICK, UNBREAKABLEBRICK, BRICK_TYPE_MAX };

static const vector<BRICKTYPE> ONEHIT_BRICKS { REDBRICK, BLUEBRICK, GREENBRICK, ORANGEBRICK, WHITEBRICK };
static const vector<BRICKTYPE> TWOHIT_BRICKS { MEDIUMBRICK };

class BrickMap
{
    public:

        BrickMap();
        virtual ~BrickMap();

        vector<vector<BRICKTYPE>> bricks;

        int getNumberOfActiveBricks();
        void setBrickWhenHit(int row, int column);

    protected:

    private:
};

#endif // BRICKMAP_H
