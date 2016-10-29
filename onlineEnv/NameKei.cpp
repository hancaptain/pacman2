// NameKei
// by wd cp csy
// 将名字改为日文以增加威慑力

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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>
#include "jsoncpp/json.h"

#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100
#define MAX_DIRECTION 9

using namespace std;

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
                << "回合号【" << turnID << "】存活人数【" << aliveCount
                << "】| 图例 产生器[G] 有玩家[0/1/2/3] 多个玩家[*] 大豆[o] 小豆[.]\n";
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
            if (constructed)
                throw runtime_error(
                    "请不要再创建 GameField 对象了，整个程序中只应该有一个对象");
            constructed = true;
            DEBUG_STR = false;
        }

        GameField(const GameField &b) : GameField()
        {
        }
    };

    bool GameField::constructed = false;
}

using namespace Pacman;

namespace Utils
{
///////////////////////////////////////////////////////////////////////////////
// module distance
// by cp

#define DISTANCE_NEAR_ENEMY 1
#define INFINITY_DISTANCE 1000

#define FIELD(a, r, c) *((a) + (r)*width + (c))
#define DISTANCE(a, r1, c1, r2, c2)                                        \
    *((a) +                                                                \
      ((r1)*gameField.width + (c1)) * gameField.width * gameField.height + \
      (r2)*gameField.width + (c2))

    // array is width * height * width * height
    // array[r1][c1][r2][c2] means the shortest distance from (r1, c1) to (r2,
    // c2)
    // add considerEnemy by wd
    void floyd(GameField &gameField, int *array, bool considerEnemy = false)
    {
        int height = gameField.height;  // r
        int width = gameField.width;    // c
        int r1, c1, r2, c2, rt, ct, r, c;
        memset(
            array, 0x33,
            sizeof(int) * height * height * width * width);  // set to infinity
        for (r = 0; r < height; r++)
            for (c = 0; c < width; c++)
            {
                GridStaticType &type = gameField.fieldStatic[r][c];
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
                Player &_p = gameField.players[i];
                if (_p.dead) continue;
                if (_p.strength <= gameField.players[gameField.myID].strength)
                    continue;
                int r = _p.row;
                int c = _p.col;
                GridStaticType &type = gameField.fieldStatic[r][c];
                DISTANCE(array, r, c, r, c) += DISTANCE_NEAR_ENEMY;
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
    Direction routineFloyd(GameField &gameField, int r1, int c1, int r2, int c2,
                           int *array)
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

    void scanDeadRoad(GameField &gameField, int row, int col, int level)
    {
        if (scannedDeadRoad[row][col]) return;
        scannedDeadRoad[row][col] = true;

        int nowWallCount = 0;  // 旁边的墙或死路
        for (int d = 0; d < 4; ++d)
        {
            if (gameField.fieldStatic[row][col] & direction2OpposingWall[d])
                ++nowWallCount;
            else
            {
                int newRow =
                    (row + dy[d] + gameField.height) % gameField.height;
                int newCol = (col + dx[d] + gameField.width) % gameField.width;
                scanDeadRoad(gameField, newRow, newCol, level + 1);
                if (isDeadRoad[newRow][newCol]) ++nowWallCount;
            }
        }
        if (nowWallCount >= 3) isDeadRoad[row][col] = true;
        wallCount[row][col] = nowWallCount;
    }

    void scanAllDeadRoad(GameField &gameField)
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
                scanDeadRoad(gameField, gameField.height - i,
                             gameField.height - j, 0);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // module shoot
    // by wd

    // 根据坐标判断射击的方向
    // 如果不能射到，输出stay
    // f是射击者，t是被射者
    // TODO：考虑一次射多人
    Direction shootDirection(GameField &gameField, int fr, int fc, int tr,
                             int tc)
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

    Direction shootMustHit(GameField &gameField, int fr, int fc, int tr, int tc)
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

    ///////////////////////////////////////////////////////////////////////////////
    // module all fruits
    // by wd

    FieldProp allFruits[MAX_GENERATOR_COUNT * 8];
    int allFruitsCount;

    void scanAllFruits(GameField &gameField)
    {
        allFruitsCount = 0;
        for (int r = 0; r < gameField.height; r++)
            for (int c = 0; c < gameField.width; c++)
                if (gameField.fieldContent[r][c] & (smallFruit | largeFruit))
                {
                    allFruits[allFruitsCount].row = r;
                    allFruits[allFruitsCount].col = c;
                    ++allFruitsCount;
                }
    }

    FieldProp fruitGenPlaces[MAX_GENERATOR_COUNT * 8];
    int fruitGenPlacesCount;

    void scanFruitGenPlaces(GameField &gameField)
    {
        fruitGenPlacesCount = 0;
        for (int i = 0; i < gameField.generatorCount; ++i)
            for (int d = 0; d < 8; ++d)
            {
                int r =
                    (gameField.generators[i].row + dy[d] + gameField.height) %
                    gameField.height;
                int c =
                    (gameField.generators[i].col + dx[d] + gameField.width) %
                    gameField.width;
                if (gameField.fieldStatic[r][c] & generator) continue;
                fruitGenPlaces[fruitGenPlacesCount].row = r;
                fruitGenPlaces[fruitGenPlacesCount].col = c;
                fruitGenPlacesCount++;
            }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // module compare strength
    // by wd

    // [自己][对方]
    int allCmpStr[MAX_PLAYER_COUNT][MAX_PLAYER_COUNT];

    // 对比两个人的力量
    // 考虑距离和大果子加成剩余时间，没有考虑途中的果子
    // 距离要使用真实距离
    // fp是自己，tp是对方
    // 自己力量比对方大则返回1，相等则返回0，小则返回-1
    int cmpStr(GameField &gameField, int *realDis, Player &fp, Player &tp)
    {
        int fstr = fp.strength;
        int tstr = tp.strength;
        // 最坏情况下两人相向而行，每人走一半距离
        int dis = DISTANCE(realDis, fp.row, fp.col, tp.row, tp.col) * 0.5;
        if (fp.powerUpLeft <= dis) fstr -= gameField.LARGE_FRUIT_ENHANCEMENT;
        if (tp.powerUpLeft <= dis) tstr -= gameField.LARGE_FRUIT_ENHANCEMENT;
        if (fstr > tstr) return 1;
        if (fstr == tstr) return 0;
        return -1;
    }

    // 全部扫描并缓存
    void scanAllCmpStr(GameField &gameField, int *realDis)
    {
        for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            for (int j = 0; j < MAX_PLAYER_COUNT; ++j)
                if (i == j)
                    allCmpStr[i][j] = 0;
                else
                    allCmpStr[i][j] =
                        cmpStr(gameField, realDis, gameField.players[i],
                               gameField.players[j]);
    }
}

using namespace Utils;

#define NO_CHOICE (Direction)(-2)
#define RUN_AWAY_DISTANCE 2
#define EAT_ENEMY_DISTANCE 2
#define CHANGE_TARGET_SCORE 1

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
    if (data == "") data = "{\"s\":-1000,\"r\":-1,\"c\":-1}";
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
    // 分数高的一般不是死路
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
                if (allCmpStr[myID][i] <= 0) continue;
                int dis = DISTANCE(a, r, c, p.row, p.col);
                fieldScore[r][c] += pow(2, -dis);
            }
        }

