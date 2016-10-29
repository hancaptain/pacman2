#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <stack>
#include <stdexcept>
#include <string>
#include "jsoncpp/json.h"

#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4  // 每个象限1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100
#define pb push_back

// 你也可以选用 using namespace std; 但是会污染命名空间
using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;

// 平台提供的吃豆人相关逻辑处理程序
const int INF = 10000000;
namespace Pacman
{
    const time_t seed = time(0);
    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1},
              dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    // 枚举定义；使用枚举虽然会浪费空间（sizeof(GridContentType) ==
    // 4），但是计算机处理32位的数字效率更高

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
    inline T operator++(T &a)
    {
        return a = static_cast<T>(static_cast<int>(a) + 1);
    }
    template <typename T>
    inline T operator~(const T &a)
    {
        return static_cast<T>(~static_cast<int>(a));
    }

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
        // 下面的这几个只是为了产生器程序方便，不会实际用到
        ur = 4,  // 右上
        dr = 5,  // 右下
        dl = 6,  // 左下
        ul = 7   // 左上
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
            powerUpCancel = 4,
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
        // 场地的长和宽
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;

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

                if (!_p.dead)
                {
                    // 5. 大豆回合恢复
                    if (_p.powerUpLeft ||
                        change & TurnStateTransfer::powerUpCancel)
                        _p.powerUpLeft++;

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
                }

                // 2. 魂兮归来
                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // 1. 移形换影
                if (!_p.dead && bt.actions[_] != stay)
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
                if (!_p.dead) _p.strength -= bt.strengthDelta[_];
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

        // 判断指定玩家向指定方向移动是不是合法的（没有撞墙）
        inline bool ActionValid(int playerID, Direction &dir) const
        {
            if (dir == stay) return true;
            const Player &p = players[playerID];
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]);
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
                    else
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
                if (_p.dead) continue;

                bt.actions[_] = actions[_];

                if (actions[_] == stay) continue;

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
                        int drop = p.strength / 2;
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

            // 3. 产生豆子
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (Direction d = up; d < 8; ++d)
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

                if (_p.powerUpLeft > 0 && --_p.powerUpLeft == 0)
                {
                    _p.strength -= LARGE_FRUIT_ENHANCEMENT;
                    bt.change[_] |= TurnStateTransfer::powerUpCancel;
                    bt.strengthDelta[_] += -LARGE_FRUIT_ENHANCEMENT;
                }
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
            if (turnID >= 100) return false;

            return true;
        }

        // 读取并解析程序输入，本地调试或提交平台使用都可以。
        // 如果在本地调试，程序会先试着读取参数中指定的文件作为输入文件，失败后再选择等待用户直接输入。
        // 本地调试时可以接受多行以便操作，Windows下可以用 Ctrl-Z
        // 或一个【空行+回车】表示输入结束，但是在线评测只需接受单行即可。
        // localFileName 可以为NULL
        // obtainedData 会输出自己上回合存储供本回合使用的数据
        // obtainedGlobalData 会输出自己的 Bot 上以前存储的数据
        // 返回值是自己的 playerID
        int ReadInput(const char *localFileName, string &obtainedData,
                      string &obtainedGlobalData)
        {
            string str, chunk;
#ifdef _BOTZONE_ONLINE
            std::ios::sync_with_stdio(false);  //ω\\)
            getline(cin, str);
#else
            if (localFileName)
            {
                std::ifstream fin(localFileName);
                if (fin)
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

            return field["id"].asInt();
        }

        // 根据 static 和 content 数组准备场地的初始状况
        void PrepareInitialField(const Json::Value &staticField,
                                 const Json::Value &contentField)
        {
            int r, c, gid = 0;
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
        // action 表示本回合的移动方向，stay 为不移动
        // tauntText
        // 表示想要叫嚣的言语，可以是任意字符串，除了显示在屏幕上不会有任何作用，留空表示不叫嚣
        // data 表示自己想存储供下一回合使用的数据，留空表示删除
        // globalData
        // 表示自己想存储供以后使用的数据（替换），这个数据可以跨对局使用，会一直绑定在这个
        // Bot 上，留空表示删除
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
            Json::FastWriter writer;  // 在线评测的话能用就行……
#else
            Json::StyledWriter writer;  // 本地调试这样好看 > <
#endif
            cout << writer.write(ret) << endl;
        }

        // 用于显示当前游戏状态，调试用。
        // 提交到平台后会被优化掉。
        inline void DebugPrint() const
        {
#ifndef _BOTZONE_ONLINE
            printf(
                "回合号【%d】存活人数【%d】| 图例 产生器[G] 有玩家[0/1/2/3] 多个玩家[*] 大豆[o] 小豆[.]\n",
                turnID, aliveCount);
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                const Player &p = players[_];
                printf("[玩家%d(%d, %d)|力量%d|加成剩余回合%d|%s]\n", _, p.row,
                       p.col, p.strength, p.powerUpLeft,
                       p.dead ? "死亡" : "存活");
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++) printf("  %d ", c);
            putchar('\n');
            for (int r = 0; r < height; r++)
            {
                putchar(' ');
                putchar(' ');
                for (int c = 0; c < width; c++)
                {
                    putchar(' ');
                    printf((fieldStatic[r][c] & wallNorth) ? "---" : "   ");
                }
                printf("\n%d ", r);
                for (int c = 0; c < width; c++)
                {
                    putchar((fieldStatic[r][c] & wallWest) ? '|' : ' ');
                    putchar(' ');
                    int hasPlayer = -1;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (fieldContent[r][c] & playerID2Mask[_])
                            if (hasPlayer == -1)
                                hasPlayer = _;
                            else
                                hasPlayer = 4;
                    if (hasPlayer == 4)
                        putchar('*');
                    else if (hasPlayer != -1)
                        putchar('0' + hasPlayer);
                    else if (fieldStatic[r][c] & generator)
                        putchar('G');
                    else if (fieldContent[r][c] & playerMask)
                        putchar('*');
                    else if (fieldContent[r][c] & smallFruit)
                        putchar('.');
                    else if (fieldContent[r][c] & largeFruit)
                        putchar('o');
                    else
                        putchar(' ');
                    putchar(' ');
                }
                putchar((fieldStatic[r][width - 1] & wallEast) ? '|' : ' ');
                putchar('\n');
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++)
            {
                putchar(' ');
                printf((fieldStatic[height - 1][c] & wallSouth) ? "---"
                                                                : "   ");
            }
            putchar('\n');
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

            turnID = 0;
        }

        GameField(const GameField &b) : GameField()
        {
        }
    };

    bool GameField::constructed = false;
}

