#ifndef UTILS_H
#define UTILS_H

#include <deque>
#include "pacman.h"

using namespace Pacman;

///////////////////////////////////////////////////////////////////////////////
// module distance
// by cp

#define DISTANCE_NEAR_ENEMY 2
#define INFINITY_DISTANCE 1000

#define FIELD(a, r, c) *((a) + (r)*width + (c))
#define DISTANCE(a, r1, c1, r2, c2) \
    *((a) + ((r1)*width + (c1)) * width * height + (r2)*width + (c2))

// array is width * height * width * height
// array[r1][c1][r2][c2] means the shortest distance from (r1, c1) to (r2, c2)
// add considerEnemy by wd
void floyd(GameField& gameField, int* array, bool considerEnemy = true)
{
    int height = gameField.height;  // r
    int width = gameField.width;    // c
    int r1, c1, r2, c2, rt, ct, r, c;
    memset(array, 0x33,
           sizeof(int) * height * height * width * width);  // set to infinity
    for (r = 0; r < height; r++)
        for (c = 0; c < width; c++)
        {
            GridStaticType& type = gameField.fieldStatic[r][c];
            DISTANCE(array, r, c, r, c) = 0;
            for (int d = 0; d < 4; d++)
                if (!(type & direction2OpposingWall[d]))
                    DISTANCE(array, r, c, (r + dy[d] + height) % height,
                             (c + dx[d] + width) % width) = 1;
        }

    if (considerEnemy)
    {
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == gameField.myID) continue;
            Player& _p = gameField.players[i];
            if (_p.dead) continue;
            if (_p.strength < gameField.players[gameField.myID].strength)
                continue;
            int r = _p.row;
            int c = _p.col;
            GridStaticType& type = gameField.fieldStatic[r][c];
            for (int d = 0; d < 4; ++d)
                if (!(type & direction2OpposingWall[d]))
                    DISTANCE(array, r, c, (r + dy[d] + height) % height,
                             (c + dx[d] + width) % width) +=
                        DISTANCE_NEAR_ENEMY;
        }
    }

    for (rt = 0; rt < height; rt++)
        for (ct = 0; ct < width; ct++)
            for (r1 = 0; r1 < height; r1++)
                for (c1 = 0; c1 < width; c1++)
                    for (r2 = 0; r2 < height; r2++)
                        for (c2 = 0; c2 < width; c2++)
                        {
                            int temp = DISTANCE(array, rt, ct, r1, c1) +
                                       DISTANCE(array, rt, ct, r2, c2);
                            if (temp < DISTANCE(array, r1, c1, r2, c2))
                                DISTANCE(array, r1, c1, r2, c2) = temp;
                        }
}

// DEPRECATED
// return the first move from (r1, c1) to (r2, c2)
// return stay when an error occurs
// Direction dijkstra(GameField& gameField, int r1, int c1, int r2, int c2)
// {
// int height = gameField.height;
// int width = gameField.width;
// if (gameField.fieldStatic[r1][c1] & generator ||
// gameField.fieldStatic[r2][c2] & generator || (r1 == c1 && r2 == c2))
// return stay;
// int predirection[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
// int dis[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
// memset(dis, 0x33, sizeof(dis));
// dis[r1][c1] = 0;
// deque<int> row = {r1}, col = {c1};
// while (row.size())
// {
// int r = row.front(), c = col.front();
// row.pop_front();
// col.pop_front();
// for (int d = 0; d < 4; d++)
// if (!(gameField.fieldStatic[r][c] & direction2OpposingWall[d]))
// {
// int nr = (r + dy[d] + height) % height,
// nc = (c + dx[d] + width) % width;
// if (dis[nr][nc] > 1000)
// {
// dis[nr][nc] = dis[r][c] + 1;
// predirection[nr][nc] = d;
// row.push_back(nr);
// col.push_back(nc);
// if (nr == r2 && nc == c2) goto done;
// }
// }
// }
// return stay;
// done:
// int r = r2, c = c2;
// while (dis[r][c] > 1)
// {
// int rr = (r - dy[predirection[r][c]] + height) % height;
// int cc = (c - dx[predirection[r][c]] + width) % width;
// r = rr;
// c = cc;
// }
// return (Direction)predirection[r][c];
// }