    Direction choice = NO_CHOICE;

    // 逃离附近比自己力量大的人
    if (choice == NO_CHOICE)
    {
        tauntText = "run away";
        int minDis = INFINITY_DISTANCE;
        int r1, c1;
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
            }
        }

        if (minDis < INFINITY_DISTANCE)
        {
            tauntText += " from " + to_string(r1) + " " + to_string(c1);
            // 寻找最近的不是死路且远离敌人的格子
            int minDis2 = INFINITY_DISTANCE;
            double maxScore = 0;
            int r2, c2;
            for (int r = 0; r < height; ++r)
                for (int c = 0; c < width; ++c)
                {
                    if (isDeadRoad[r][c]) continue;
                    // 使用真实距离
                    if (DISTANCE(realDis, r, c, r1, c1) <= RUN_AWAY_DISTANCE)
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
                tauntText += " to " + to_string(r2) + " " + to_string(c2);
                choice = routineFloyd(gameField, me.row, me.col, r2, c2, a);
            }
            else
            {
                tauntText += " to nowhere!!!";
                // TODO
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
            tauntText = "shoot";
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
                tauntText += " to " + to_string(r1) + " " + to_string(c1);
                choice = d1;
            }
        }
    }

    // 吃能比力量比自己大的人先吃到的果子中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat fruit";
        int minDis = INFINITY_DISTANCE;
        double maxScore = 0;
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
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 确保在生成果子的回合移动到生成果子的位置
    if (choice == NO_CHOICE)
    {
        tauntText = "go to gen";
        int minDis = INFINITY_DISTANCE;
        double maxScore = 0;
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
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 吃附近比自己力量小的人中的最近者
    if (choice == NO_CHOICE)
    {
        tauntText = "eat enemy";
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
            tauntText += " to " + to_string(r1) + " " + to_string(c1);
            choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        }
    }

    // 移动到附近分数较大的格子
    if (choice == NO_CHOICE)
    {
        tauntText = "go to high score";
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
        tauntText += " to " + to_string(r1) + " " + to_string(c1);
        choice = routineFloyd(gameField, me.row, me.col, r1, c1, a);
        js["s"] = tScore;
        js["r"] = r1;
        js["c"] = c1;
    }

    Json::FastWriter writer;
    data = writer.write(js);
    gameField.DebugPrint();
#ifdef _BOTZONE_ONLINE
    gameField.WriteOutput(choice, "", data, globalData);
#else
    gameField.WriteOutput(choice, tauntText, data, globalData);
#endif
    return 0;
}
