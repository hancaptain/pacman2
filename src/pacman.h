// Pacman2 样例程序
// 作者：zhouhy
// 时间：2016/10/12 12:54
//
// 【命名惯例】
// r/R/y/Y：Row，行，纵坐标
// c/C/x/X：Column，列，横坐标
// 数组的下标都是[y][x]或[r][c]的顺序
// 玩家编号0123
//
// 【坐标系】
//   0 1 2 3 4 5 6 7 8
// 0 +----------------> x
// 1 |
// 2 |
// 3 |
// 4 |
// 5 |
// 6 |
// 7 |
// 8 |
//   v y
//
// 【提示】你可以使用
// #ifndef _BOTZONE_ONLINE
// 这样的预编译指令来区分在线评测和本地评测
//
// 【提示】一般的文本编辑器都会支持将代码块折叠起来
// 如果你觉得自带代码太过冗长，可以考虑将整个namespace折叠
//
// edit by wd
// 修改了DebugPrint使之输出到debug.txt

#ifndef PACMAN_H
#define PACMAN_H

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>
#include "jsoncpp/json.h"

#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4  // 每个象限1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100
#define MAX_DIRECTION 9

using namespace std;
// using std::string;
// using std::swap;
// using std::cin;
// using std::cout;
// using std::endl;
// using std::getline;
// using std::runtime_error;
// using std::setw;

