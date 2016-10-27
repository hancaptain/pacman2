// 目前用于测试判断死路的功能
// by wd

#include "pacman.h"
#include "utils.h"

using namespace Pacman;

int main()
{
    GameField gameField;
    string data, globalData;
    int myID = gameField.ReadInput("input.txt", data, globalData);

    scanAllDeadRoad(gameField);
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << isDeadRoad[i][j];
        cout << endl;
    }
    cout << endl;
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << wallCount[i][j];
        cout << endl;
    }
    cout << endl;

    gameField.WriteOutput((Direction)(-1), "SERVER_STOP", data, globalData);
    return 0;
}
