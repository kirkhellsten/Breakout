#ifndef BRICKMAP_H
#define BRICKMAP_H

#include <vector>

using namespace std;

enum BRICKTYPE { BRICKNONE, REDBRICK, BLUEBRICK, GREENBRICK, ORANGEBRICK,
                 WHITEBRICK, MEDIUMBRICK, HARDBRICK, UNBREAKABLEBRICK, BRICK_TYPE_MAX };

class BrickMap
{
    public:

        BrickMap();
        virtual ~BrickMap();

        vector<vector<BRICKTYPE>> bricks;

        int getNumberOfActiveBricks();

    protected:

    private:
};

#endif // BRICKMAP_H
