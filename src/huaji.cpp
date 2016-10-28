// by wd
// edit 1028am
// 写了一点射击的功能
// 计算距离和吃果子时不考虑力量不小于自己的人
// 把场上的所有果子存到allFruits
// 所有生成果子的位置存到fruitGenPlaces
// 用me表示自己
// 打算写一个计算每个人吃每个果子的可能性的算法，基于迭代

#include "helpers.h"
#include "pacman.h"
#include "utils.h"

#define NO_CHOICE (Direction)(-2)
#define RUN_AWAY_DISTANCE 2
#define EAT_ENEMY_DISTANCE 2

int main()
{
    GameField gameField;
#ifdef DEFINE_DEBUG_STR
    gameField.DEBUG_STR = false;
#endif
    string data, globalData;  // 这是回合之间可以传递的信息
    string tauntText = "";
    int myID = gameField.ReadInput("input.txt", data, globalData);
    int height = gameField.height;
    int width = gameField.width;
    Player& me = gameField.players[myID];

    // 自己已死
    if (me.dead)
    {
        gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    srand(seed + myID + gameField.turnID);
    int* a = (int*)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, a, true);  // 考虑绕开力量不小于自己的人之后的距离
    scanAllDeadRoad(gameField);
    scanAllFruits(gameField);

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
            if (_p.strength <= me.strength) continue;
            int r = _p.row;
            int c = _p.col;
            int nowDis = DISTANCE(a, r, c, me.row, me.col);
            if (nowDis <= RUN_AWAY_DISTANCE && nowDis < minDis)
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
                        int nowDis = DISTANCE(a, r, c, me.row, me.col);
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
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 射必中则射，目前射最远的人，TODO：考虑一次射多人
    if (choice == NO_CHOICE)
    {
        // 判断力量足够
        if ((me.powerUpLeft == 0 && me.strength > gameField.SKILL_COST) ||
            me.strength >
                gameField.SKILL_COST + gameField.LARGE_FRUIT_ENHANCEMENT)
        {
            tauntText = "shoot";
            int maxDis = -INFINITY_DISTANCE;
            int r1, c1;
            Direction d1;
            for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            {
                if (i == myID) continue;
                Player& _p = gameField.players[i];
                if (_p.dead) continue;
                int r = _p.row;
                int c = _p.col;
                Direction d = shootMustHit(gameField, me.row, me.col, r, c);
                // cout << gameField.turnID << " shoot test " << r << " " << c
                // << " " << d << endl;
                if (d == stay) continue;
                int nowDis = DISTANCE(a, r, c, me.row, me.col);
                // cout << nowDis << " " << maxDis << endl;
                if (nowDis > maxDis)
                {
                    maxDis = nowDis;
                    r1 = r;
                    c1 = c;
                    d1 = d;
                }
            }
            if (maxDis > -INFINITY_DISTANCE)
            {
                tauntText += " to " + to_string(r1) + " " + to_string(c1);
                choice = d1;
            }
        }
    }

    // 贪心，吃能比力量比自己大的人先吃到的果子中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat fruit";
        int minDis = INFINITY_DISTANCE;
        int r1, c1;
        for (int fru = 0; fru < allFruitsCount; ++fru)
        {
            int r = allFruits[fru].row;
            int c = allFruits[fru].col;
            int dis = INFINITY_DISTANCE;
            int id;
            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                Player& _p = gameField.players[i];
                if (_p.dead) continue;
                if (i != myID && _p.strength <= me.strength) continue;
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
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 贪心，吃比自己力量小的人中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat enemy";
        int minDis = INFINITY_DISTANCE;
        int r1, c1, str1 = 0;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player& _p = gameField.players[i];
            if (_p.dead) continue;
            if (_p.strength >= me.strength) continue;
            int r = _p.row;
            int c = _p.col;
            int nowDis = DISTANCE(a, r, c, me.row, me.col);
            // cout << gameField.turnID << " eat enemy test " << r << " " << c
            // << " " << nowDis << " " << minDis << endl;
            // 距离相同则吃力量大者
            if (nowDis <= EAT_ENEMY_DISTANCE &&
                (nowDis < minDis || (nowDis == minDis && _p.strength > str1)))
            {
                minDis = nowDis;
                r1 = r;
                c1 = c;
                str1 = _p.strength;
            }
        }
        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 随机模拟
    if (choice == NO_CHOICE)
    {
        tauntText = "random play";
        Helpers::RandomInit();
        for (int i = 0; i < 1000; i++) Helpers::RandomPlay(gameField, myID);
        for (int i = 0; i < 4; i++)
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