namespace Exia
{
    typedef std::pair<int, int> II;
    std::list<II> L;
    bool searched[20][20];

    const int dx[10] = {0, 1, 0, -1, 1, 1, -1, -1};
    const int dy[10] = {-1, 0, 1, 0, -1, 1, 1, -1};
    int height, width;

    void check(II &nxt)
    {
        if (nxt.first < 0) nxt.first += height;
        if (nxt.first >= height) nxt.first -= height;
        if (nxt.second < 0) nxt.second += width;
        if (nxt.second >= width) nxt.second -= width;
    }
    int sumstep;
    int Dfs(Pacman::GameField &G, II now, II target, int step)
    {
        if (step > sumstep) return -1000;
        if (now == target)
        {
            sumstep = step;
            return -1;
        }

        searched[now.first][now.second] = true;

        int sum_dir = -1000;
        for (int dir = 3; dir >= 0; dir--)
        {
            if (G.fieldStatic[now.first][now.second] & (1 << dir))
                continue;  //撞墙
            II nxt;
            nxt.first = now.first + dy[dir];
            nxt.second = now.second + dx[dir];

            check(nxt);

            if (searched[nxt.first][nxt.second]) continue;

            int res = Dfs(G, nxt, target, step + 1);

            if (res == -1000) continue;

            sum_dir = dir;
        }

        searched[now.first][now.second] = false;
        return sum_dir;
    }
    int get_neighborhood(Pacman::GameField &G, const II st)
    {
        L.clear();
        L.push_back(st);

        memset(searched, false, sizeof(searched));

        searched[st.first][st.second] = true;

        int total = 1;

        while (!L.empty())
        {
            II now = L.front();
            L.pop_front();
            for (int dir = 3; dir >= 0; dir--)
            {
                II nxt;
                nxt.first = now.first + dy[dir];
                nxt.second = now.second + dx[dir];

                if (G.fieldStatic[now.first][now.second] & (1 << dir))
                    continue;  //撞墙

                check(nxt);

                if (searched[nxt.first][nxt.second]) continue;
                searched[nxt.first][nxt.second] = true;

                if ((G.fieldContent[nxt.first][nxt.second] & 16) ||
                    (G.fieldContent[nxt.first][nxt.second] & 32))
                {
                    total++;
                    L.push_back(nxt);
                }
            }
        }
        memset(searched, false, sizeof(searched));
        return total;
    }

