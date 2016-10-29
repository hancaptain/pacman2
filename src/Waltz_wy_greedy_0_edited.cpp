#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include "pacman.h"

const int timeLimit = 0.1 * CLOCKS_PER_SEC;  // edited 0.9

class GameField2 : public GameField
{
   public:
    // 判断指定玩家向指定方向移动是不是合法的（没有撞墙且没有踩到豆子产生器）
    inline bool ActionValid(int playerID, Direction &dir) const
    {
        if (dir == stay) return true;
        const Player &p = players[playerID];
        const GridStaticType &s = fieldStatic[p.row][p.col];
        return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]) &&
               !(s & generator);
    }

    //判断指定位置向指定方向移动是不是合法的
    inline bool ActionValid(const int &y, const int &x, Direction &dir)
    {
        if (dir == stay) return true;
        const GridStaticType &s = fieldStatic[y][x];
        return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]) &&
               !(s & generator);
    }

    inline bool ActionWise(int playerID, Direction &dir) const
    {
        int ny = players[playerID].row, nx = players[playerID].col;
        if (dir != stay)
        {
            ny = (ny + dy[dir] + height) % height;
            nx = (nx + dx[dir] + width) % width;
        }
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (players[i].dead || i == playerID ||
                players[i].strength <= players[playerID].strength)
                continue;
            if (shortPath[ny][nx].dist[players[i].row][players[i].col] <= 1)
                return false;
        }
        return true;
    }

    class searchNode
    {
       public:
        int y, x;
        searchNode(){};
        searchNode(int _y, int _x) : x(_x), y(_y){};
    };

    class sPath
    {
       public:
        int dist[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        Direction dir[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        sPath()
        {
            memset(dist, 0, sizeof(dist));
            memset(dir, 0, sizeof(dir));
        }
    };

    sPath shortPath[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

    int num[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH],
        vis[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH], blocks[FIELD_MAX_HEIGHT];

    void BFS(int y1, int x1)
    {
        int visited[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(visited, 0, sizeof(visited));
        std::queue<searchNode> q;
        while (!q.empty()) q.pop();

        q.push(searchNode(y1, x1));
        visited[y1][x1] = 1;
        shortPath[y1][x1].dist[y1][x1] = 0;
        shortPath[y1][x1].dir[y1][x1] = stay;

        while (!q.empty())
        {
            searchNode index = q.front();
            q.pop();

            for (Direction _d = up; _d < 4; ++_d)
            {
                if (ActionValid(index.y, index.x, _d) &&
                    !visited[(index.y + dy[_d] + height) % height]
                            [(index.x + dx[_d] + width) % width])
                {
                    int ty = (index.y + dy[_d] + height) % height,
                        tx = (index.x + dx[_d] + width) % width;
                    q.push(searchNode(ty, tx));
                    visited[ty][tx] = 1;
                    shortPath[y1][x1].dist[ty][tx] =
                        shortPath[y1][x1].dist[index.y][index.x] + 1;
                    shortPath[y1][x1].dir[ty][tx] =
                        (index.x == x1 && index.y == y1)
                            ? _d
                            : shortPath[y1][x1].dir[index.y][index.x];
                }
            }
        }
    }

    void FindAllPath()
    {
        for (int _y = 0; _y < height; _y++)
        {
            for (int _x = 0; _x < width; _x++)
            {
                if (!(fieldStatic[_y][_x] & generator)) BFS(_y, _x);
            }
        }
    }

    class dGrid
    {
       public:
        int type;  // 0:free 1:deadroad 2:entrance
        searchNode entry;
        int fruitTopick;
        dGrid()
        {
            type = fruitTopick = 0;
        }
    };

    dGrid deadGrid[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

    int validDirection(int y, int x)
    {
        int temp = 0;
        for (Direction i = up; i < 4; ++i)
        {
            if (ActionValid(y, x, i)) temp++;
        }
        return temp;
    }

    void CheckGrid()
    {
        for (int _y = 0; _y < height; _y++)
        {
            for (int _x = 0; _x < width; _x++)
            {
                if (!(fieldStatic[_y][_x] & generator) &&
                    validDirection(_y, _x) == 1)
                {
                    std::stack<searchNode> s;
                    int ty = _y, tx = _x;
                    deadGrid[ty][tx].fruitTopick =
                        (fieldContent[ty][tx] & smallFruit) ? 1 : 0;
                    if (fieldContent[ty][tx] & largeFruit)
                        deadGrid[ty][tx].fruitTopick += LARGE_FRUIT_ENHANCEMENT;
                    while (validDirection(ty, tx) <= 2)
                    {
                        s.push(searchNode(ty, tx));
                        deadGrid[ty][tx].type = 1;
                        Direction i;
                        for (i = up; i < 4; ++i)
                            if (ActionValid(ty, tx, i) &&
                                deadGrid[ty + dy[i]][tx + dx[i]].type != 1)
                                break;

                        int pre_ty = ty, pre_tx = tx;
                        ty = (ty + dy[i] + height) % height;
                        tx = (tx + dx[i] + width) % width;

                        deadGrid[ty][tx].fruitTopick =
                            deadGrid[pre_ty][pre_tx].fruitTopick;
                        if (fieldContent[ty][tx] & smallFruit)
                            deadGrid[ty][tx].fruitTopick += 1;
                        if (fieldContent[ty][tx] & largeFruit)
                            deadGrid[ty][tx].fruitTopick +=
                                LARGE_FRUIT_ENHANCEMENT;
                    }
                    deadGrid[ty][tx].type = 2;
                    deadGrid[ty][tx].fruitTopick = 0;
                    while (!s.empty())
                    {
                        deadGrid[s.top().y][s.top().x].entry =
                            searchNode(ty, tx);
                        s.pop();
                    }
                }
            }
        }
    }

    bool notInDeadEnd(int myID, Direction dir)
    {
        int ny = players[myID].row;
        int nx = players[myID].col;
        if (dir != stay)
        {
            ny = (ny + dy[dir] + height) % height;
            nx = (nx + dx[dir] + width) % width;
        }
        if (deadGrid[ny][nx].type != 1) return true;
        int ex = deadGrid[ny][nx].entry.x, ey = deadGrid[ny][nx].entry.y;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (players[i].dead || myID == i ||
                players[i].strength <= players[myID].strength)
                continue;
            if (shortPath[ey][ex].dist[players[i].row][players[i].col] - 1 <=
                    shortPath[ey][ex].dist[ny][nx] &&
                players[i].strength >
                    players[myID].strength + deadGrid[ny][nx].fruitTopick)
                return false;
        }
        return true;
    }

    double gridEvaluation(int _y, int _x, int myID)
    {
        int visited[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(visited, 0, sizeof(visited));
        int dist[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(dist, 1, sizeof(dist));
        int pickfruit[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(pickfruit, 0, sizeof(pickfruit));
        std::queue<searchNode> q;
        while (!q.empty()) q.pop();

        q.push(searchNode(players[myID].row, players[myID].col));
        visited[players[myID].row][players[myID].col] = 1;
        dist[players[myID].row][players[myID].col] = 0;
        while (!q.empty())
        {
            searchNode index = q.front();
            q.pop();
            if (index.y == _y && index.x == _x) break;
            for (Direction _d = up; _d < 4; ++_d)
            {
                if (ActionValid(index.y, index.x, _d) &&
                    !visited[(index.y + dy[_d] + height) % height]
                            [(index.x + dx[_d] + width) % width])
                {
                    int ty = (index.y + dy[_d] + height) % height,
                        tx = (index.x + dx[_d] + width) % width;
                    bool Wise = true;
                    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                    {
                        if ((fieldContent[ty][tx] & playerID2Mask[i]) &&
                            players[i].strength > players[myID].strength)
                            Wise = false;
                    }
                    if (!Wise) continue;

                    q.push(searchNode(ty, tx));
                    visited[ty][tx] = 1;
                    dist[ty][tx] = dist[index.y][index.x] + 1;
                    pickfruit[ty][tx] = pickfruit[index.y][index.x];
                    if ((fieldContent[ty][tx] & smallFruit) |
                        (fieldContent[ty][tx] & largeFruit))
                        pickfruit[ty][tx]++;
                }
            }
        }

        if (visited[_y][_x])
        {
            double FruitExpectation = 0;
            int Node = 0;
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    if ((fieldContent[i][j] & smallFruit) |
                        (fieldContent[i][j] & largeFruit))
                    {
                        ++Node;
                        FruitExpectation +=
                            1.0 / std::max(shortPath[_y][_x].dist[i][j], 1);
                    }
                }
            }
            FruitExpectation /= Node;

            double ToGenerator = 0;
            for (int i = 0; i < generatorCount; i++)
            {
                double temp = 0;
                for (int d = 0; d < 8; d++)
                {
                    int ny = (generators[i].row + dy[d] + height) % height,
                        nx = (generators[i].col + dx[d] + width) % width;
                    temp += 1.0 / std::max(shortPath[_y][_x].dist[ny][nx], 1);
                }
                ToGenerator += temp / 8;
            }

            return pickfruit[_y][_x] + FruitExpectation * 2;
        }
        else
        {
            return -1;
        }
    }

    double getStrenth(int playersID)
    {
        const Player &p = players[playersID];
        if (p.powerUpLeft == 0)
            return p.strength;
        else
        {
            return (p.strength - LARGE_FRUIT_ENHANCEMENT) + 1;
        }
    }
};

double Evaluation(GameField2 &gF, int myID)
{
    //考虑：豆子距离，当前玩家吃到的豆子数，到生成器的时空关系
    double FruitExpectation = 0;
    int Node_small = 0, Node_large = 0, rank = 0;
    Player p = gF.players[myID];
    for (int player = 0; player < MAX_PLAYER_COUNT; player++)
    {
        if (gF.getStrenth(player) > gF.getStrenth(myID)) rank++;
    }
    for (int i = 0; i < gF.height; i++)
    {
        for (int j = 0; j < gF.width; j++)
        {
            if (gF.fieldContent[i][j] & smallFruit)
            {
                ++Node_small;
                FruitExpectation +=
                    1.0 / std::max(gF.shortPath[p.row][p.col].dist[i][j], 1);
            }
            else if (gF.fieldContent[i][j] & largeFruit)
            {
                ++Node_large;
                FruitExpectation +=
                    (rank + 1) /
                    std::max(gF.shortPath[p.row][p.col].dist[i][j], 1);
            }
        }
    }

    double ToGenerator = 0;
    for (int i = 0; i < gF.generatorCount; i++)
    {
        double temp = 0;
        for (int d = 0; d < 8; d++)
        {
            int ny = (gF.generators[i].row + dy[d] + gF.height) % gF.height,
                nx = (gF.generators[i].col + dx[d] + gF.width) % gF.width;
            temp += 1.0 / std::max(gF.shortPath[p.row][p.col].dist[ny][nx], 1);
            for (int player = 0; player < MAX_PLAYER_COUNT;
                 player++)  //虐菜判断（赶走弱者）
            {
                double advantage =
                           gF.getStrenth(player) < (gF.getStrenth(myID) - 5),
                       dist = gF.shortPath[gF.players[player].row]
                                          [gF.players[player].col]
                                              .dist[ny][nx] <
                              gF.shortPath[p.row][p.col].dist[ny][nx];
                temp += advantage * dist / 4;
            }
        }
        ToGenerator += temp / 8;
    }

    return 15 * gF.getStrenth(myID) + 2 * FruitExpectation +
           3 * ToGenerator / gF.generatorTurnLeft;
}

bool overtime = false;

double GreedySearch(GameField2 &gF, int depth, int myID, int next)
{
    if (overtime) return 0;
    if (clock() > timeLimit)
    {
        overtime = true;
        return 0;
    }
    if (!next)
    {
        return 0;
    }

    if (depth == 0)
    {
        return Evaluation(gF, myID);
    }
    else
    {
        double bestScore = 0;
        for (Direction _d = Direction::up; _d < 4; ++_d)
        {
            if (gF.ActionValid(myID, _d) && gF.ActionWise(myID, _d))
            {
                for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                    gF.actions[i] = Direction::stay;
                gF.actions[myID] = _d;
                bool hasNext = gF.NextTurn();
                if (gF.players[myID].dead) hasNext = false;
                double temp;
                if (hasNext)
                {
                    temp = GreedySearch(gF, depth - 1, myID, hasNext);
                    bestScore = std::max(bestScore, temp);
                }
                gF.PopState();
            }
        }
        return Evaluation(gF, myID) + bestScore;
    }
}

int main()
{
    GameField2 GameField2;
    string data, globalData;
    int myID = GameField2.ReadInput("input.txt", data, globalData);

    if (GameField2.players[myID].dead)
    {
        GameField2.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    Player o = GameField2.players[myID];
    GameField2.FindAllPath();
    GameField2.CheckGrid();

    Direction ans = Direction::stay, finalAns = ans;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (GameField2.players[i].dead || i == myID) continue;
        if (GameField2.players[i].strength <
                GameField2.players[myID].strength &&
            GameField2
                    .deadGrid[GameField2.players[i].row]
                             [GameField2.players[i].col]
                    .type == 1)
        {
            int _y = GameField2
                         .deadGrid[GameField2.players[i].row]
                                  [GameField2.players[i].col]
                         .entry.y;
            int _x = GameField2
                         .deadGrid[GameField2.players[i].row]
                                  [GameField2.players[i].col]
                         .entry.x;
            if (GameField2.shortPath[_y][_x].dist[GameField2.players[i].row]
                                                 [GameField2.players[i].col] >=
                    GameField2.shortPath[_y][_x]
                        .dist[GameField2.players[myID].row]
                             [GameField2.players[myID].col] &&
                GameField2.ActionWise(
                    myID, GameField2
                              .shortPath[GameField2.players[myID].row]
                                        [GameField2.players[myID].col]
                              .dir[GameField2.players[i].row]
                                  [GameField2.players[i].col]))
            {
                ans = GameField2
                          .shortPath[GameField2.players[myID].row]
                                    [GameField2.players[myID].col]
                          .dir[GameField2.players[i].row]
                              [GameField2.players[i].col];
            }
        }
    }
    if (ans != Direction::stay)
    {
        GameField2.WriteOutput(ans, "Got'ya!!!", data, globalData);
        return 0;
    }

    bool Threat = false;
    int depth = 6;
    while (!overtime)
    {
        int Can[5] = {0};
        double Best[5] = {0};
        double maxB = -100000;
        depth++;
        for (Direction _d = Direction::stay; _d < 4; ++_d)
        {
            if (GameField2.ActionValid(myID, _d) &&
                GameField2.ActionWise(myID, _d) &&
                GameField2.notInDeadEnd(myID, _d))
            {
                Can[int(_d) + 1] = 1;
                for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                    GameField2.actions[i] = Direction::stay;
                GameField2.actions[myID] = _d;
                bool hasNext = GameField2.NextTurn();
                Best[int(_d) + 1] =
                    GreedySearch(GameField2, depth, myID, hasNext);
                GameField2.PopState();

                if (maxB < Best[int(_d) + 1])
                {
                    maxB = Best[int(_d) + 1];
                    ans = _d;
                }
            }
        }
        if (maxB == -100000)
        {
            for (Direction _d = Direction::stay; _d < 4; ++_d)
            {
                if (GameField2.ActionValid(myID, _d) &&
                    GameField2.ActionWise(myID, _d))
                {
                    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                        GameField2.actions[i] = Direction::stay;
                    GameField2.actions[myID] = _d;
                    bool hasNext = GameField2.NextTurn();
                    Best[int(_d) + 1] =
                        GreedySearch(GameField2, depth, myID, hasNext);
                    GameField2.PopState();

                    if (maxB < Best[int(_d) + 1])
                    {
                        maxB = Best[int(_d) + 1];
                        ans = _d;
                    }
                }
            }
        }
        if (!overtime) finalAns = ans;
        if (clock() > timeLimit) overtime = true;
    }

    GameField2.DebugPrint();
    GameField2.WriteOutput(finalAns, "", data, globalData);
    return 0;
}
