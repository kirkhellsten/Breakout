#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include "BrickMap.h"

using namespace std;

enum GAMESTATE { GAMESTATE_RUNNING, GAMESTATE_PAUSED, GAMESTATE_PLAYERDIED, GAMESTATE_EXIT };

// Blue Powerups are good, grey is neither good or bad, red powerups are bad
enum POWERUPTYPE { LASER, SPLITBALL, STICKYPADDLE, THRUBALL, SLOWBALL, LARGEPADDLE, SMALLPADDLE, FASTBALL, DEATH, POWER_UP_MAX };

enum BALLTYPE { BALLTYPE_NORMAL, BALLTYPE_THRUBALL };
enum PADDLETYPE { PADDLETYPE_NORMAL, PADDLETYPE_LASER, PADDLETYPE_STICKY };
enum LASERTYPE { LASERTYPE_NORMAL, LASERTYPE_THRU };

enum BALLSTATE { NOTRELEASED, RELEASED, MOVING, BALLSTATE_STUCK };
enum STARTLOCATION { CENTERBOTTOM, CENTER };
enum RELEASEDDIRECTION { RELEASED_LEFT, RELEASED_RIGHT, RELEASED_NONE };
enum AXIS { X_AXIS, Y_AXIS };

enum COLLISIONDIRECTION { COL_LEFT, COL_TOP, COL_RIGHT, COL_BOTTOM };

const int INITIAL_PADDLE_WIDTH = 100;
const int INITIAL_PADDLE_HEIGHT = 20;
const int INITIAL_BALL_RADIUS = 5;
const int BALL_MOVEMENT_IN_PIXEL_X_LPS = 1;
const int BALL_MOVEMENT_IN_PIXEL_Y_LPS = 1;
const double BALL_RELEASE_SPEED = 2;
const int PADDLE_WIDTH_INCREMENT = 30;

const int POWER_UP_APPEAR_CHANGE_PER_100 = 14;
const double BALL_RELEASE_ANGLE_LEFT = 240;
const double BALL_RELEASE_ANGLE_RIGHT = 300;

const int NUM_BRICKS_PER_ROW = 20;
const int NUM_BRICKS_PER_COLUMN = 20;

const int POWER_UP_WIDTH = 35;
const int POWER_UP_HEIGHT = 35;

const double BALL_SPEED_MAX = 10;
const double BALL_SPEED_INCREMENT = 0.001;
const double MINIMUM_DEFLECT_ANGLE = 200;
const double MAXIMUM_DEFLECT_ANGLE = 340;
const double INITIAL_BALL_SPEED = 3;
const double MINIMUM_BALL_SPEED = 1.5;

const int BRICK_WIDTH = 40;
const int BRICK_HEIGHT = 20;
const int PADDLE_BORDER_THICKNESS = 5;
const int BALL_BORDER_THICKNESS = 2;
const int POWER_UP_BORDER_THICKNESS = 2;

const int BRICK_BORDER_THICKNESS = 2;

const int LASER_GUN_FIRE_PER_SECOND = 10;
const double LASER_GUN_RECOIL_TIME = 1.0 / LASER_GUN_FIRE_PER_SECOND;

const int PADDLE_CENTERBOTTOM_LOCATION_DISTANCEY = 40;
const int FPS = 60;
const int LPS = 120;

const int LASER_SIDE_WIDTH = 15;
const int LASER_GUN_WIDTH = 5;
const int LASER_GUN_HEIGHT = 10;

const int NORMAL_SIDE_WIDTH = 10;

const int LASER_BULLET_THICKNESS = 2;
const int LASER_BULLET_LENGTH = 10;

const int LASER_BULLET_INITIAL_SPEED = 10;
const int LASER_BULLET_INITIAL_ANGLE = 270;

const int MAX_NUMBER_OF_PADDLE_LARGE_INCREMENTS = 5;
const int MAX_NUMBER_OF_PADDLE_SMALL_INCREMENTS = 2;
const int MAXIMUM_PADDLE_WIDTH = INITIAL_PADDLE_WIDTH + (PADDLE_WIDTH_INCREMENT * MAX_NUMBER_OF_PADDLE_LARGE_INCREMENTS);
const int MINIMUM_PADDLE_WIDTH = INITIAL_PADDLE_WIDTH - (PADDLE_WIDTH_INCREMENT * MAX_NUMBER_OF_PADDLE_SMALL_INCREMENTS);

const int MAX_NUM_OF_BALLS = 16;

const int SCREEN_FADE_INCREMENT = 2;

vector<ALLEGRO_COLOR> brickFillColors (BRICK_TYPE_MAX);
vector<ALLEGRO_COLOR> brickBorderColors (BRICK_TYPE_MAX);

vector<ALLEGRO_COLOR> powerUpFillColors (POWER_UP_MAX);
vector<ALLEGRO_COLOR> powerUpBorderColors (POWER_UP_MAX);

vector<ALLEGRO_COLOR> laserPaddleColors (6);
vector<ALLEGRO_COLOR> normalPaddleColors (6);
vector<ALLEGRO_COLOR> stickyPaddleColors (7);
vector<ALLEGRO_COLOR> laserBulletColors (2);
vector<ALLEGRO_COLOR> ballColors (2);
vector<ALLEGRO_COLOR> levelBorderColor (1);

vector<const char *> powerUpsText (POWER_UP_MAX);

ALLEGRO_TIMER *LBRTimer = NULL;

GAMESTATE gameState;
int numberOfLives;

struct BrickCollisionInfo {
    BRICKTYPE brickType;
    int row;
    int column;
    COLLISIONDIRECTION collisionDirection;
};

struct Laser {
    double x, y;
    double speed;
    double angle;
    int thickness;
    int length;
    LASERTYPE laserType;
};



struct PowerUp{
    double x, y;
    POWERUPTYPE powerUpType;
    double speed;
    double angle;
    double thickness;
};

struct Ball{
    double x, y;
    int radius;
    ALLEGRO_COLOR colorFilled;
    ALLEGRO_COLOR colorBorder;
    float thickness;
    BALLSTATE ballState;
    BALLTYPE ballType;
    RELEASEDDIRECTION releaseDirection;
    double speed;
    double angle;
    double stuckRelativeX, stuckRelativeY;
};

struct Paddle{
    double x, y;
    int width, height;
    ALLEGRO_COLOR colorFilled;
    ALLEGRO_COLOR colorBorder;
    float thickness;
    PADDLETYPE paddleType;
    bool isInRecoil;
};

void setBallsStuckToMove(vector<Ball*> *balls);

void setTexts() {
    powerUpsText[LASER] = "LAS";
    powerUpsText[SPLITBALL] = "SPL";
    powerUpsText[STICKYPADDLE] = "STI";
    powerUpsText[THRUBALL] = "THR";
    powerUpsText[SLOWBALL] = "SLO";
    powerUpsText[LARGEPADDLE] = "LAR";
    powerUpsText[SMALLPADDLE] = "SMA";
    powerUpsText[FASTBALL] = "FAS";
    powerUpsText[DEATH] = "DEA";
}

