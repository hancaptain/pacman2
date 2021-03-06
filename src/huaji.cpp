// by wd
//
// edit 1028am
// 写了一点射击的功能
// 计算距离和吃果子时不考虑力量不小于自己的人
// 把场上的所有果子存到allFruits
// 所有生成果子的位置存到fruitGenPlaces
// 用me表示自己
// 打算写一个计算每个人吃每个果子的可能性的算法，基于迭代
//
// edit 1028pm
// 给每个格子打分，逃跑和吃果子时距离相同时考虑分数
// 防止力量相等的玩家站在同一格上谁都吃不到果子
// 比较力量的函数cmpStr
//
// edit 1029am
// 接下来要考虑怎么射，目前地图上有长直线时会被虐
// data的初始化和写入移到模块外面
// 移动到生成果子的位置时，距离相同时考虑分数
// 计算分数时考虑力量与自己相等和比自己大的人
//
// edit 1029pm
// 逃跑时周围4格有果子且吃后能反超则吃
// 如果连续两回合跟别人站在同一格上， 第二回合往第一回合的位置射一发

#include "pacman.h"
#include "utils.h"

#define NO_CHOICE (Direction)(-2)
#define RUN_AWAY_DISTANCE 2
#define EAT_ENEMY_DISTANCE 2
#define CHANGE_TARGET_SCORE 1.0
#define SAME_ENEMY_SCORE -0.5
#define LARGE_ENEMY_SCORE -2.0

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
    Player &me = gameField.players[myID];
    srand(seed + myID + gameField.turnID);

    // 自己已死
    if (me.dead)
    {
        gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    // 初始化data
    if (data == "")
        data =
            "{\"s\":-1000,\"r\":-1,\"c\":-1,\"p\":[[-1,-1],[-1,-1],[-1,-1],[-1,-1]],\"a\":0}";
    Json::Reader reader;
    Json::Value js;
    reader.parse(data, js);

    // 分析场地
    int *a = (int *)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, a, true);  // 考虑绕开力量不小于自己的人之后的距离
    int *realDis = (int *)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, realDis, false);  // 真实距离
    scanAllDeadRoad(gameField);
    scanAllFruits(gameField);
    scanFruitGenPlaces(gameField);
    scanAllCmpStr(gameField, realDis);

    int nextGenTurn = 0;  // 下次生成果子的回合
    while (nextGenTurn < gameField.turnID)
        nextGenTurn += gameField.GENERATOR_INTERVAL;
    int leftGenTurn = nextGenTurn - gameField.turnID;

    // 给每个格子打分
    // 对每个果子，玩家能赶到的生成果子的位置，力量比自己小的人，
    // 分数 += 2^(-dis)
    // 对每个力量与自己相等的人，
    // 分数 += SAME_ENEMY_SCORE * 2^(-dis)
    // 对每个力量比自己大的人，
    // 分数 += LARGE_ENEMY_SCORE * 2^(-dis)
    // 分数高的格子一般不是死路
    double fieldScore[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
    for (int r = 0; r < gameField.height; ++r)
        for (int c = 0; c < gameField.width; ++c)
        {
            fieldScore[r][c] = 0;
            for (int i = 0; i < allFruitsCount; ++i)
            {
                int dis = DISTANCE(a, r, c, allFruits[i].row, allFruits[i].col);
                fieldScore[r][c] += pow(2, -dis);
            }
            for (int i = 0; i < fruitGenPlacesCount; ++i)
            {
                int dis = DISTANCE(a, r, c, fruitGenPlaces[i].row,
                                   fruitGenPlaces[i].col);
                if (dis <= leftGenTurn) fieldScore[r][c] += pow(2, -dis);
            }
            for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            {
                if (i == myID) continue;
                Player &p = gameField.players[i];
                if (p.dead) continue;
                int dis = DISTANCE(a, r, c, p.row, p.col);
                if (allCmpStr[myID][i] > 0)
                    fieldScore[r][c] += pow(2, -dis);
                else if (allCmpStr[myID][i] == 0)
                    fieldScore[r][c] += SAME_ENEMY_SCORE * pow(2, -dis);
                else
                    fieldScore[r][c] += LARGE_ENEMY_SCORE * pow(2, -dis);
            }
        }

    Direction choice = NO_CHOICE;

    // TODO：在做所有动作时防止被射，走进死路

    // 逃离附近比自己力量大的人
    if (choice == NO_CHOICE)
    {
#ifndef _BOTZONE_ONLINE
        tauntText = "run away";
#endif
        int minDis = INFINITY_DISTANCE;
        int r1, c1, id1;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player &p = gameField.players[i];
            if (p.dead) continue;
            if (allCmpStr[myID][i] >= 0) continue;
            int r = p.row;
            int c = p.col;
            // 使用真实距离
            int nowDis = DISTANCE(realDis, r, c, me.row, me.col);
            if (nowDis <= RUN_AWAY_DISTANCE && nowDis < minDis)
            {
                minDis = nowDis;
                r1 = r;
                c1 = c;
                id1 = i;
            }
        }

        if (minDis < INFINITY_DISTANCE)
        {
// 附近有敌人，id为id1
#ifndef _BOTZONE_ONLINE
            tauntText += " from " + to_string(r1) + " " + to_string(c1);
#endif
            // 看周围4格是否有果子且吃后能反超
            double maxScore = -INFINITY_DISTANCE;
            Direction d2 = stay;
            for (int d = 0; d < 4; ++d)
            {
                if (gameField.fieldStatic[me.row][me.col] &
                    direction2OpposingWall[d])
                    continue;
                int tr = (me.row + dy[d] + height) % height;
                int tc = (me.col + dx[d] + width) % width;
                if (((gameField.fieldStatic[tr][tc] & smallFruit &&
                      cmpStr(gameField, realDis, me, gameField.players[id1], 1,
                             0) >= 0) ||
                     (gameField.fieldStatic[tr][tc] & largeFruit &&
                      cmpStr(gameField, realDis, me, gameField.players[id1],
                             gameField.LARGE_FRUIT_ENHANCEMENT, 0) >= 0)) &&
                    fieldScore[tr][tc] > maxScore)
                {
                    maxScore = fieldScore[tr][tc];
                    d2 = (Direction)d;
                }
            }

            if (d2 != stay)
            {
// 找到上述果子
#ifndef _BOTZONE_ONLINE
                tauntText += " eat fruit to " + to_string((int)d2);
#else
                tauntText += "ahhh";
#endif
                choice = d2;
            }
            else
            {
                // 寻找最近的不是死路且远离敌人的格子
                int minDis2 = INFINITY_DISTANCE;
                double maxScore = -INFINITY_DISTANCE;
                int r2, c2;
                for (int r = 0; r < height; ++r)
                    for (int c = 0; c < width; ++c)
                    {
                        if (isDeadRoad[r][c]) continue;
                        // 使用真实距离
                        if (DISTANCE(realDis, r, c, r1, c1) <=
                            RUN_AWAY_DISTANCE)
                            continue;
                        int nowDis2 = DISTANCE(a, r, c, me.row, me.col);
                        if (nowDis2 < minDis2 ||
                            (nowDis2 == minDis2 && fieldScore[r][c] > maxScore))
                        {
                            minDis2 = nowDis2;
                            maxScore = fieldScore[r][c];
                            r2 = r;
                            c2 = c;
                        }
                    }

                if (minDis2 < INFINITY_DISTANCE)
                {
// 找到上述格子
#ifndef _BOTZONE_ONLINE
                    tauntText += " to " + to_string(r2) + " " + to_string(c2);
#endif
                    choice = routineFloyd(gameField, me.row, me.col, r2, c2, a);
                }
                else
                {
#ifndef _BOTZONE_ONLINE
                    tauntText += " to nowhere!!!";
#endif
                    // TODO：被堵在死路里怎么办
                }
            }
        }
    }

    // 射必中则射，目前射最远的人
    // TODO：一次射多人
    // TODO：在敌人与自己连线上有果子时射
    // TODO：敌人从远处来吃自己时射
    if (choice == NO_CHOICE)
    {
        if (me.strength > gameField.SKILL_COST)
        {
#ifndef _BOTZONE_ONLINE
            tauntText = "shoot";
#endif
            int maxDis = -INFINITY_DISTANCE;
            int r1, c1;
            Direction d1;
            for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            {
                if (i == myID) continue;
                Player &p = gameField.players[i];
                if (p.dead) continue;
                int r = p.row;
                int c = p.col;
                Direction d = shootMustHit(gameField, me.row, me.col, r, c);
                if (d == stay) continue;
                int nowDis = DISTANCE(a, r, c, me.row, me.col);
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
#ifndef _BOTZONE_ONLINE
                tauntText += " to " + to_string(r1) + " " + to_string(c1);
#endif
                choice = d1;
            }
        }
    }

    // czllgzmzl跟我出现了5回合行动完全一样的情况？
    // 如果连续两回合跟别人站在同一格上， 第二回合往第一回合的位置射一发
    if (choice == NO_CHOICE)
    {
        tauntText = "shoot at same block";
        int sameBlockTurn = 0;
        bool sameBlock = false;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player &p = gameField.players[i];
            if (p.dead) continue;
            if (p.row == me.row && p.col == me.col)
            {
                sameBlock = true;
                break;
            }
        }
        if (sameBlock)
        {
            sameBlockTurn = js["a"].asInt();
            sameBlockTurn++;
            if (sameBlockTurn >= 2)
            {
                int r1 = js["p"][myID][(Json::Value::UInt)0].asInt();
                int c1 = js["p"][myID][(Json::Value::UInt)1].asInt();
                Direction d = shootDirection(gameField, me.row, me.col, r1, c1);
                if (d != stay) choice = d;
            }
        }
        js["a"] = sameBlockTurn;
    }

    // 吃能比力量比自己大的人先吃到的果子中的最近者
    if (choice == NO_CHOICE)
    {
#ifndef _BOTZONE_ONLINE
        tauntText = "eat fruit";
#endif
        int minDis = INFINITY_DISTANCE;
        double maxScore = -INFINITY_DISTANCE;
        int r1, c1;
        for (int fru = 0; fru < allFruitsCount; ++fru)
        {
            int r = allFruits[fru].row;
            int c = allFruits[fru].col;
            // 力量相等的玩家站在同一格上谁都吃不到果子
            if (r == me.row && c == me.col) continue;
            int dis = INFINITY_DISTANCE;
            int id;  // 离这个果子最近者的id
            for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            {
                Player &p = gameField.players[i];
                if (p.dead) continue;
                if (i != myID && allCmpStr[myID][i] >= 0) continue;
                if (DISTANCE(a, r, c, p.row, p.col) < dis)
                {
                    dis = DISTANCE(a, r, c, p.row, p.col);
                    id = i;
                }
            }
            if (id == myID && (dis < minDis ||
                               (dis == minDis && fieldScore[r][c] > maxScore)))
            {
                minDis = dis;
                maxScore = fieldScore[r][c];
                r1 = r;
                c1 = c;
            }
        }
        if (minDis < INFINITY_DISTANCE)
        {
#ifndef _BOTZONE_ONLINE
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
#endif
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 确保在生成果子的回合移动到生成果子的位置
    if (choice == NO_CHOICE)
    {
#ifndef _BOTZONE_ONLINE
        tauntText = "go to gen";
#endif
        int minDis = INFINITY_DISTANCE;
        double maxScore = -INFINITY_DISTANCE;
        int r1, c1;
        for (int fru = 0; fru < fruitGenPlacesCount; ++fru)
        {
            int r = fruitGenPlaces[fru].row;
            int c = fruitGenPlaces[fru].col;
            int nowDis = DISTANCE(a, r, c, me.row, me.col);
            if (nowDis < minDis ||
                (nowDis == minDis && fieldScore[r][c] > maxScore))
            {
                minDis = nowDis;
                maxScore = fieldScore[r][c];
                r1 = r;
                c1 = c;
            }
        }

        if (minDis >= leftGenTurn)
        {
#ifndef _BOTZONE_ONLINE
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
#endif
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 吃附近比自己力量小的人中的最近者
    if (choice == NO_CHOICE)
    {
#ifndef _BOTZONE_ONLINE
        tauntText = "eat enemy";
#endif
        int minDis = INFINITY_DISTANCE;
        int maxStr = 0;
        int r1, c1;
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
        {
            if (i == myID) continue;
            Player &p = gameField.players[i];
            if (p.dead) continue;
            if (allCmpStr[myID][i] <= 0) continue;
            int r = p.row;
            int c = p.col;
            int nowDis = DISTANCE(a, r, c, me.row, me.col);
            // 距离相同则吃力量大者
            if (nowDis <= EAT_ENEMY_DISTANCE &&
                (nowDis < minDis || (nowDis == minDis && p.strength > maxStr)))
            {
                minDis = nowDis;
                r1 = r;
                c1 = c;
                maxStr = p.strength;
            }
        }
        if (minDis < INFINITY_DISTANCE)
        {
#ifndef _BOTZONE_ONLINE
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
#endif
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 移动到附近分数较大的格子
    if (choice == NO_CHOICE)
    {
#ifndef _BOTZONE_ONLINE
        tauntText = "go to high score";
#endif
        double tScore = js["s"].asDouble();
        int r1 = js["r"].asInt();
        int c1 = js["c"].asInt();
        // 如果有格子的分数超过tScore + CHANGE_TARGET_SCORE则修改目标
        double maxScore = tScore + CHANGE_TARGET_SCORE;
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                if (fieldScore[r][c] > maxScore)
                {
                    tScore = maxScore = fieldScore[r][c];
                    r1 = r;
                    c1 = c;
                }
#ifndef _BOTZONE_ONLINE
        tauntText += " to " + to_string(r1) + " " + to_string(c1);
#endif
        choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        js["s"] = tScore;
        js["r"] = r1;
        js["c"] = c1;
    }

    // 向js写入每个人的位置，死者为(-2, -2)
    for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
    {
        Player &p = gameField.players[i];
        if (p.dead)
        {
            js["p"][i][(Json::Value::UInt)0] = -2;
            js["p"][i][(Json::Value::UInt)1] = -2;
        }
        else
        {
            js["p"][i][(Json::Value::UInt)0] = p.row;
            js["p"][i][(Json::Value::UInt)1] = p.col;
        }
    }

    Json::FastWriter writer;
    data = writer.write(js);
    gameField.DebugPrint();
    gameField.WriteOutput(choice, tauntText, data, globalData);
    return 0;
}
