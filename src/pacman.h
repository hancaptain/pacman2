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

using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;
using std::setw;

namespace Pacman
{
    static const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1},
                     dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

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
    static const GridContentType playerID2Mask[] = {player1, player2, player3,
                                                    player4};
    static const string playerID2str[] = {"0", "1", "2", "3"};

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
    static const GridStaticType direction2OpposingWall[] = {
        wallNorth, wallEast, wallSouth, wallWest};

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
    };

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
       public:
        // 记录每回合的变化（栈）
        TurnStateTransfer backtrack[MAX_TURN];
        // 这个对象是否已经创建
        static bool constructed;
        // 场地的长和宽
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT,
            SKILL_COST;

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
        bool PopState();

        // 判断指定玩家向指定方向移动/施放技能是不是合法的（没有撞墙且没有踩到豆子产生器、力量足够）
        inline bool ActionValid(int playerID, Direction &dir) const
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
        bool NextTurn();

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
                      string &obtainedGlobalData);

        // 根据 static 和 content 数组准备场地的初始状况
        void PrepareInitialField(const Json::Value &staticField,
                                 const Json::Value &contentField);

        // 完成决策，输出结果。
        // action表示本回合的移动方向，stay为不移动，
        // shoot开头的动作表示向指定方向施放技能
        // tauntText表示想要叫嚣的言语，
        // 可以是任意字符串，除了显示在屏幕上不会有任何作用，留空表示不叫嚣
        // data表示自己想存储供下一回合使用的数据，留空表示删除
        // globalData表示自己想存储供以后使用的数据（替换），
        // 这个数据可以跨对局使用，会一直绑定在这个Bot上，留空表示删除
        void WriteOutput(Direction action, string tauntText = "",
                         string data = "", string globalData = "") const;

        // 用于显示当前游戏状态，调试用。
        // 提交到平台后会被优化掉。
        void DebugPrint() const;

        Json::Value SerializeCurrentTurnChange();

        // 初始化游戏管理器
        GameField()
        {
            constructed = true;

            turnID = 0;
        }
        GameField(const GameField &b) : GameField()
        {
        }
    };
}

#endif