    int max(int x, int y)
    {
        return x > y ? x : y;
    }

    int Find_road(Pacman::GameField &G, II now, II target)
    {
        if (now == target) return 0;

        searched[now.first][now.second] = true;

        int res = 1000000;
        for (int i = 3; i >= 0; i--)
        {
            II nxt;

            if (G.fieldStatic[now.first][now.second] & (1 << i))
                continue;  //撞墙

            nxt.first = now.first + dy[i];
            nxt.second = now.second + dx[i];

            check(nxt);

            if (searched[nxt.first][nxt.second]) continue;

            int sum = Find_road(G, nxt, target);
            if (sum == 1000000) continue;

            if (sum == -1)
            {
                res = -1;
                break;
            }
            if (res != 1000000)
            {
                res = -1;
                break;
            }
            res = 1 + sum;
        }

        searched[now.first][now.second] = false;
        return res;
    }
    int calc_power(Pacman::GameField &G, int id, int turn)
    {
        if (G.players[id].powerUpLeft != 0 && turn > G.players[id].powerUpLeft)
            return G.players[id].strength - G.LARGE_FRUIT_ENHANCEMENT;
        else
            return G.players[id].strength;
    }
    struct node
    {
        II cor;
        int dir;
        int dis;
    };
    int calc_shortest_near(Pacman::GameField &G, int nowx, int nowy)
    {
        int res = 100000;
        for (int i = 0; i < G.generatorCount; i++)
        {
            II now(G.generators[i].row, G.generators[i].col);

            for (int dir = 0; dir <= 7; dir++)
            {
                II nxt;

                nxt.first = now.first + dy[dir];
                nxt.second = now.second + dx[dir];
                if (!((0 <= nxt.first && nxt.first < height) ||
                      (0 <= nxt.second && nxt.second < width)))
                    continue;
                check(nxt);

                bool error = false;
                for (int j = 0; j < G.generatorCount; j++)
                {
                    if (nxt.first == G.generators[j].row &&
                        nxt.second == G.generators[j].col)
                        error = true;
                }
                if (error) continue;

                bool bad_point = false;
                for (int kkk = 0; kkk < 4; kkk++)
                {
                    if ((G.fieldStatic[nxt.first][nxt.second] | (1 << kkk)) ==
                        15)
                        bad_point = true;
                }
                if (bad_point) continue;

                sumstep = 100000;
                Dfs(G, II(nowx, nowy), nxt, 0);
                if (sumstep < res) res = sumstep;
            }
        }
        return res;
    }
    bool cmp(node A, node B)
    {
        return A.dis < B.dis;
    }
    int my_strategy(Pacman::GameField &G, int myID)
    {
        height = G.height;
        width = G.width;
        //
        // printf("%d %d\n", G.players[myID].row, G.players[myID].col);
        // printf("%d\n", G.turnID);
        //
        bool warning[5], must_move[5];
        bool MM = false;
        memset(warning, false, sizeof(warning));
        memset(must_move, false, sizeof(must_move));
        for (int i = 3; i >= 0; i--)
        {
            II now(G.players[myID].row, G.players[myID].col);

            if (G.fieldStatic[now.first][now.second] & (1 << i))
                continue;  //撞墙

            II nxt;
            nxt.first = now.first + dy[i];
            nxt.second = now.second + dx[i];
            check(nxt);

            bool meet[5];
            memset(meet, false, sizeof(meet));
            for (int k = 0; k < MAX_PLAYER_COUNT; k++)
            {
                if (k == myID) continue;
                if (G.players[k].dead) continue;

                if (nxt.first == G.players[k].row &&
                    nxt.second == G.players[k].col)
                {
                    meet[k] = true;

                    if (calc_power(G, k, 1) > calc_power(G, myID, 1))
                    {
                        MM = true;
                        must_move[i] = true;
                    }
                }
                for (int dir = 3; dir >= 0; dir--)
                {
                    II enemy_now(G.players[k].row, G.players[k].col);

                    if (G.fieldStatic[enemy_now.first][enemy_now.second] &
                        (1 << dir))
                        continue;  //撞墙
                    II enemy_nxt;
                    enemy_nxt.first = enemy_now.first + dy[dir];
                    enemy_nxt.second = enemy_now.second + dx[dir];
                    check(enemy_nxt);

                    if (nxt.first == enemy_nxt.first &&
                        nxt.second == enemy_nxt.second)
                        meet[k] = true;
                }
            }

            for (int k = 0; k < MAX_PLAYER_COUNT; k++)
            {
                if (k == myID) continue;
                if (meet[k] == false) continue;

                if (calc_power(G, myID, 1) < calc_power(G, k, 1))
                    warning[i] = true;
            }
        }

        int eat_enemy = -1;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (i == myID) continue;
            if (G.players[i].dead) continue;
            memset(searched, false, sizeof(searched));
            int sum = Find_road(G, II(G.players[myID].row, G.players[myID].col),
                                II(G.players[i].row, G.players[i].col));

            if (sum == 1000000 || sum == -1) continue;
            if (calc_power(G, myID, sum) > calc_power(G, i, sum))
            {
                sumstep = 100000;
                int maybe = Dfs(G, II(G.players[myID].row, G.players[myID].col),
                                II(G.players[i].row, G.players[i].col), 0);

                if (warning[maybe]) continue;

                if (eat_enemy == -1)
                    eat_enemy = i;
                else if (calc_power(G, i, 0) > calc_power(G, eat_enemy, 0))
                    eat_enemy = i;
            }
        }

