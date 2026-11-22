#include "Game.h"
#include "DrivingGame.h"

Game* CreateGame()
{
    return new DrivingGame;
}
