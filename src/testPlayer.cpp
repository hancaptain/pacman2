// 目前用于测试判断死路的功能
// by wd

#include "pacman.h"

using namespace Pacman;

GameField gameField;

bool isDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
bool scannedDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
int arrWallCount[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

void scanDeadRoad(int row, int col, int level)
{
    if (scannedDeadRoad[row][col]) return;
    // for (int i = 0; i < level; ++i) cout << " ";
    // cout << row << " " << col << " "
    // << gameField.fieldStatic[row][col] << endl;
    scannedDeadRoad[row][col] = true;

    int wallCount = 0;  // 旁边的墙或死路
    for (int d = 0; d < 4; ++d)
    {
        if (gameField.fieldStatic[row][col] & (1 << d))
            ++wallCount;
        else
        {
            int newRow = (row + dy[d] + gameField.height) % gameField.height;
            int newCol = (col + dx[d] + gameField.width) % gameField.width;
            scanDeadRoad(newRow, newCol, level + 1);
            if (isDeadRoad[newRow][newCol]) ++wallCount;
        }
    }
    if (wallCount >= 3) isDeadRoad[row][col] = true;
    arrWallCount[row][col] = wallCount;
}

void scanAllDeadRoad()
{
    memset(&isDeadRoad, 0, sizeof(isDeadRoad));
    memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
    for (int i = 0; i < gameField.height; ++i)
        for (int j = 0; j < gameField.width; ++j) scanDeadRoad(i, j, 0);
    memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
    for (int i = 0; i < gameField.height; ++i)
        for (int j = 0; j < gameField.width; ++j)
            scanDeadRoad(gameField.height - i, gameField.height - j, 0);
}

int main()
{
    string data, globalData;
    int myID = gameField.ReadInput("input.txt", data, globalData);

    scanAllDeadRoad();
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << isDeadRoad[i][j];
        cout << endl;
    }
    cout << endl;
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << arrWallCount[i][j];
        cout << endl;
    }
    cout << endl;

    gameField.WriteOutput((Direction)(-1), "SERVER_STOP", data, globalData);
    return 0;
}