// 平台提供的吃豆人相关逻辑处理程序
namespace Pacman
{
    const time_t seed = time(0);
    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1},
              dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    // 让枚举也可以用这些运算了（不加会编译错误）
    template <typename T>
    inline T operator|=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template <typename T>
    inline T operator|(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template <typename T>
    inline T operator&=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template <typename T>
    inline T operator&(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template <typename T>
    inline T operator-(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) - static_cast<int>(b));
    }
    template <typename T>
    inline T operator++(T &a)
    {
        return a = static_cast<T>(static_cast<int>(a) + 1);
    }
    template <typename T>
    inline T operator~(const T &a)
    {
        return static_cast<T>(~static_cast<int>(a));
    }

    // 枚举定义；使用枚举虽然会浪费空间（sizeof(GridContentType) == 4），
    // 但是计算机处理32位的数字效率更高

    // 每个格子可能变化的内容，会采用“或”逻辑进行组合
    enum GridContentType
    {
        empty = 0,                   // 其实不会用到
        player1 = 1,                 // 1号玩家
        player2 = 2,                 // 2号玩家
        player3 = 4,                 // 3号玩家
        player4 = 8,                 // 4号玩家
        playerMask = 1 | 2 | 4 | 8,  // 用于检查有没有玩家等
        smallFruit = 16,             // 小豆子
        largeFruit = 32              // 大豆子
    };

    // 用玩家ID换取格子上玩家的二进制位
    GridContentType playerID2Mask[] = {player1, player2, player3, player4};
    string playerID2str[] = {"0", "1", "2", "3"};

    // 每个格子固定的东西，会采用“或”逻辑进行组合
    enum GridStaticType
    {
        emptyWall = 0,  // 其实不会用到
        wallNorth = 1,  // 北墙（纵坐标减少的方向）
        wallEast = 2,   // 东墙（横坐标增加的方向）
        wallSouth = 4,  // 南墙（纵坐标增加的方向）
        wallWest = 8,   // 西墙（横坐标减少的方向）
        generator = 16  // 豆子产生器
    };

    // 用移动方向换取这个方向上阻挡着的墙的二进制位
    GridStaticType direction2OpposingWall[] = {wallNorth, wallEast, wallSouth,
                                               wallWest};

    // 方向，可以代入dx、dy数组，同时也可以作为玩家的动作
    enum Direction
    {
        stay = -1,
        up = 0,
        right = 1,
        down = 2,
        left = 3,
        shootUp = 4,     // 向上发射金光
        shootRight = 5,  // 向右发射金光
        shootDown = 6,   // 向下发射金光
        shootLeft = 7    // 向左发射金光
    };

    // 场地上带有坐标的物件
    struct FieldProp
    {
        int row, col;
    };

    // 场地上的玩家
    struct Player : FieldProp
    {
        int strength;
        int powerUpLeft;
        bool dead;
    };

    // 回合新产生的豆子的坐标
    struct NewFruits
    {
        FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
        int newFruitCount;
    } newFruits[MAX_TURN];
    int newFruitsCount = 0;

    // 状态转移记录结构
    struct TurnStateTransfer
    {
        enum StatusChange  // 可组合
        {
            none = 0,
            ateSmall = 1,
            ateLarge = 2,
            powerUpDrop = 4,
            die = 8,
            error = 16
        };

        // 玩家选定的动作
        Direction actions[MAX_PLAYER_COUNT];

        // 此回合该玩家的状态变化
        StatusChange change[MAX_PLAYER_COUNT];

        // 此回合该玩家的力量变化
        int strengthDelta[MAX_PLAYER_COUNT];
    };

    // 游戏主要逻辑处理类，包括输入输出、回合演算、状态转移，全局唯一
    class GameField
    {
       private:
        // 为了方便，大多数属性都不是private的

        // 记录每回合的变化（栈）
        TurnStateTransfer backtrack[MAX_TURN];

        // 这个对象是否已经创建
        static bool constructed;

       public:
#define DEFINE_DEBUG_STR
        // 设为true时DebugStr才有效
        bool DEBUG_STR;

        // 场地的长和宽
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT,
            SKILL_COST;
        int myID;

        // 场地格子固定的内容
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

        // 场地格子会变化的内容
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

        int generatorTurnLeft;  // 多少回合后产生豆子
        int aliveCount;         // 有多少玩家存活
        int smallFruitCount;
        int turnID;

        FieldProp generators[MAX_GENERATOR_COUNT];  // 有哪些豆子产生器
        Player players[MAX_PLAYER_COUNT];           // 有哪些玩家

        // 玩家选定的动作
        Direction actions[MAX_PLAYER_COUNT];

        // 恢复到上次场地状态。可以一路恢复到最开始。
        // 恢复失败（没有状态可恢复）返回false
        bool PopState()
        {
            if (turnID <= 0) return false;

            const TurnStateTransfer &bt = backtrack[--turnID];
            int i, _;

            // 倒着来恢复状态

            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                GridContentType &content = fieldContent[_p.row][_p.col];
                TurnStateTransfer::StatusChange change = bt.change[_];

                // 5. 大豆回合恢复
                if (change & TurnStateTransfer::powerUpDrop) _p.powerUpLeft++;

                // 4. 吐出豆子
                if (change & TurnStateTransfer::ateSmall)
                {
                    content |= smallFruit;
                    smallFruitCount++;
                }
                else if (change & TurnStateTransfer::ateLarge)
                {
                    content |= largeFruit;
                    _p.powerUpLeft -= LARGE_FRUIT_DURATION;
                }

                // 2. 魂兮归来
                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // 1. 移形换影
                if (!_p.dead && bt.actions[_] != stay &&
                    bt.actions[_] < shootUp)
                {
                    fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                    _p.row = (_p.row - dy[bt.actions[_]] + height) % height;
                    _p.col = (_p.col - dx[bt.actions[_]] + width) % width;
                    fieldContent[_p.row][_p.col] |= playerID2Mask[_];
                }

                // 0. 救赎不合法的灵魂
                if (change & TurnStateTransfer::error)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // *. 恢复力量
                _p.strength -= bt.strengthDelta[_];
            }

            // 3. 收回豆子
            if (generatorTurnLeft == GENERATOR_INTERVAL)
            {
                generatorTurnLeft = 1;
                NewFruits &fruits = newFruits[--newFruitsCount];
                for (i = 0; i < fruits.newFruitCount; i++)
                {
                    fieldContent[fruits.newFruits[i].row]
                                [fruits.newFruits[i].col] &= ~smallFruit;
                    smallFruitCount--;
                }
            }
            else
                generatorTurnLeft++;

            return true;
        }

        // 判断指定玩家向指定方向移动/施放技能是不是合法的（没有撞墙且没有踩到豆子产生器、力量足够）
        inline bool ActionValid(int playerID, Direction dir) const
        {
            if (dir == stay) return true;
            const Player &p = players[playerID];
            if (dir >= up && dir <= left &&
                !(fieldStatic[p.row][p.col] & direction2OpposingWall[dir]))
                return true;
            if (dir >= shootUp && dir <= shootLeft && p.strength > SKILL_COST)
                return true;
            return false;
        }

        // 在向actions写入玩家动作后，演算下一回合局面，并记录之前所有的场地状态，可供日后恢复。
        // 是终局的话就返回false
        bool NextTurn()
        {
            int _, i, j;

            TurnStateTransfer &bt = backtrack[turnID];
            memset(&bt, 0, sizeof(bt));

            // 0. 杀死不合法输入
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &p = players[_];
                if (!p.dead)
                {
                    Direction &action = actions[_];
                    if (action == stay) continue;

                    if (!ActionValid(_, action))
                    {
                        bt.strengthDelta[_] += -p.strength;
                        bt.change[_] = TurnStateTransfer::error;
                        fieldContent[p.row][p.col] &= ~playerID2Mask[_];
                        p.strength = 0;
                        p.dead = true;
                        aliveCount--;
                    }
                    else if (action < shootUp)
                    {
                        // 遇到比自己强♂壮的玩家是不能前进的
                        GridContentType target =
                            fieldContent[(p.row + dy[action] + height) % height]
                                        [(p.col + dx[action] + width) % width];
                        if (target & playerMask)
                            for (i = 0; i < MAX_PLAYER_COUNT; i++)
                                if (target & playerID2Mask[i] &&
                                    players[i].strength > p.strength)
                                    action = stay;
                    }
                }
            }

            // 1. 位置变化
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];

                bt.actions[_] = actions[_];

                if (_p.dead || actions[_] == stay || actions[_] >= shootUp)
                    continue;

                // 移动
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }

            // 2. 玩家互殴
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                // 判断是否有玩家在一起
                int player, containedCount = 0;
                int containedPlayers[MAX_PLAYER_COUNT];
                for (player = 0; player < MAX_PLAYER_COUNT; player++)
                    if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
                        containedPlayers[containedCount++] = player;

                if (containedCount > 1)
                {
                    // NAIVE
                    for (i = 0; i < containedCount; i++)
                        for (j = 0; j < containedCount - i - 1; j++)
                            if (players[containedPlayers[j]].strength <
                                players[containedPlayers[j + 1]].strength)
                                swap(containedPlayers[j],
                                     containedPlayers[j + 1]);

                    int begin;
                    for (begin = 1; begin < containedCount; begin++)
                        if (players[containedPlayers[begin - 1]].strength >
                            players[containedPlayers[begin]].strength)
                            break;

                    // 这些玩家将会被杀死
                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];

                        // 从格子上移走
                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength * 0.5;
                        bt.strengthDelta[id] += -drop;
                        bt.change[id] |= TurnStateTransfer::die;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }

                    // 分配给其他玩家
                    int inc = lootedStrength / begin;
                    for (i = 0; i < begin; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
                        bt.strengthDelta[id] += inc;
                        p.strength += inc;
                    }
                }
            }

            // 2.5 金光法器
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead || actions[_] < shootUp) continue;

                _p.strength -= SKILL_COST;
                bt.strengthDelta[_] -= SKILL_COST;

                int r = _p.row, c = _p.col, player;
                Direction dir = actions[_] - shootUp;

                // 向指定方向发射金光（扫描格子直到被挡）
                while (!(fieldStatic[r][c] & direction2OpposingWall[dir]))
                {
                    r = (r + dy[dir] + height) % height;
                    c = (c + dx[dir] + width) % width;

                    // 如果转了一圈回来……
                    if (r == _p.row && c == _p.col) break;

                    if (fieldContent[r][c] & playerMask)
                        for (player = 0; player < MAX_PLAYER_COUNT; player++)
                            if (fieldContent[r][c] & playerID2Mask[player])
                            {
                                players[player].strength -= SKILL_COST * 1.5;
                                bt.strengthDelta[player] -= SKILL_COST * 1.5;
                                _p.strength += SKILL_COST * 1.5;
                                bt.strengthDelta[_] += SKILL_COST * 1.5;
                            }
                }
            }

            // *. 检查一遍有无死亡玩家
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead || _p.strength > 0) continue;

                // 从格子上移走
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.dead = true;

                // 使其力量变为0
                bt.strengthDelta[_] += -_p.strength;
                bt.change[_] |= TurnStateTransfer::die;
                _p.strength = 0;
                aliveCount--;
            }

            // 3. 产生豆子
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (int d = 0; d < 8; ++d)
                    {
                        // 取余，穿过场地边界
                        int r = (generators[i].row + dy[d] + height) % height,
                            c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator ||
                            fieldContent[r][c] & (smallFruit | largeFruit))
                            continue;
                        fieldContent[r][c] |= smallFruit;
                        fruits.newFruits[fruits.newFruitCount].row = r;
                        fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
            }

            // 4. 吃掉豆子
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                GridContentType &content = fieldContent[_p.row][_p.col];

                // 只有在格子上只有自己的时候才能吃掉豆子
                if (content & playerMask & ~playerID2Mask[_]) continue;

                if (content & smallFruit)
                {
                    content &= ~smallFruit;
                    _p.strength++;
                    bt.strengthDelta[_]++;
                    smallFruitCount--;
                    bt.change[_] |= TurnStateTransfer::ateSmall;
                }
                else if (content & largeFruit)
                {
                    content &= ~largeFruit;
                    if (_p.powerUpLeft == 0)
                    {
                        _p.strength += LARGE_FRUIT_ENHANCEMENT;
                        bt.strengthDelta[_] += LARGE_FRUIT_ENHANCEMENT;
                    }
                    _p.powerUpLeft += LARGE_FRUIT_DURATION;
                    bt.change[_] |= TurnStateTransfer::ateLarge;
                }
            }

            // 5. 大豆回合减少
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                if (_p.powerUpLeft > 0)
                {
                    bt.change[_] |= TurnStateTransfer::powerUpDrop;
                    if (--_p.powerUpLeft == 0)
                    {
                        _p.strength -= LARGE_FRUIT_ENHANCEMENT;
                        bt.strengthDelta[_] += -LARGE_FRUIT_ENHANCEMENT;
                    }
                }
            }

            // *. 检查一遍有无死亡玩家
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead || _p.strength > 0) continue;

                // 从格子上移走
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.dead = true;

                // 使其力量变为0
                bt.strengthDelta[_] += -_p.strength;
                bt.change[_] |= TurnStateTransfer::die;
                _p.strength = 0;
                aliveCount--;
            }

            ++turnID;

            // 是否只剩一人？
            if (aliveCount <= 1)
            {
                for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                    {
                        bt.strengthDelta[_] += smallFruitCount;
                        players[_].strength += smallFruitCount;
                    }
                return false;
            }

            // 是否回合超限？
            if (turnID >= MAX_TURN) return false;

            return true;
        }

        // 读取并解析程序输入，本地调试或提交平台使用都可以。
        // 如果在本地调试，程序会先试着读取参数中指定的文件作为输入文件，
        // 失败后再选择等待用户直接输入。
        // 本地调试时可以接受多行以便操作，
        // Windows下可以用Ctrl-Z或一个【空行+回车】表示输入结束，
        // 但是在线评测只需接受单行即可。
        // localFileName可以为NULL
        // obtainedData会输出自己上回合存储供本回合使用的数据
        // obtainedGlobalData会输出自己的Bot上以前存储的数据
        // 返回值是自己的playerID
        int ReadInput(const char *localFileName, string &obtainedData,
                      string &obtainedGlobalData)
        {
            string str, chunk;
#ifdef _BOTZONE_ONLINE
            ios::sync_with_stdio(false);  //ω\\)
            getline(cin, str);
#else
            if (localFileName)
            {
                ifstream fin(localFileName);
                if (1)
                    while (getline(fin, chunk) && chunk != "") str += chunk;
                else
                    while (getline(cin, chunk) && chunk != "") str += chunk;
            }
            else
                while (getline(cin, chunk) && chunk != "") str += chunk;
#endif
            Json::Reader reader;
            Json::Value input;
            reader.parse(str, input);

            int len = input["requests"].size();

            // 读取场地静态状况
            Json::Value field = input["requests"][(Json::Value::UInt)0],
                        staticField = field["static"],  // 墙面和产生器
                contentField = field["content"];        // 豆子和玩家
            height = field["height"].asInt();
            width = field["width"].asInt();
            LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
            LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
            SKILL_COST = field["SKILL_COST"].asInt();
            generatorTurnLeft = GENERATOR_INTERVAL =
                field["GENERATOR_INTERVAL"].asInt();

            PrepareInitialField(staticField, contentField);

            // 根据历史恢复局面
            for (int i = 1; i < len; i++)
            {
                Json::Value req = input["requests"][i];
                for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                        actions[_] =
                            (Direction)req[playerID2str[_]]["action"].asInt();
                NextTurn();
            }

            obtainedData = input["data"].asString();
            obtainedGlobalData = input["globaldata"].asString();

            myID = field["id"].asInt();
            return myID;
        }

        // 根据 static 和 content 数组准备场地的初始状况
        void PrepareInitialField(const Json::Value &staticField,
                                 const Json::Value &contentField)
        {
            int r, c, gid = 0;
            turnID = 0;
            generatorCount = 0;
            aliveCount = 0;
            smallFruitCount = 0;
            generatorTurnLeft = GENERATOR_INTERVAL;
            for (r = 0; r < height; r++)
                for (c = 0; c < width; c++)
                {
                    GridContentType &content = fieldContent[r][c] =
                        (GridContentType)contentField[r][c].asInt();
                    GridStaticType &s = fieldStatic[r][c] =
                        (GridStaticType)staticField[r][c].asInt();
                    if (s & generator)
                    {
                        generators[gid].row = r;
                        generators[gid++].col = c;
                        generatorCount++;
                    }
                    if (content & smallFruit) smallFruitCount++;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (content & playerID2Mask[_])
                        {
                            Player &p = players[_];
                            p.col = c;
                            p.row = r;
                            p.powerUpLeft = 0;
                            p.strength = 1;
                            p.dead = false;
                            aliveCount++;
                        }
                }
        }

        // 完成决策，输出结果。
        // action表示本回合的移动方向，stay为不移动，
        // shoot开头的动作表示向指定方向施放技能
        // tauntText表示想要叫嚣的言语，
        // 可以是任意字符串，除了显示在屏幕上不会有任何作用，留空表示不叫嚣
        // data表示自己想存储供下一回合使用的数据，留空表示删除
        // globalData表示自己想存储供以后使用的数据（替换），
        // 这个数据可以跨对局使用，会一直绑定在这个Bot上，留空表示删除
        void WriteOutput(Direction action, string tauntText = "",
                         string data = "", string globalData = "") const
        {
            Json::Value ret;
            ret["response"]["action"] = action;
            ret["response"]["tauntText"] = tauntText;
            ret["data"] = data;
            ret["globaldata"] = globalData;
            ret["debug"] = (Json::Int)seed;

#ifdef _BOTZONE_ONLINE
            Json::FastWriter writer;
            cout << writer.write(ret) << endl;
#else
            // 本地也用FastWriter好了
            // Json::StyledWriter writer;
            Json::FastWriter writer;
            ofstream fout(".\\output.txt");
            fout << writer.write(ret);
#endif
        }

        // 用于显示当前游戏状态，调试用。
        // 提交到平台后会被优化掉。
        inline void DebugPrint() const
        {
#ifndef _BOTZONE_ONLINE
            ofstream fdebug(".\\debug.txt", fstream::app);
            fdebug
                << "回合号[" << turnID << "]存活人数[" << aliveCount
                << "]| 图例 产生器[G] 有玩家[0/1/2/3] 多个玩家[*] 大豆[o] 小豆[.]\n";
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                const Player &p = players[_];
                fdebug << "[玩家" << _ << "(" << p.row << ", " << p.col
                       << ")|力量" << p.strength << "|加成剩余回合"
                       << p.powerUpLeft << "|" << (p.dead ? "死亡" : "存活")
                       << "]\n";
            }
            fdebug << "  ";
            for (int c = 0; c < width; c++)
                fdebug << " " << setw(2) << c << " ";
            fdebug << "\n";
            for (int r = 0; r < height; r++)
            {
                fdebug << "  ";
                for (int c = 0; c < width; c++)
                {
                    fdebug << " ";
                    fdebug << ((fieldStatic[r][c] & wallNorth) ? "---" : "   ");
                }
                fdebug << "\n";
                fdebug << setw(2) << r;
                for (int c = 0; c < width; c++)
                {
                    fdebug << ((fieldStatic[r][c] & wallWest) ? "|" : " ")
                           << " ";
                    int hasPlayer = -1;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (fieldContent[r][c] & playerID2Mask[_])
                            if (hasPlayer == -1)
                                hasPlayer = _;
                            else
                                hasPlayer = 4;
                    if (hasPlayer == 4)
                        fdebug << "*";
                    else if (hasPlayer != -1)
                        fdebug << (char)('0' + hasPlayer);
                    else if (fieldStatic[r][c] & generator)
                        fdebug << "G";
                    else if (fieldContent[r][c] & playerMask)
                        fdebug << "*";
                    else if (fieldContent[r][c] & smallFruit)
                        fdebug << ".";
                    else if (fieldContent[r][c] & largeFruit)
                        fdebug << "o";
                    else
                        fdebug << " ";
                    fdebug << " ";
                }
                fdebug << ((fieldStatic[r][width - 1] & wallEast) ? '|' : ' ')
                       << "\n";
            }
            fdebug << "  ";
            for (int c = 0; c < width; c++)
            {
                fdebug << " ";
                fdebug << ((fieldStatic[height - 1][c] & wallSouth) ? "---"
                                                                    : "   ");
            }
            fdebug << "\n\n";
#endif
        }

        // 输出自定义的调试信息
        inline void DebugStr(const string &s) const
        {
#ifndef _BOTZONE_ONLINE
            if (DEBUG_STR)
            {
                cout << s << endl;
                cout << "turnID " << turnID << endl;
            }
#endif
        }

        Json::Value SerializeCurrentTurnChange()
        {
            Json::Value result;
            TurnStateTransfer &bt = backtrack[turnID - 1];
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                result["actions"][_] = bt.actions[_];
                result["strengthDelta"][_] = bt.strengthDelta[_];
                result["change"][_] = bt.change[_];
            }
            return result;
        }

        // 初始化游戏管理器
        GameField()
        {
            if (constructed) throw runtime_error("One GameField Only");
            constructed = true;
            DEBUG_STR = false;
        }

        GameField(const GameField &b) : GameField()
        {
        }
    };

    bool GameField::constructed = false;
}

#endif

using namespace Pacman;