        if (eat_enemy != -1)
        {
            sumstep = 100000;
            return Dfs(G, II(G.players[myID].row, G.players[myID].col),
                       II(G.players[eat_enemy].row, G.players[eat_enemy].col),
                       0);
        }

        int shortest[20][20], id[20][20];
        for (int i = 0; i <= 15; i++)
            for (int j = 0; j <= 15; j++) shortest[i][j] = INF;

        memset(id, -1, sizeof(id));

        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (G.players[i].dead) continue;

            int now_shortest[20][20];

            for (int i = 0; i <= 15; i++)
                for (int j = 0; j <= 15; j++) now_shortest[i][j] = INF;

            L.clear();
            L.push_back(II(G.players[i].row, G.players[i].col));
            now_shortest[G.players[i].row][G.players[i].col] = 0;

            while (!L.empty())
            {
                II now = L.front();
                L.pop_front();
                for (int dir = 3; dir >= 0; dir--)
                {
                    if (G.fieldStatic[now.first][now.second] & (1 << dir))
                        continue;  //撞墙
                    II nxt;
                    nxt.first = now.first + dy[dir];
                    nxt.second = now.second + dx[dir];

                    check(nxt);

                    if (now_shortest[nxt.first][nxt.second] >
                        now_shortest[now.first][now.second] + 1)
                    {
                        now_shortest[nxt.first][nxt.second] =
                            now_shortest[now.first][now.second] + 1;
                        L.push_back(nxt);
                    }
                }
            }