void setColors() {

    levelBorderColor[0] = al_map_rgb(192, 192, 192);
    ballColors[0] = al_map_rgb(192, 192, 192);
    ballColors[1] = al_map_rgb(255, 255, 255);

    laserBulletColors[0] = al_map_rgb(255, 0, 0);
    laserBulletColors[1] = al_map_rgb(0, 0, 0);

    normalPaddleColors[0] = al_map_rgb(109,164,154);
    normalPaddleColors[1] = al_map_rgb(0, 0, 0);
    normalPaddleColors[2] = al_map_rgb(192, 192, 192);
    normalPaddleColors[3] = al_map_rgb(0, 0, 0);
    normalPaddleColors[4] = al_map_rgb(80,98,175);
    normalPaddleColors[5] = al_map_rgb(0, 0, 0);

    laserPaddleColors[0] = al_map_rgb(146,0,1);
    laserPaddleColors[1] = al_map_rgb(0, 0, 0);
    laserPaddleColors[2] = al_map_rgb(192, 192, 192);
    laserPaddleColors[3] = al_map_rgb(0, 0, 0);
    laserPaddleColors[4] = al_map_rgb(80,98,175);
    laserPaddleColors[5] = al_map_rgb(0, 0, 0);

    stickyPaddleColors[0] = al_map_rgb(0,104,118);
    stickyPaddleColors[1] = al_map_rgb(0, 0, 0);
    stickyPaddleColors[2] = al_map_rgb(190,0,190);
    stickyPaddleColors[3] = al_map_rgb(0, 0, 0);
    stickyPaddleColors[4] = al_map_rgb(2,51,104);
    stickyPaddleColors[5] = al_map_rgb(0,62,58);
    stickyPaddleColors[6] = al_map_rgb(0,130,111);

    brickFillColors[BRICKNONE] = al_map_rgb(0,0,0);
    brickFillColors[REDBRICK] = al_map_rgb(255,0,0);
    brickFillColors[BLUEBRICK] = al_map_rgb(0,0,255);
    brickFillColors[GREENBRICK] = al_map_rgb(0,255,0);
    brickFillColors[WHITEBRICK] = al_map_rgb(255,255,255);
    brickFillColors[ORANGEBRICK] = al_map_rgb(255,165,0);
    brickFillColors[MEDIUMBRICK] = al_map_rgb(220,20,60);
    brickFillColors[HARDBRICK] = al_map_rgb(128,0,0);
    brickFillColors[UNBREAKABLEBRICK] = al_map_rgb(255,215,0);

    brickBorderColors[BRICKNONE] = al_map_rgb(0,0,0);
    brickBorderColors[REDBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[BLUEBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[GREENBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[WHITEBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[ORANGEBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[MEDIUMBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[HARDBRICK] = al_map_rgb(0,0,0);
    brickBorderColors[UNBREAKABLEBRICK] = al_map_rgb(0,0,0);

        //LASER, SPLITBALL, STICKYBALL, THRUBALL, SLOWBALL, LARGEPADDLE, SMALLPADDLE, FASTBALL, DEATH

    powerUpFillColors[LASER] = al_map_rgb(0,0,128);
    powerUpFillColors[SPLITBALL] = al_map_rgb(192,192,192);
    powerUpFillColors[STICKYPADDLE] = al_map_rgb(0,0,128);
    powerUpFillColors[THRUBALL] = al_map_rgb(0,0,128);
    powerUpFillColors[SLOWBALL] = al_map_rgb(0,0,128);
    powerUpFillColors[LARGEPADDLE] = al_map_rgb(192,192,192);
    powerUpFillColors[SMALLPADDLE] = al_map_rgb(192,192,192);
    powerUpFillColors[FASTBALL] = al_map_rgb(178,34,34);
    powerUpFillColors[DEATH] = al_map_rgb(178,34,34);

    powerUpBorderColors[LASER] = al_map_rgb(0,0,0);
    powerUpBorderColors[SPLITBALL] = al_map_rgb(0,0,0);
    powerUpBorderColors[STICKYPADDLE] = al_map_rgb(0,0,0);
    powerUpBorderColors[THRUBALL] = al_map_rgb(0,0,0);
    powerUpBorderColors[SLOWBALL] = al_map_rgb(0,0,0);
    powerUpBorderColors[LARGEPADDLE] = al_map_rgb(0,0,0);
    powerUpBorderColors[SMALLPADDLE] = al_map_rgb(0,0,0);
    powerUpBorderColors[FASTBALL] = al_map_rgb(0,0,0);
    powerUpBorderColors[DEATH] = al_map_rgb(0,0,0);

}

vector<double> getBallXYSpeeds(Ball *ball) {
    vector<double> XYSpeeds (2);
    double PI = atan(1)*4;
    XYSpeeds[0] = cos(ball->angle*PI/180) * ball->speed;
    XYSpeeds[1] = sin(ball->angle*PI/180) * ball->speed;
    return XYSpeeds;
}

vector<double> getXYSpeeds(double speed, double angle) {
    vector<double> XYSpeeds (2);
    double PI = atan(1)*4;
    XYSpeeds[0] = cos(angle*PI/180) * speed;
    XYSpeeds[1] = sin(angle*PI/180) * speed;
    return XYSpeeds;
}

void setBallsSpeed(vector<Ball*> *balls, double speed) {
    for (int i = 0; i < balls->size(); ++i) {
        (*balls)[i]->speed = speed;
    }
}

void setBallAngle(Ball *ball, double angle) {
    if (angle < 0) {
        angle += 360;
    } else if (angle > 360) {
        angle -= 360;
    }
    ball->angle = angle;
}



void deleteLaserBullet(vector<Laser*> *lasers, Laser *laser) {
    cout << "Laser Bullet Deleted " << endl;
    delete laser;
    lasers->erase(remove(lasers->begin(), lasers->end(), laser), lasers->end());
}

void deleteLaserBullets(vector<Laser*> *lasers) {
    for (int i = 0; i < lasers->size(); ++i) {
        deleteLaserBullet(lasers, (*lasers)[i]);
    }
    lasers->clear();
}

void deleteBall(vector<Ball*> *balls, Ball *ball) {

    cout << "Deleted Ball" << endl;
    delete ball;
    balls->erase(remove(balls->begin(), balls->end(), ball), balls->end());

    if (balls->size() == 0) {
        gameState = GAMESTATE_PLAYERDIED;
    }

}

void deletePowerUp(vector<PowerUp*> *powerUps, PowerUp *powerUp) {
    cout << "Deleted Power Up" << endl;
    delete powerUp;
    powerUps->erase(remove(powerUps->begin(), powerUps->end(), powerUp), powerUps->end());
}

void deleteBalls(vector<Ball*> *balls) {
    for (int i = 0; i < balls->size(); ++i) {
        deleteBall(balls, (*balls)[i]);
    }
    balls->clear();
}

void deletePowerUps(vector<PowerUp*> *powerUps) {
    for (int i = 0; i < powerUps->size(); ++i) {
        deletePowerUp(powerUps, (*powerUps)[i]);
    }
    powerUps->clear();
}


vector<double> addVector(vector<double> v1, vector<double> v2) {

    double PI = atan(1)*4;

    vector<double> resultantVector (2);

    vector<double> xySpeeds1 = getXYSpeeds(v1[0], v1[1]);
    vector<double> xySpeeds2 = getXYSpeeds(v2[0], v2[1]);
    vector<double> resulstantSpeeds (2);
    resulstantSpeeds[0] = xySpeeds1[0] + xySpeeds2[0];
    resulstantSpeeds[1] = xySpeeds1[1] + xySpeeds2[1];

    resultantVector[0] = sqrt(resulstantSpeeds[0] * resulstantSpeeds[0] + resulstantSpeeds[1] * resulstantSpeeds[1]);
    resultantVector[1] = atan2(resulstantSpeeds[1], resulstantSpeeds[0]) * 180 / PI;

    // Make sure degrees 0 to 360
    if (resultantVector[1] < 0) {
        resultantVector[1] += 360;
    }

    return resultantVector;

}

vector<double> getPreviousBallLocation(Ball *ball) {
    vector<double> previousBallLocation (2);
    vector<double> ballXYSpeeds = getBallXYSpeeds(ball);
    previousBallLocation[0] = ball->x - ballXYSpeeds[0];
    previousBallLocation[1] = ball->y - ballXYSpeeds[1];
    return previousBallLocation;
}

void correctBallAngle(Ball *ball) {
    if (ball->angle < MINIMUM_DEFLECT_ANGLE) {
        setBallAngle(ball, MINIMUM_DEFLECT_ANGLE);
    } else if (ball->angle > MAXIMUM_DEFLECT_ANGLE) {
        setBallAngle(ball, MAXIMUM_DEFLECT_ANGLE);
    }
}

void createLaserBeams(vector<Laser*> *lasers, Paddle *paddle, LASERTYPE laserType = LASERTYPE_NORMAL) {

    if (paddle->isInRecoil == true) {
        al_start_timer(LBRTimer);
        return;
    }

    Laser *laserBulletLeft = new Laser();
    laserBulletLeft->x = paddle->x+LASER_SIDE_WIDTH/2;
    laserBulletLeft->y = paddle->y+paddle->height/2;
    laserBulletLeft->thickness = LASER_BULLET_THICKNESS;
    laserBulletLeft->length = LASER_BULLET_LENGTH;
    laserBulletLeft->speed = LASER_BULLET_INITIAL_SPEED;
    laserBulletLeft->angle = LASER_BULLET_INITIAL_ANGLE;
    laserBulletLeft->laserType = laserType;

    Laser *laserBulletRight = new Laser();
    laserBulletRight->x = paddle->x + paddle->width-LASER_SIDE_WIDTH/2;
    laserBulletRight->y = paddle->y+paddle->height/2;
    laserBulletRight->thickness = LASER_BULLET_THICKNESS;
    laserBulletRight->length = LASER_BULLET_LENGTH;
    laserBulletRight->speed = LASER_BULLET_INITIAL_SPEED;
    laserBulletRight->angle = LASER_BULLET_INITIAL_ANGLE;
    laserBulletRight->laserType = laserType;

    lasers->push_back(laserBulletLeft);
    lasers->push_back(laserBulletRight);

    paddle->isInRecoil = true;

}

double reflectAngle(double angle, AXIS axis) {

    if (axis == X_AXIS) {

        if (angle >= 0 && angle < 90) {
            angle = 180 - angle;
        } else if (angle >= 90 && angle < 180) {
            angle = 180 - angle;
        } else if (angle >= 180 && angle < 270) {
            angle = 270 + (270 - angle);
        } else if (angle >= 270 && angle < 360) {
            angle = 180 + (360 - angle);
        }

    } else if (axis == Y_AXIS) {

        if (angle > 180 && angle <= 270) {
            angle = 180 - (angle - 180);
        } else if (angle >= 270 && angle < 360) {
            angle = 360 - angle;
        } else if (angle >= 0 && angle < 90) {
            angle = 360 - angle;
        } else if (angle >= 90 && angle < 180) {
            angle = 180 + (180 - angle);
        }
    }

    return angle;
}

bool isBallCollidingWithBrick(Ball *ball, BrickMap *brickMap, BrickCollisionInfo *brickCollisionInfo) {

    int minimumRow = (int) (ball->y / BRICK_HEIGHT);
    int maximumRow = minimumRow + 1;
    int minimumColumn = (int) (ball->x / BRICK_WIDTH);
    int maximumColumn = minimumColumn + 1;

    if (maximumRow >= brickMap->bricks.size()) {
        maximumRow = minimumRow;
    }

    if (maximumColumn >= brickMap->bricks[0].size()) {
        maximumColumn = minimumColumn;
    }

    // Do not bother looking if ball is below all bricks
    if (minimumRow >= brickMap->bricks.size()) {
        return false;
    }

    double brickX = 0, brickY = 0;
    for (int row = minimumRow; row <= maximumRow; ++row) {
        brickY = row * BRICK_HEIGHT;
        for (int column = minimumColumn; column <= maximumColumn; ++column) {
            brickX = column * BRICK_WIDTH;

            // Do not bother if brick is nothing
            if (brickMap->bricks[row][column] == BRICKNONE) {
                continue;
            } else {

                if (ball->x > brickX - ball->radius * 2 &&
                    ball->x < brickX + BRICK_WIDTH &&
                    ball->y > brickY - ball->radius * 2 &&
                    ball->y < brickY + BRICK_HEIGHT) {

                    brickCollisionInfo->brickType = brickMap->bricks[row][column];
                    brickCollisionInfo->row = row;
                    brickCollisionInfo->column = column;

                    vector<double> previousBallLocation = getPreviousBallLocation(ball);

                    if (previousBallLocation[1] <= brickY - ball->radius * 2) {
                        brickCollisionInfo->collisionDirection = COL_TOP;
                    } else if (previousBallLocation[1] >= brickY + BRICK_HEIGHT) {
                        brickCollisionInfo->collisionDirection = COL_BOTTOM;
                    } else if (previousBallLocation[0] <= brickX - ball->radius * 2) {
                        brickCollisionInfo->collisionDirection = COL_LEFT;
                    } else if (previousBallLocation[0] >= brickX + BRICK_WIDTH) {
                        brickCollisionInfo->collisionDirection = COL_RIGHT;
                    }

                    return true;
                }
            }



        }
    }

    return false;
}

bool isLaserBulletCollidingWithBricks(Laser *laser, BrickMap *brickMap, BrickCollisionInfo *brickCollisionInfo) {

    int minimumRow = (int) (laser->y / BRICK_HEIGHT);
    int maximumRow = minimumRow + 1;
    int minimumColumn = (int) (laser->x / BRICK_WIDTH);
    int maximumColumn = minimumColumn + 1;

    if (maximumRow >= brickMap->bricks.size()) {
        maximumRow = minimumRow;
    }

    if (maximumColumn >= brickMap->bricks[0].size()) {
        maximumColumn = minimumColumn;
    }

    // Do not bother looking if laser is below all bricks
    if (minimumRow >= brickMap->bricks.size()) {
        return false;
    }

    double brickX = 0, brickY = 0;
    for (int row = minimumRow; row <= maximumRow; ++row) {
        brickY = row * BRICK_HEIGHT;
        for (int column = minimumColumn; column <= maximumColumn; ++column) {
            brickX = column * BRICK_WIDTH;

            // Do not bother if brick is nothing
            if (brickMap->bricks[row][column] == BRICKNONE) {
                continue;
            } else {

                if (laser->x > brickX - laser->thickness &&
                    laser->x < brickX + BRICK_WIDTH &&
                    laser->y > brickY - laser->length &&
                    laser->y < brickY + BRICK_HEIGHT) {

                    brickCollisionInfo->brickType = brickMap->bricks[row][column];
                    brickCollisionInfo->row = row;
                    brickCollisionInfo->column = column;

                    brickCollisionInfo->collisionDirection = COL_BOTTOM;

                    return true;
                }
            }

        }
    }

    return false;
}

bool isBallCollidingWithPaddle(Ball *ball, Paddle *paddle) {

    if (ball->x > paddle->x - ball->radius * 2 - paddle->thickness &&
        ball->x < paddle->x + paddle->width &&
        ball->y > paddle->y - ball->radius * 2 - paddle->thickness &&
        ball->y < paddle->y + paddle->height) {
        return true;
    } else {
        return false;
    }

}

void setMap(BrickMap *brickMap) {

    const BRICKTYPE levelOne[NUM_BRICKS_PER_ROW][NUM_BRICKS_PER_COLUMN] =
        {{ BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE},
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK,
           REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK,},
         { BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK,
           BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK},
         { GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK,
           GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK},
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE},
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE,
           BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE, BRICKNONE },
         { REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK,
           REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK, REDBRICK},
         { BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK,
           BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK, BLUEBRICK},
         { GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK,
           GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK, GREENBRICK}};

    vector<vector<BRICKTYPE>> bricks (NUM_BRICKS_PER_ROW);
    for (int i = 0; i < bricks.size(); ++i) {
        vector<BRICKTYPE> rowBricks (NUM_BRICKS_PER_COLUMN);
        bricks[i] = rowBricks;
        for (int ii = 0; ii < bricks[i].size(); ++ii) {
            bricks[i][ii] = levelOne[i][ii];
        }
    }

    brickMap->bricks = bricks;
}

void drawLaserBullets(vector<Laser*> *lasers) {
    for (int i = 0; i < lasers->size(); ++i) {
        Laser *laser = (*lasers)[i];
        al_draw_line(laser->x, laser->y-laser->length, laser->x, laser->y,
            laserBulletColors[0], laser->thickness);
    }
}

void drawPowerUps(vector<PowerUp*> *powerUps) {

    static ALLEGRO_FONT *font = al_create_builtin_font();

    for (int i = 0; i < powerUps->size(); ++i) {

        PowerUp *powerUp = (*powerUps)[i];

        al_draw_filled_rectangle(powerUp->x,
                                 powerUp->y,
                                 powerUp->x + POWER_UP_WIDTH,
                                 powerUp->y + POWER_UP_HEIGHT,
                                 powerUpFillColors[powerUp->powerUpType]);

        al_draw_rectangle(powerUp->x,
                             powerUp->y,
                             powerUp->x + POWER_UP_WIDTH,
                             powerUp->y + POWER_UP_HEIGHT,
                             powerUpBorderColors[powerUp->powerUpType],
                             powerUp->thickness);


        al_draw_textf(font, al_map_rgb(255, 255, 255),
                    powerUp->x + POWER_UP_WIDTH / 5.5, powerUp->y + POWER_UP_HEIGHT / 2.75, 0, powerUpsText[(int)powerUp->powerUpType]);


    }
}

void drawPaddle(Paddle *paddle) {

    static int colorChangeRate = 20;
    static int colorChangeIncrement = 0;
    static int color1 = 0;
    static int color2 = 2;
    static int color3 = 5;

    if (paddle->paddleType == PADDLETYPE_NORMAL) {

        al_draw_rectangle(paddle->x,
              paddle->y,
              paddle->x+paddle->width,
              paddle->y+paddle->height,
              normalPaddleColors[3],
              paddle->thickness/2);

        al_draw_filled_rectangle(paddle->x,
                          paddle->y,
                          paddle->x+paddle->width,
                          paddle->y+paddle->height,
                          normalPaddleColors[2]);

        al_draw_filled_rectangle(paddle->x,
                          paddle->y,
                          paddle->x+NORMAL_SIDE_WIDTH,
                          paddle->y+paddle->height,
                          normalPaddleColors[0]);

        al_draw_rectangle(paddle->x,
              paddle->y,
              paddle->x+NORMAL_SIDE_WIDTH,
              paddle->y+paddle->height,
              normalPaddleColors[1],
              paddle->thickness/2);

        al_draw_filled_rectangle(paddle->x + paddle->width - NORMAL_SIDE_WIDTH,
                          paddle->y,
                          paddle->x + paddle->width,
                          paddle->y+paddle->height,
                          normalPaddleColors[0]);

        al_draw_rectangle(paddle->x + paddle->width - NORMAL_SIDE_WIDTH,
              paddle->y,
              paddle->x + paddle->width,
              paddle->y+paddle->height,
              normalPaddleColors[1],
              paddle->thickness/2);

    } else if (paddle->paddleType == PADDLETYPE_LASER) {

        al_draw_rectangle(paddle->x,
              paddle->y,
              paddle->x+paddle->width,
              paddle->y+paddle->height,
              laserPaddleColors[3],
              paddle->thickness);

        al_draw_filled_rectangle(paddle->x,
                          paddle->y,
                          paddle->x+paddle->width,
                          paddle->y+paddle->height,
                          laserPaddleColors[2]);

        al_draw_filled_rectangle(paddle->x,
                          paddle->y,
                          paddle->x+LASER_SIDE_WIDTH,
                          paddle->y+paddle->height,
                          laserPaddleColors[0]);

        al_draw_rectangle(paddle->x,
              paddle->y,
              paddle->x+LASER_SIDE_WIDTH,
              paddle->y+paddle->height,
              laserPaddleColors[1],
              paddle->thickness);

        al_draw_filled_rectangle(paddle->x + paddle->width - LASER_SIDE_WIDTH,
                          paddle->y,
                          paddle->x + paddle->width,
                          paddle->y+paddle->height,
                          laserPaddleColors[0]);

        al_draw_rectangle(paddle->x + paddle->width - LASER_SIDE_WIDTH,
              paddle->y,
              paddle->x + paddle->width,
              paddle->y+paddle->height,
              laserPaddleColors[1],
              paddle->thickness);

        al_draw_line(paddle->x+LASER_SIDE_WIDTH/2, paddle->y,
                     paddle->x+LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[4], paddle->thickness);

        al_draw_line(paddle->x+paddle->width-LASER_SIDE_WIDTH/2, paddle->y,
                     paddle->x+paddle->width-LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[4], paddle->thickness);


        al_draw_line(paddle->x+LASER_SIDE_WIDTH/2-1, paddle->y-LASER_GUN_HEIGHT+4,
                     paddle->x+LASER_SIDE_WIDTH/2-1, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[5], paddle->thickness/2);

        al_draw_line(paddle->x+paddle->width-LASER_SIDE_WIDTH/2-1, paddle->y-LASER_GUN_HEIGHT+4,
                     paddle->x+paddle->width-LASER_SIDE_WIDTH/2-1, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[5], paddle->thickness/2);

        al_draw_line(paddle->x+LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT+4,
                     paddle->x+LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[5], paddle->thickness/2);

        al_draw_line(paddle->x+paddle->width-LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT+4,
                     paddle->x+paddle->width-LASER_SIDE_WIDTH/2, paddle->y-LASER_GUN_HEIGHT,
            laserPaddleColors[5], paddle->thickness/2);

    } else if (paddle->paddleType == PADDLETYPE_STICKY) {

        al_draw_filled_rectangle(paddle->x,
                          paddle->y,
                          paddle->x+paddle->width,
                          paddle->y+paddle->height,
                          stickyPaddleColors[4]);

        al_draw_filled_rectangle(paddle->x+1,
                          paddle->y,
                          paddle->x+paddle->width,
                          paddle->y+3,
                          stickyPaddleColors[color1]);

        al_draw_filled_rectangle(paddle->x+1,
                          paddle->y+3,
                          paddle->x+paddle->width,
                          paddle->y+5,
                          stickyPaddleColors[color2]);

        colorChangeIncrement++;

        if (colorChangeIncrement >= colorChangeRate) {
            colorChangeIncrement = 0;

            if (color1 == 0) {
                color1 = 2;
            } else {
                color1 = 0;
            }
            if (color2 == 0) {
                color2 = 2;
            } else {
                color2 = 0;
            }

            if (color3 == 6) {
                color3 = 5;
            } else {
                color3 = 6;
            }
        }



        al_draw_rectangle(paddle->x,
              paddle->y,
              paddle->x+paddle->width,
              paddle->y+paddle->height,
              stickyPaddleColors[color3],
              paddle->thickness/2);


    }

}

void drawBalls(vector<Ball*> *balls) {

    for (int i = 0; i < balls->size(); ++i) {
        Ball *ball = (*balls)[i];
            al_draw_circle(ball->x + ball->radius,
              ball->y + ball->radius,
              ball->radius,
              ballColors[1],
              ball->thickness);

            al_draw_filled_circle(ball->x + ball->radius,
                                  ball->y + ball->radius,
                                  ball->radius,
                                  ballColors[0]);

    }
}

void drawBrickMap(BrickMap *brickMap) {
    for (int i = 0; i < brickMap->bricks.size(); ++i) {
        for (int ii = 0; ii < brickMap->bricks[i].size(); ++ii) {
            BRICKTYPE brickType = brickMap->bricks[i][ii];

            if (brickType != BRICKNONE) {
                al_draw_filled_rectangle(ii * BRICK_WIDTH,
                                         i * BRICK_HEIGHT,
                                         ii * BRICK_WIDTH + BRICK_WIDTH,
                                         i * BRICK_HEIGHT + BRICK_HEIGHT,
                                         brickFillColors[brickType]);


                al_draw_rectangle(i * BRICK_WIDTH,
                                         i * BRICK_HEIGHT,
                                         ii * BRICK_WIDTH + BRICK_WIDTH,
                                         i * BRICK_HEIGHT + BRICK_HEIGHT,
                                         brickBorderColors[brickType],
                                         BRICK_BORDER_THICKNESS);

            }

        }
    }
}

void activateDeathPowerUp() {
    gameState = GAMESTATE_PLAYERDIED;
}

void activateSmallPaddlePowerUp(Paddle *paddle, vector<Ball*> *balls) {
    if (paddle->width != MINIMUM_PADDLE_WIDTH) {
        int newWidthSize = paddle->width - PADDLE_WIDTH_INCREMENT;

        for (int i = 0; i < balls->size(); ++i) {
            Ball *ball = (*balls)[i];
            if (ball->ballState == BALLSTATE_STUCK) {
                double currentStuckRatio = ball->stuckRelativeX / paddle->width;
                cout << "Before Ratio: " << currentStuckRatio << endl;
                ball->stuckRelativeX = newWidthSize * currentStuckRatio;
                cout << "After Ratio: " << ball->stuckRelativeX / newWidthSize << endl;
            }
        }

        paddle->width = newWidthSize;
        paddle->x += PADDLE_WIDTH_INCREMENT / 2;

    }
}

void activateLargePaddlePowerUp(Paddle *paddle, vector<Ball*> *balls) {
    if (paddle->width != MAXIMUM_PADDLE_WIDTH) {

        int newWidthSize = paddle->width + PADDLE_WIDTH_INCREMENT;

        for (int i = 0; i < balls->size(); ++i) {
            Ball *ball = (*balls)[i];

            if (ball->ballState == BALLSTATE_STUCK) {
                double currentStuckRatio = ball->stuckRelativeX / paddle->width;
                cout << "Before Ratio: " << currentStuckRatio << endl;
                ball->stuckRelativeX = newWidthSize * currentStuckRatio;
                cout << "After Ratio: " <<  ball->stuckRelativeX / newWidthSize << endl;
            }
        }

        paddle->width = newWidthSize;
        paddle->x -= PADDLE_WIDTH_INCREMENT / 2;

    }
}

void activateLaserPowerUp(Paddle *paddle, vector<Ball*> *balls) {
    paddle->paddleType = PADDLETYPE_LASER;
    setBallsStuckToMove(balls);
}

void activateThruBallPowerUp(vector<Ball*> *balls) {
    for (int i = 0; i < balls->size(); ++i) {
        (*balls)[i]->ballType = BALLTYPE_THRUBALL;
    }
}
void activateSplitBallPowerUp(vector<Ball*> *balls) {

    cout << "Size of list: " << balls->size() << endl;
    // Make sure to not create more than max number of balls
    if (balls->size() >= MAX_NUM_OF_BALLS) {
        return;
    }

    int originalContainerSize = balls->size();
    // Create a new ball from every existing ball
    for (int i = 0; i < originalContainerSize; ++i) {

        // Make sure to not create anymore than max number of balls
        if (balls->size() >= MAX_NUM_OF_BALLS) {
            return;
        }

        Ball *currentBall = (*balls)[i];
        Ball *splitBall = new Ball();


        setBallAngle(splitBall, currentBall->angle + 180);

        splitBall->ballState = currentBall->ballState;
        splitBall->ballType = currentBall->ballType;
        splitBall->colorBorder = currentBall->colorBorder;
        splitBall->colorFilled = currentBall->colorFilled;
        splitBall->radius = currentBall->radius;
        splitBall->releaseDirection = currentBall->releaseDirection;
        splitBall->speed = currentBall->speed;
        splitBall->thickness = currentBall->thickness;
        splitBall->x = currentBall->x;
        splitBall->y = currentBall->y;
        splitBall->stuckRelativeX = currentBall->stuckRelativeX;
        splitBall->stuckRelativeY = currentBall->stuckRelativeY;

        balls->push_back(splitBall);

    }

}

void setPaddlePositionByMouseEvent(ALLEGRO_MOUSE_EVENT mouseEvent, ALLEGRO_DISPLAY *display, Paddle *paddle) {

    int calculateX = mouseEvent.x - (double) paddle->width / 2;
    int displayWidth = al_get_display_width(display);

    int minimumX = paddle->thickness - 2,
        maximumX = displayWidth - paddle->width - paddle->thickness + 2;

    if (calculateX < minimumX) {
        calculateX = minimumX;
    } else if (calculateX > maximumX) {
        calculateX = maximumX;
    }

    paddle->x = calculateX;

}

void setPowerUpsPosition(vector<PowerUp*> *powerUps, Paddle *paddle, vector<Ball*> *balls, ALLEGRO_DISPLAY *display) {

    int displayWidth = al_get_display_width(display);
    int displayHeight = al_get_display_width(display);

    for (int i = 0; i < powerUps->size(); ++i) {
        PowerUp *powerUp = (*powerUps)[i];

        // Add gravity velocity vector to power up velocity, this will make power up go down
        vector<double> powerUpVelocity (2);
        powerUpVelocity[0] = powerUp->speed;
        powerUpVelocity[1] = powerUp->angle;

        vector<double> velocityDownward (2);
        velocityDownward[0] = 0.025;
        velocityDownward[1] = 90;

        vector<double> newPowerUpVelocity = addVector(powerUpVelocity, velocityDownward);

        powerUp->speed = newPowerUpVelocity[0];
        powerUp->angle = newPowerUpVelocity[1];

        // Add component speeds to power up coordinate
        vector<double> xySpeeds = getXYSpeeds(powerUp->speed, powerUp->angle);
        powerUp->x += xySpeeds[0];
        powerUp->y += xySpeeds[1];



        // Collision detection for power up
        if (powerUp->x < 0) {
            powerUp->angle = reflectAngle(powerUp->angle, X_AXIS);
            powerUp->x = 0;
        } else if (powerUp->x + POWER_UP_WIDTH > displayWidth) {
            powerUp->angle = reflectAngle(powerUp->angle, X_AXIS);
            powerUp->x = displayWidth - POWER_UP_WIDTH;
        }

        if (powerUp->y < 0) {
            powerUp->y = 0;
            powerUp->angle = reflectAngle(powerUp->angle, Y_AXIS);
        }

        if (powerUp->y > displayHeight) {
            deletePowerUp(powerUps, powerUp);
            --i;
            continue;
        }

        if (powerUp->y + POWER_UP_HEIGHT > paddle->y &&
            powerUp->x + POWER_UP_WIDTH > paddle->x &&
            powerUp->x < paddle->x + paddle->width &&
            powerUp->y < paddle->y + paddle->height) {

            if (powerUp->powerUpType == LASER) {
                activateLaserPowerUp(paddle, balls);
            } else if (powerUp->powerUpType == STICKYPADDLE) {
                paddle->paddleType = PADDLETYPE_STICKY;
            } else if (powerUp->powerUpType == FASTBALL) {
                setBallsSpeed(balls, BALL_SPEED_MAX);
            } else if (powerUp->powerUpType == SLOWBALL) {
                setBallsSpeed(balls, MINIMUM_BALL_SPEED);
            } else if (powerUp->powerUpType == LARGEPADDLE) {
                activateLargePaddlePowerUp(paddle, balls);
            } else if (powerUp->powerUpType == SMALLPADDLE) {
                activateSmallPaddlePowerUp(paddle, balls);
            } else if (powerUp->powerUpType == SPLITBALL) {
                activateSplitBallPowerUp(balls);
            } else if (powerUp->powerUpType == THRUBALL) {
                activateThruBallPowerUp(balls);
            } else if (powerUp->powerUpType == DEATH) {
                activateDeathPowerUp();
            }



            deletePowerUp(powerUps, powerUp);
            --i;
        }

    }
}

void setLaserBulletsPosition(vector<Laser*> *lasers, BrickMap *brickMap) {

    static BrickCollisionInfo brickCollisionInfo;

    for (int i = 0; i < lasers->size(); ++i) {
        Laser *laser = (*lasers)[i];

        vector<double> xySpeeds = getXYSpeeds(laser->speed, laser->angle);
        laser->x += xySpeeds[0];
        laser->y += xySpeeds[1];

        if (isLaserBulletCollidingWithBricks(laser, brickMap, &brickCollisionInfo)) {

            if (brickCollisionInfo.brickType > BRICKNONE) {
                brickMap->bricks[brickCollisionInfo.row][brickCollisionInfo.column] = BRICKNONE;
            }

            if (laser->laserType == LASERTYPE_NORMAL) {
                deleteLaserBullet(lasers, laser);
                i--;
                continue;
            }

        }

        if (laser->y < -laser->length) {
            if (laser->laserType == LASERTYPE_NORMAL) {
                deleteLaserBullet(lasers, laser);
                i--;
                continue;
            }
        }
    }

}

void setBallDeflectFromPaddle(Ball *ball, Paddle *paddle) {

    double xRatioFromMiddle = (ball->x + ball->radius - (paddle->x + paddle->width / 2)) / ((paddle->width + ball->radius * 2) / 2);
    double calculatedAngle = 90 * xRatioFromMiddle;
    setBallAngle(ball, 270 + calculatedAngle);

    correctBallAngle(ball);

}

void setBallsPosition(vector<Ball*> *balls, Paddle *paddle, BrickMap *brickMap, vector<PowerUp*> *powerUps, ALLEGRO_DISPLAY *display) {

    static BrickCollisionInfo brickCollisionInfo;

    for (int i = 0; i < balls->size(); ++i) {

        Ball *ball = (*balls)[i];

        if (ball->ballState == BALLSTATE_STUCK && paddle->paddleType == PADDLETYPE_STICKY) {

            ball->x = paddle->x + ball->stuckRelativeX;
            ball->y = paddle->y + ball->stuckRelativeY;

        } else if (ball->ballState == RELEASED) {

            ball->speed = INITIAL_BALL_SPEED;

            if (ball->releaseDirection == RELEASED_LEFT) {
                setBallAngle(ball, BALL_RELEASE_ANGLE_LEFT);
            } else if (ball->releaseDirection == RELEASED_RIGHT) {
                setBallAngle(ball, BALL_RELEASE_ANGLE_RIGHT);
            }

            ball->ballState = MOVING;
        } else if (ball->ballState == MOVING) {

            if (ball->speed > BALL_SPEED_MAX) {
                ball->speed = BALL_SPEED_MAX;
            } else {
                ball->speed += BALL_SPEED_INCREMENT;
            }

            vector<double> previousBallLocation = getPreviousBallLocation(ball);

            int minimumX = 0;
            int minimumY = 0;
            int maximumX = al_get_display_width(display) - ball->radius * 2;

            int displayHeight = al_get_display_height(display);

            vector<double> xySpeeds = getBallXYSpeeds(ball);

            ball->x += xySpeeds[0];
            ball->y += xySpeeds[1];

            if (ball->x < minimumX) {
                ball->angle = reflectAngle(ball->angle, X_AXIS);
                ball->x = minimumX;
            } else if (ball->x > maximumX) {
                ball->angle = reflectAngle(ball->angle, X_AXIS);
                ball->x = maximumX;
            }

            if (ball->y < minimumY) {
                ball->angle = reflectAngle(ball->angle, Y_AXIS);
                ball->y = minimumY;
            } else if (ball->y > displayHeight) {

                deleteBall(balls, (*balls)[i]);


            }


            if (isBallCollidingWithPaddle(ball, paddle)) {

                vector<double> previousBallLocation = getPreviousBallLocation(ball);

                // Ball y + ball radius + (border thickness), is above paddle
                if (previousBallLocation[1] <= paddle->y - ball->radius * 2 - ball->thickness) {


                    setBallDeflectFromPaddle(ball, paddle);

                    ball->y = paddle->y - ball->radius * 2 - paddle->thickness;


                    if (paddle->paddleType == PADDLETYPE_STICKY) {

                        ball->ballState = BALLSTATE_STUCK;
                        ball->stuckRelativeX = ball->x - paddle->x;
                        ball->stuckRelativeY = ball->y - paddle->y;

                    }

                } else {
                    ball->angle = reflectAngle(ball->angle, X_AXIS);
                }
            }

            if (isBallCollidingWithBrick(ball, brickMap, &brickCollisionInfo)) {



                int randPowerUpCreation = rand() % 100;
                if (randPowerUpCreation < POWER_UP_APPEAR_CHANGE_PER_100) {


                    int numberOfPowerUpsPer100 = 100 / (POWER_UP_MAX - 1);
                    POWERUPTYPE randPowerUpType = (POWERUPTYPE) ((rand() % 100) / numberOfPowerUpsPer100);


                    PowerUp *powerUp = new PowerUp();
                    powerUp->powerUpType = randPowerUpType,
                    //powerUp->powerUpType = DEATH;
                    powerUp->angle = ball->angle;
                    powerUp->speed = ball->speed / 2;
                    powerUp->x = ball->x - POWER_UP_WIDTH / 2;
                    powerUp->y = ball->y - POWER_UP_HEIGHT / 2;
                    powerUp->thickness = POWER_UP_BORDER_THICKNESS;

                    powerUps->push_back(powerUp);
                    cout << "Maximum number of power ups: " << POWER_UP_MAX << endl;
                    cout << "Power Up Container Size: " << powerUps->size() << endl;
                    cout << "Power Up Created: " << randPowerUpType << endl;


                }

                if (ball->ballType == BALLTYPE_THRUBALL) {
                    brickMap->bricks[brickCollisionInfo.row][brickCollisionInfo.column] = BRICKNONE;
                } else if (ball->ballType == BALLTYPE_NORMAL) {

                    if (brickCollisionInfo.collisionDirection == COL_BOTTOM ||
                        brickCollisionInfo.collisionDirection == COL_TOP) {
                        ball->angle = reflectAngle(ball->angle, Y_AXIS);
                    } else {
                        ball->angle = reflectAngle(ball->angle, X_AXIS);
                    }

                    if (brickCollisionInfo.brickType > BRICKNONE) {
                        brickMap->bricks[brickCollisionInfo.row][brickCollisionInfo.column] = BRICKNONE;
                    }

                }

            }


        }

    }


}

void setBallPositionByMouseEvent(ALLEGRO_MOUSE_EVENT mouseEvent, ALLEGRO_DISPLAY *display, Paddle *paddle, Ball *ball) {

    if (ball->ballState == NOTRELEASED) {

        int calculateX = ball->x;
        int calculateY = ball->y;
        int displayWidth = al_get_display_width(display);

        int minimumX = paddle->width / 2 + ball->radius - paddle->thickness - 2,
        maximumX = displayWidth - paddle->width / 2 - ball->radius - paddle->thickness + 2;

        calculateX = mouseEvent.x - (double) ball->radius;

        if (calculateX < minimumX) {
            calculateX = minimumX;
        } else if (calculateX > maximumX) {
            calculateX = maximumX;
        }

        ball->x = calculateX;
        ball->y = calculateY;

    }
}

vector<int> getBallStartLocation(ALLEGRO_DISPLAY *display, Paddle *paddle, Ball *ball, STARTLOCATION startLocation) {
    vector<int> locationCoordinate (2);
    int xLocation = 0, yLocation = 0;

    if (startLocation == CENTERBOTTOM) {

        xLocation = al_get_display_width(display) / 2 - ball->radius;
        yLocation = al_get_display_height(display) -
                    PADDLE_CENTERBOTTOM_LOCATION_DISTANCEY -
                    paddle->thickness -
                    ball->radius*2 -
                    ball->thickness;

    } else if (startLocation == CENTER) {
        xLocation = al_get_display_width(display) / 2 - ball->radius;
        yLocation = al_get_display_height(display) / 2 - ball->radius;
    }

    locationCoordinate[0] = xLocation;
    locationCoordinate[1] = yLocation;

    return locationCoordinate;
}

vector<int> getPaddleStartLocation(ALLEGRO_DISPLAY *display, Paddle *paddle, STARTLOCATION startLocation) {
    vector<int> locationCoordinate (2);
    int xLocation = 0, yLocation = 0;

    if (startLocation == CENTERBOTTOM) {
        xLocation = al_get_display_width(display) / 2 - paddle->width / 2;
        yLocation = al_get_display_height(display) - PADDLE_CENTERBOTTOM_LOCATION_DISTANCEY;
    }

    locationCoordinate[0] = xLocation;
    locationCoordinate[1] = yLocation;

    return locationCoordinate;
}

void drawLives(int numberOfLives) {
    static ALLEGRO_FONT *font = al_create_builtin_font();

    al_draw_textf(font, al_map_rgb(255, 255, 255),
                5, 5, ALLEGRO_ALIGN_LEFT, "Lives: %d", numberOfLives);

}

void setBallsStuckToMove(vector<Ball*> *balls) {
    for (int i = 0; i < balls->size(); ++i) {
        Ball *ball = (*balls)[i];
        if (ball->ballState == BALLSTATE_STUCK) {
            ball->ballState = MOVING;
        }
    }
}

void resetGame(vector<Ball*> *balls, vector<Laser*> *lasers, vector<PowerUp*> *powerUps,
               Paddle *paddle, ALLEGRO_DISPLAY *display) {

    int displayWidth = al_get_display_width(display);

    deleteBalls(balls);
    deleteLaserBullets(lasers);
    deletePowerUps(powerUps);

    paddle->width = INITIAL_PADDLE_WIDTH;
    paddle->height = INITIAL_PADDLE_HEIGHT;

    vector<int> paddleStartLocation;
    paddleStartLocation = getPaddleStartLocation(display, paddle, CENTERBOTTOM);

    paddle->x = paddleStartLocation[0];
    paddle->y = paddleStartLocation[1];
    paddle->colorFilled = al_map_rgb(192, 192, 192);
    paddle->colorBorder = al_map_rgb(105, 105, 105);
    paddle->thickness = 5;
    paddle->paddleType = PADDLETYPE_NORMAL;
    paddle->isInRecoil = false;


    Ball *ball = new Ball();
    ball->radius = INITIAL_BALL_RADIUS;

    vector<int> ballStartLocation;
    ballStartLocation = getBallStartLocation(display, paddle, ball, CENTERBOTTOM);
    ball->x = ballStartLocation[0];
    ball->y = ballStartLocation[1];
    ball->speed = INITIAL_BALL_SPEED;
    ball->colorFilled = ballColors[0];
    ball->colorBorder = ballColors[1];
    ball->thickness = BALL_BORDER_THICKNESS;
    ball->ballState = NOTRELEASED;
    ball->releaseDirection = RELEASED_NONE;
    ball->ballType = BALLTYPE_NORMAL;


    balls->push_back(ball);

    al_set_mouse_xy(display, displayWidth/2, paddle->y);

    --numberOfLives;

}

void drawLevelBorder(ALLEGRO_DISPLAY *display) {

    int displayWidth = al_get_display_width(display);
    int displayHeight = al_get_display_width(display);

    al_draw_rectangle(0, 0, displayWidth, displayHeight, levelBorderColor[0], 5);
}

int WINAPI WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{

    srand (time(NULL));

    al_init();
    al_init_primitives_addon();
    al_install_mouse();
    al_init_font_addon();

    //al_set_new_display_flags(ALLEGRO_FULLSCREEN);

    ALLEGRO_EVENT_QUEUE *eventQueue = al_create_event_queue();
    ALLEGRO_DISPLAY *display = al_create_display(800, 600);

    ALLEGRO_EVENT e;
    ALLEGRO_TIMER *FPSTimer = NULL;
    ALLEGRO_TIMER *LPSTimer = NULL;

    FPSTimer = al_create_timer(1.0 / FPS);
    LPSTimer = al_create_timer(1.0 / LPS);
    LBRTimer = al_create_timer(LASER_GUN_RECOIL_TIME);

    al_set_window_title(display, "Breakout");
    al_register_event_source(eventQueue, al_get_display_event_source(display));
    al_register_event_source(eventQueue, al_get_timer_event_source(FPSTimer));
    al_register_event_source(eventQueue, al_get_timer_event_source(LPSTimer));
    al_register_event_source(eventQueue, al_get_timer_event_source(LBRTimer));
    al_register_event_source(eventQueue, al_get_mouse_event_source());

    al_clear_to_color(al_map_rgb(0,0,0));
    al_flip_display();

    al_start_timer(FPSTimer);
    al_start_timer(LPSTimer);


    Paddle *paddle = new Paddle();
    vector<Ball*> balls;
    vector<PowerUp*> powerUps;
    vector<Laser*> lasers;

    al_hide_mouse_cursor(display);

    BrickMap *brickMap = new BrickMap();
    setMap(brickMap);
    setColors();

    setTexts();


    numberOfLives = 4;

    resetGame(&balls, &lasers, &powerUps, paddle, display);

    gameState = GAMESTATE_RUNNING;

    while (gameState != GAMESTATE_EXIT) {

        static int displayWidth = al_get_display_width(display);
        static int displayHeight = al_get_display_height(display);

        al_wait_for_event(eventQueue, &e);

        if (e.timer.source == LBRTimer) {
            paddle->isInRecoil = false;
            al_stop_timer(LBRTimer);
        } else if(e.timer.source == LPSTimer) {

            if (gameState == GAMESTATE_PLAYERDIED) {
                continue;
            }

            setBallsPosition(&balls, paddle, brickMap, &powerUps, display);
            setPowerUpsPosition(&powerUps, paddle, &balls, display);
            setLaserBulletsPosition(&lasers, brickMap);

            cout << brickMap->getNumberOfActiveBricks() << endl;
            if (brickMap->getNumberOfActiveBricks() == 0) {
                gameState == GAMESTATE_EXIT;
            }

        } else if (e.type == ALLEGRO_EVENT_MOUSE_AXES) {

            if (gameState == GAMESTATE_PLAYERDIED) {
                continue;
            }

            setPaddlePositionByMouseEvent(e.mouse, display, paddle);
            setBallPositionByMouseEvent(e.mouse, display, paddle, balls[0]);
        } else if (e.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {

            if (gameState == GAMESTATE_PLAYERDIED) {
                continue;
            }

            ALLEGRO_MOUSE_STATE state;

            al_get_mouse_state(&state);

            if (paddle->paddleType == PADDLETYPE_LASER) {
                if (state.buttons & 1) {
                    cout << "Create Laser Beam " << endl;

                    LASERTYPE laserType;

                    if (balls[0]->ballType == BALLTYPE_THRUBALL) {
                        laserType = LASERTYPE_THRU;
                    } else if (balls[0]->ballType == BALLTYPE_NORMAL) {
                        laserType = LASERTYPE_NORMAL;
                    }

                    createLaserBeams(&lasers, paddle, laserType);

                }
            }

            setBallsStuckToMove(&balls);

            if (balls[0]->ballState == NOTRELEASED) {

                balls[0]->ballState = RELEASED;

                if (state.buttons & 1) {
                    balls[0]->releaseDirection = RELEASED_LEFT;
                }
                if (state.buttons & 2) {
                    balls[0]->releaseDirection = RELEASED_RIGHT;
                }
            }

        } else if(e.timer.source == FPSTimer) {

            static int fadeValue = 0;
            al_clear_to_color(al_map_rgb(0,0,0));


            drawLaserBullets(&lasers);
            drawPaddle(paddle);
            drawBrickMap(brickMap);
            drawBalls(&balls);
            drawPowerUps(&powerUps);
            drawLives(numberOfLives);
            drawLevelBorder(display);

            if (gameState == GAMESTATE_PLAYERDIED) {
                fadeValue += SCREEN_FADE_INCREMENT;
                al_draw_filled_rectangle(0, 0, displayWidth, displayHeight,
                    al_map_rgba(0,0,0,fadeValue));

                if (fadeValue >= 255) {
                    resetGame(&balls, &lasers, &powerUps, paddle, display);

                    if (numberOfLives == -1) {
                        gameState = GAMESTATE_EXIT;
                    } else {
                        gameState = GAMESTATE_RUNNING;
                    }
                    fadeValue = 0;

                }

            }


            al_flip_display();
        } else if(e.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            gameState = GAMESTATE_EXIT;
        }

    }


    delete paddle;
    delete brickMap;
    deleteBalls(&balls);
    deletePowerUps(&powerUps);
    deleteLaserBullets(&lasers);


    al_destroy_timer(FPSTimer);
    al_destroy_timer(LPSTimer);
    al_destroy_display(display);
    al_destroy_event_queue(eventQueue);
    al_shutdown_primitives_addon();
    al_uninstall_mouse();

  return 0;
}
