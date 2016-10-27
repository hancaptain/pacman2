// by wd

#include "helpers.h"
#include "pacman.h"
#include "utils.h"

#define NO_CHOICE (Direction)(-2)
#define RUN_AWAY_DISTANCE 2

int main()
{
    GameField gameField;
#ifdef DEFINE_DEBUG_STR
    gameField.DEBUG_STR = false;
#endif
    string data, globalData;  // 这是回合之间可以传递的信息
    string tauntText = "";

    int myID = gameField.ReadInput("input.txt", data, globalData);

    // 自己已死
    if (gameField.players[myID].dead)
    {
        gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    srand(seed + myID + gameField.turnID);

    int height = gameField.height;
    int width = gameField.width;
    int* a = (int*)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, a, true);  // 考虑绕开力量不小于自己的人之后的距离
    scanAllDeadRoad(gameField);

    Direction choice = NO_CHOICE;

    // 逃离比自己力量大的人
    if (choice == NO_CHOICE)
    {
        tauntText = "run away";
        int minDis = INFINITY_DISTANCE;
        int r1, c1;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player& _p = gameField.players[i];
            if (_p.dead) continue;
            if (_p.strength <= gameField.players[myID].strength) continue;
            int r = _p.row;
            int c = _p.col;
            int nowDis = DISTANCE(a, r, c, gameField.players[myID].row,
                                  gameField.players[myID].col);
            if (nowDis < minDis && nowDis <= RUN_AWAY_DISTANCE)
            {
                minDis = nowDis;
                r1 = r;
                c1 = c;
            }
        }

        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " from " + to_string(r1) + " " + to_string(c1);
            // 寻找最近的不是死路的格子
            minDis = INFINITY_DISTANCE;
            for (int r = 0; r < height; r++)
                for (int c = 0; c < width; c++)
                    if (!isDeadRoad[r][c])
                    {
                        int nowDis =
                            DISTANCE(a, r, c, gameField.players[myID].row,
                                     gameField.players[myID].col);
                        if (nowDis < minDis)
                        {
                            minDis = nowDis;
                            r1 = r;
                            c1 = c;
                        }
                    }
        }

        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, gameField.players[myID].row,
                                  gameField.players[myID].col, r1, c1, a);
        }
    }

    // 贪心，吃能比其他人先吃到的果子中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat fruit";
        int minDis = INFINITY_DISTANCE;
        int r1, c1;
        for (int r = 0; r < height; r++)
            for (int c = 0; c < width; c++)
                if (gameField.fieldContent[r][c] & (smallFruit | largeFruit))
                {
                    int dis = INFINITY_DISTANCE;
                    int id;
                    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                    {
                        Player& _p = gameField.players[i];
                        if (_p.dead) continue;
                        if (DISTANCE(a, r, c, _p.row, _p.col) < dis)
                        {
                            dis = DISTANCE(a, r, c, _p.row, _p.col);
                            id = i;
                        }
                    }
                    if (id == myID && dis < minDis)
                    {
                        minDis = dis;
                        r1 = r;
                        c1 = c;
                    }
                }
        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, gameField.players[myID].row,
                                  gameField.players[myID].col, r1, c1, a);
        }
    }

    // 贪心，吃比自己力量小的人中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat small enemy";
        int minDis = INFINITY_DISTANCE;
        int r1, c1;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player& _p = gameField.players[i];
            if (_p.dead) continue;
            if (_p.strength >= gameField.players[myID].strength) continue;
            int r = _p.row;
            int c = _p.col;
            int nowDis = DISTANCE(a, r, c, gameField.players[myID].row,
                                  gameField.players[myID].col);
            if (nowDis < minDis)
            {
                minDis = nowDis;
                r1 = r;
                c1 = c;
            }
        }
        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, gameField.players[myID].row,
                                  gameField.players[myID].col, r1, c1, a);
        }
    }

    // 随机模拟
    if (choice == NO_CHOICE)
    {
        tauntText = "random play";
        Helpers::RandomInit();
        for (int i = 0; i < 1000; i++) Helpers::RandomPlay(gameField, myID);
        for (int i = 0; i < MAX_DIRECTION; i++)
        {
            if (choice == NO_CHOICE ||
                Helpers::actionScore[i] > Helpers::actionScore[choice])
                choice = (Direction)i;
        }
        choice = (Direction)((int)choice - 1);
    }

    gameField.DebugPrint();
    gameField.WriteOutput(choice, tauntText, data, globalData);
    return 0;
}