// return the first move, make use of the result of floyd
// in O(1) time
Direction routineFloyd(GameField& gameField, int r1, int c1, int r2, int c2,
                       int* array)
{
    if (gameField.fieldStatic[r1][c1] & generator ||
        gameField.fieldStatic[r2][c2] & generator || (r1 == r2 && c1 == c2))
        return stay;
    int height = gameField.height;  // r
    int width = gameField.width;    // c
    int d;
    for (d = 0; d < 4; d++)
        if (!(gameField.fieldStatic[r1][c1] & direction2OpposingWall[d]))
        {
            int nr = (r1 + dy[d] + height) % height,
                nc = (c1 + dx[d] + width) % width;
            if (DISTANCE(array, r2, c2, r1, c1) ==
                DISTANCE(array, r2, c2, nr, nc) + 1)
                break;
        }
    return Direction(d);
}

///////////////////////////////////////////////////////////////////////////////
// module dead road
// by wd

bool isDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
bool scannedDeadRoad[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
int wallCount[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

void scanDeadRoad(GameField& gameField, int row, int col, int level)
{
    if (scannedDeadRoad[row][col]) return;
    // for (int i = 0; i < level; ++i) cout << " ";
    // cout << row << " " << col << " "
    // << gameField.fieldStatic[row][col] << endl;
    scannedDeadRoad[row][col] = true;

    int nowWallCount = 0;  // 旁边的墙或死路
    for (int d = 0; d < 4; ++d)
    {
        if (gameField.fieldStatic[row][col] & direction2OpposingWall[d])
            ++nowWallCount;
        else
        {
            int newRow = (row + dy[d] + gameField.height) % gameField.height;
            int newCol = (col + dx[d] + gameField.width) % gameField.width;
            scanDeadRoad(gameField, newRow, newCol, level + 1);
            if (isDeadRoad[newRow][newCol]) ++nowWallCount;
        }
    }
    if (nowWallCount >= 3) isDeadRoad[row][col] = true;
    wallCount[row][col] = nowWallCount;
}

void scanAllDeadRoad(GameField& gameField)
{
    memset(&isDeadRoad, 0, sizeof(isDeadRoad));
    // 从左上角和右下角各floodfill一次，如果只做一次在初始位置附近会出问题
    memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
    for (int i = 0; i < gameField.height; ++i)
        for (int j = 0; j < gameField.width; ++j)
            scanDeadRoad(gameField, i, j, 0);
    memset(&scannedDeadRoad, 0, sizeof(scannedDeadRoad));
    for (int i = 0; i < gameField.height; ++i)
        for (int j = 0; j < gameField.width; ++j)
            scanDeadRoad(gameField, gameField.height - i, gameField.height - j,
                         0);
}

///////////////////////////////////////////////////////////////////////////////
// module shoot
// by wd

// 根据坐标判断射击的方向
// 如果不能射到，输出stay
// f是射击者，t是被射者
// TODO：考虑一次射多人
Direction shootDirection(GameField& gameField, int fr, int fc, int tr, int tc)
{
    bool canShoot;
    int height = gameField.height;
    int width = gameField.width;
    if (fc == tc)
    {
        canShoot = true;
        for (int i = fr; i != tr; i = (i - 1 + height) % height)
        {
            if (gameField.fieldStatic[i][fc] & wallNorth)
            {
                canShoot = false;
                break;
            }
        }
        if (canShoot) return shootUp;

        canShoot = true;
        for (int i = fr; i != tr; i = (i + 1 + height) % height)
        {
            if (gameField.fieldStatic[i][fc] & wallSouth)
            {
                canShoot = false;
                break;
            }
        }
        if (canShoot) return shootDown;
    }
    if (fr == tr)
    {
        canShoot = true;
        for (int i = fc; i != tc; i = (i - 1 + width) % width)
        {
            if (gameField.fieldStatic[fr][i] & wallWest)
            {
                canShoot = false;
                break;
            }
        }
        if (canShoot) return shootLeft;

        canShoot = true;
        for (int i = fc; i != tc; i = (i + 1 + width) % width)
        {
            if (gameField.fieldStatic[fr][i] & wallEast)
            {
                canShoot = false;
                break;
            }
        }
        if (canShoot) return shootRight;
    }
    return stay;
}

Direction shootMustHit(GameField& gameField, int fr, int fc, int tr, int tc)
{
    Direction d = shootDirection(gameField, fr, fc, tr, tc);
    if (d == stay)
        return stay;
    else if (d == shootUp || d == shootDown)
    {
        if (gameField.fieldStatic[tr][tc] & wallWest &&
            gameField.fieldStatic[tr][tc] & wallEast)
            return d;
        else
            return stay;
    }
    else if (d == shootLeft | d == shootRight)
    {
        if (gameField.fieldStatic[tr][tc] & wallNorth &&
            gameField.fieldStatic[tr][tc] & wallSouth)
            return d;
        else
            return stay;
    }
}

#endif