            for (int j = height - 1; j >= 0; j--)
            {
                for (int k = width - 1; k >= 0; k--)
                {
                    if (now_shortest[j][k] < shortest[j][k] ||
                        (id[j][k] != -1 &&
                         now_shortest[j][k] == shortest[j][k] &&
                         calc_power(G, i, shortest[j][k]) >
                             calc_power(G, id[j][k], shortest[j][k])) ||
                        (id[j][k] != -1 &&
                         now_shortest[j][k] == shortest[j][k] &&
                         calc_power(G, i, shortest[j][k]) ==
                             calc_power(G, id[j][k], shortest[j][k]) &&
                         i == myID))
                    {
                        shortest[j][k] = now_shortest[j][k];
                        id[j][k] = i;
                    }
                }
            }
        }
        II canEat[1010];
        int NUM = 0;
        double value[1010];

        for (int i = height - 1; i >= 0; i--)
        {
            for (int j = width - 1; j >= 0; j--)
            {
                if (id[i][j] != myID) continue;

                if ((G.fieldContent[i][j] & 16) || (G.fieldContent[i][j] & 32))
                {
                    if (shortest[i][j] + calc_shortest_near(G, i, j) >
                        20 - G.turnID % 20)
                    {
                        if (!((G.fieldContent[i][j] & 32) &&
                              shortest[i][j] + G.turnID > 91 &&
                              shortest[i][j] + G.turnID <= 101))
                            continue;
                    }
                    NUM++;

                    canEat[NUM] = II(i, j);
                    value[NUM] = get_neighborhood(G, canEat[NUM]);
                    if (G.fieldContent[i][j] & 32)
                        value[NUM] = (value[NUM] - 1) +
                                     1 * (6 * 7 / (double)(height * width));

                    if ((G.fieldContent[i][j] & 32) &&
                        shortest[i][j] + G.turnID > 91 &&
                        shortest[i][j] + G.turnID <= 101)
                        value[NUM] += 100000;
                    value[NUM] /=
                        (log((double)(shortest[i][j] + 1.001)) / log(1.5));
                }
            }
        }
        int target = -1;
        for (int i = 1; i <= NUM; i++)
        {
            sumstep = 100000;
            if (G.players[myID].row == canEat[i].first &&
                G.players[myID].col == canEat[i].second)
                continue;
            if (warning[Dfs(G, II(G.players[myID].row, G.players[myID].col),
                            canEat[i], 0)])
                continue;

            if (target == -1)
                target = i;
            else if (value[i] > value[target])
                target = i;
        }

        if (target == -1)
        {
            II ed(-1, -1);
            int Last = 100000;
            for (int i = 0; i < G.generatorCount; i++)
            {
                II now(G.generators[i].row, G.generators[i].col);

                for (int dir = 0; dir <= 7; dir++)
                {
                    II nxt;

                    nxt.first = now.first + dy[dir];
                    nxt.second = now.second + dx[dir];
                    if (!((0 <= nxt.first && nxt.first < height) ||
                          (0 <= nxt.second && nxt.second < width)))
                        continue;
                    check(nxt);

                    bool error = false;
                    for (int j = 0; j < G.generatorCount; j++)
                    {
                        if (nxt.first == G.generators[j].row &&
                            nxt.second == G.generators[j].col)
                            error = true;
                    }
                    if (error) continue;

                    if (G.players[myID].row == nxt.first &&
                        G.players[myID].col == nxt.second && MM)
                        continue;

                    bool bad_point = false;
                    for (int kkk = 0; kkk < 4; kkk++)
                    {
                        if ((G.fieldStatic[nxt.first][nxt.second] |
                             (1 << kkk)) == 15)
                            bad_point = true;
                    }
                    if (bad_point) continue;

                    if (ed.first == -1 && ed.second == -1)
                    {
                        sumstep = 100000;
                        if (warning[Dfs(
                                G, II(G.players[myID].row, G.players[myID].col),
                                nxt, 0)])
                            continue;
                        Last = sumstep;
                        ed = nxt;
                    }
                    else
                    {
                        sumstep = 100000;
                        if (warning[Dfs(
                                G, II(G.players[myID].row, G.players[myID].col),
                                nxt, 0)])
                            continue;

                        sumstep = Last;
                        Dfs(G, II(G.players[myID].row, G.players[myID].col),
                            nxt, 0);
                        int New = sumstep;

                        if (New < Last)
                        {
                            Last = New;
                            ed = nxt;
                        }
                    }
                }
            }

            NUM = 0;
            node uu[20 * 20];
            for (int j = height - 1; j >= 0; j--)
            {
                for (int k = width - 1; k >= 0; k--)
                {
                    if ((G.fieldContent[j][k] & 16) ||
                        (G.fieldContent[j][k] & 32))
                    {
                        NUM++;
                        uu[NUM].cor = II(j, k);

                        sumstep = 100000;
                        uu[NUM].dir =
                            Dfs(G, II(G.players[myID].row, G.players[myID].col),
                                uu[NUM].cor, 0);
                        if (warning[uu[NUM].dir] ||
                            (MM && j == G.players[myID].row &&
                             G.players[myID].col == k))
                        {
                            NUM--;
                            continue;
                        }
                        uu[NUM].dis = sumstep;
                    }
                }
            }
            std::sort(uu + 1, uu + 1 + NUM, cmp);

            if ((Last + 1 >= 20 - G.turnID % 20) || NUM == 0)
            {
                if ((ed.first == -1 && ed.second == -1) ||
                    (G.players[myID].row == ed.first &&
                     G.players[myID].col == ed.second))
                {
                    int beiyong[5], num = 0;
                    for (int i = 3; i >= 0; i--)
                    {
                        if (must_move[i] == false &&
                            (G.fieldStatic[G.players[myID].row]
                                          [G.players[myID].col] &
                             (1 << i)) == 0)
                            beiyong[++num] = i;
                    }

                    if (num == 0 || MM == false)
                        return -1;
                    else
                        return beiyong[rand() % num + 1];
                }

                sumstep = 100000;
                return Dfs(G, II(G.players[myID].row, G.players[myID].col), ed,
                           0);
            }
            else
            {
                return uu[1].dir;
            }
        }

        memset(searched, false, sizeof(searched));

        // kkk = canEat[target].first) +  string(canEat[target].second);

        if (canEat[target].first == G.players[myID].row &&
            canEat[target].second == G.players[myID].col)
            return -1;

        sumstep = 100000;
        return Dfs(G, II(G.players[myID].row, G.players[myID].col),
                   canEat[target], 0);
    }
}

int main()
{
    // freopen("input.txt", "r", stdin);
    Pacman::GameField gameField;
    string data, globalData;  // 这是回合之间可以传递的信息

    // 如果在本地调试，有input.txt则会读取文件内容作为输入
    // 如果在平台上，则不会去检查有无input.txt
    int myID = gameField.ReadInput("input.txt", data,
                                   globalData);  // 输入，并获得自己ID
    srand(Pacman::seed + myID);

    // gameField.DebugPrint();
    int dir = Exia::my_strategy(gameField, myID);
    // printf("%d\n", dir);
    gameField.WriteOutput((Pacman::Direction)(dir), "", data, globalData);
    return 0;
}
