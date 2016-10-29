#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include "jsoncpp/json.h"

#define FIELD_MAX_HEIGHT 12
#define FIELD_MAX_WIDTH 12
#define MAX_GENERATOR_COUNT 4
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100

using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::pair;
using std::min;
using std::max;
using std::cerr;

namespace Pacman
{
    const double infi = 1e300;
    const time_t seed = time(0);
    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1},
              dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    enum GridContentType
    {
        empty = 0,
        player1 = 1,
        player2 = 2,
        player3 = 4,
        player4 = 8,
        playerMask = 1 | 2 | 4 | 8,
        smallFruit = 16,
        largeFruit = 32
    };

    GridContentType playerID2Mask[] = {player1, player2, player3, player4};
    string playerID2str[] = {"0", "1", "2", "3"};

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

    enum GridStaticType
    {
        emptyWall = 0,
        wallNorth = 1,
        wallEast = 2,
        wallSouth = 4,
        wallWest = 8,
        generator = 16
    };

    GridStaticType direction2OpposingWall[] = {wallNorth, wallEast, wallSouth,
                                               wallWest};

    enum Direction
    {
        stay = -1,
        up = 0,
        right = 1,
        down = 2,
        left = 3,
        ur = 4,
        dr = 5,
        dl = 6,
        ul = 7,
        nop = 8
    };

    struct FieldProp
    {
        int row, col;
    };

    struct Player : FieldProp
    {
        int strength;
        int powerUpLeft;
        bool dead;
    };

    struct NewFruits
    {
        FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
        int newFruitCount;
    } newFruits[MAX_TURN];
    int newFruitsCount = 0;

    struct TurnStateTransfer
    {
        enum StatusChange
        {
            none = 0,
            ateSmall = 1,
            ateLarge = 2,
            powerUpCancel = 4,
            die = 8,
            error = 16
        };

        Direction actions[MAX_PLAYER_COUNT];
        StatusChange change[MAX_PLAYER_COUNT];
        int strengthDelta[MAX_PLAYER_COUNT];
    };

    class GameField
    {
        TurnStateTransfer backtrack[MAX_TURN];

       public:
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        int generatorTurnLeft;
        int aliveCount;
        int smallFruitCount;
        int turnID;
        FieldProp generators[MAX_GENERATOR_COUNT];
        Player players[MAX_PLAYER_COUNT];
        Direction actions[MAX_PLAYER_COUNT];

        GameField() : turnID(0)
        {
        }

        bool PopState()
        {
            if (turnID <= 0) return false;

            const TurnStateTransfer &bt = backtrack[--turnID];
            int i, _;

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
            {
                Player &_p = players[_];
                GridContentType &content = fieldContent[_p.row][_p.col];
                TurnStateTransfer::StatusChange change = bt.change[_];

                if (!_p.dead)
                {
                    if (_p.powerUpLeft ||
                        change & TurnStateTransfer::powerUpCancel)
                    {
                        _p.powerUpLeft++;
                    }

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

                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                if (!_p.dead && bt.actions[_] != stay)
                {
                    fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                    _p.row = (_p.row - dy[bt.actions[_]] + height) % height;
                    _p.col = (_p.col - dx[bt.actions[_]] + width) % width;
                    fieldContent[_p.row][_p.col] |= playerID2Mask[_];
                }

                if (change & TurnStateTransfer::error)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                if (!_p.dead) _p.strength -= bt.strengthDelta[_];
            }

            if (generatorTurnLeft == GENERATOR_INTERVAL)
            {
                generatorTurnLeft = 1;
                NewFruits &fruits = newFruits[--newFruitsCount];
                for (i = 0; i < fruits.newFruitCount; ++i)
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

        inline bool ActionValid(const FieldProp &p, Direction &dir) const
        {
            if (dir == stay) return true;
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]) &&
                   !(s & generator);
        }

        inline bool ActionValid(int playerID, Direction &dir) const
        {
            return ActionValid(players[playerID], dir);
        }

        bool NextTurn()
        {
            int _, i, j;

            TurnStateTransfer &bt = backtrack[turnID];
            memset(&bt, 0, sizeof(bt));

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
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
                        GridContentType target =
                            fieldContent[(p.row + dy[action] + height) % height]
                                        [(p.col + dx[action] + width) % width];
                        if (target & playerMask)
                            for (i = 0; i < MAX_PLAYER_COUNT; ++i)
                            {
                                if (target & playerID2Mask[i] &&
                                    players[i].strength > p.strength)
                                {
                                    action = stay;
                                }
                            }
                    }
                }
            }

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
            {
                Player &_p = players[_];
                if (_p.dead) continue;
                bt.actions[_] = actions[_];
                if (actions[_] == stay) continue;

                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                int player, containedCount = 0;
                int containedPlayers[MAX_PLAYER_COUNT];
                for (player = 0; player < MAX_PLAYER_COUNT; player++)
                {
                    if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
                    {
                        containedPlayers[containedCount++] = player;
                    }
                }

                if (containedCount > 1)
                {
                    for (i = 0; i < containedCount; ++i)
                    {
                        for (j = 0; j < containedCount - i - 1; ++j)
                        {
                            if (players[containedPlayers[j]].strength <
                                players[containedPlayers[j + 1]].strength)
                            {
                                swap(containedPlayers[j],
                                     containedPlayers[j + 1]);
                            }
                        }
                    }

                    int begin;
                    for (begin = 1; begin < containedCount; begin++)
                    {
                        if (players[containedPlayers[begin - 1]].strength >
                            players[containedPlayers[begin]].strength)
                        {
                            break;
                        }
                    }

                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; ++i)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];

                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength / 2;
                        bt.strengthDelta[id] += -drop;
                        bt.change[id] |= TurnStateTransfer::die;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }

                    int inc = lootedStrength / begin;
                    for (i = 0; i < begin; ++i)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
                        bt.strengthDelta[id] += inc;
                        p.strength += inc;
                    }
                }
            }

            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; ++i)
                {
                    for (Direction d = up; d < 8; ++d)
                    {
                        int r = (generators[i].row + dy[d] + height) % height;
                        int c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator ||
                            fieldContent[r][c] & (smallFruit | largeFruit))
                        {
                            continue;
                        }
                        fieldContent[r][c] |= smallFruit;
                        fruits.newFruits[fruits.newFruitCount].row = r;
                        fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
                }
            }

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
            {
                Player &_p = players[_];
                if (_p.dead) continue;
                GridContentType &content = fieldContent[_p.row][_p.col];
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

            for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
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

            if (aliveCount <= 1)
            {
                for (_ = 0; _ < MAX_PLAYER_COUNT; ++_)
                {
                    if (!players[_].dead)
                    {
                        bt.strengthDelta[_] += smallFruitCount;
                        players[_].strength += smallFruitCount;
                    }
                }
                return false;
            }
            return turnID < 100;
        }

        int ReadInput(string &obtainedData, string &obtainedGlobalData)
        {
            string str;
            std::ios::sync_with_stdio(false);
            getline(cin, str);
            Json::Reader reader;
            Json::Value input;
            reader.parse(str, input);

            int len = input["requests"].size();

            Json::Value field = input["requests"][(Json::Value::UInt)0],
                        staticField = field["static"],
                        contentField = field["content"];
            height = field["height"].asInt();
            width = field["width"].asInt();
            LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
            LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
            generatorTurnLeft = GENERATOR_INTERVAL =
                field["GENERATOR_INTERVAL"].asInt();

            obtainedData = input["data"].asString();
            obtainedGlobalData = input["globaldata"].asString();

            PrepareInitialField(staticField, contentField);

            for (int i = 1; i < len; ++i)
            {
                Json::Value req = input["requests"][i];
                for (int _ = 0; _ < MAX_PLAYER_COUNT; ++_)
                {
                    if (!players[_].dead)
                    {
                        actions[_] =
                            (Direction)req[playerID2str[_]]["action"].asInt();
                    }
                }
                NextTurn();
            }

            return field["id"].asInt();
        }

        void PrepareInitialField(const Json::Value &staticField,
                                 const Json::Value &contentField)
        {
            int r, c, gid = 0;
            generatorCount = 0;
            aliveCount = 0;
            smallFruitCount = 0;
            generatorTurnLeft = GENERATOR_INTERVAL;
            for (r = 0; r < height; ++r)
            {
                for (c = 0; c < width; ++c)
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
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; ++_)
                    {
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
            }
        }

        void WriteOutput(Direction action, string tauntText = "",
                         string data = "", string globalData = "") const
        {
            Json::Value ret;
            ret["response"]["action"] = action;
            ret["response"]["tauntText"] = tauntText;
            ret["data"] = data;
            ret["globaldata"] = globalData;
            ret["debug"] = (Json::Int)seed;

            Json::FastWriter writer;

            cout << writer.write(ret) << endl;
        }

        int GetValids(int id, Direction *valids)
        {
            int ret = 0;
            for (Direction d = stay; d < 4; ++d)
            {
                if (ActionValid(id, d)) valids[ret++] = d;
            }
            return ret;
        }

        inline FieldProp Front(const FieldProp &u, const Direction &d)
        {
            if (d == -1) return u;
            FieldProp ret;
            ret.row = (u.row + dy[d] + height) % height;
            ret.col = (u.col + dx[d] + width) % width;
            return ret;
        }
    };

    namespace Util
    {
        inline int Random(int a, int b)
        {
            if (a > b) swap(a, b);
            return rand() % (b - a) + a;
        }
    }

    class Simulator
    {
        GameField &game;
        int id;

       public:
        Simulator(GameField &g, int m) : game(g), id(m)
        {
        }

        bool RandomMove(Direction action)
        {
            for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
            {
                if (game.players[i].dead) continue;
                if (id != i)
                {
                    Direction valids[5];
                    int vnt = game.GetValids(i, valids);
                    game.actions[i] = valids[Util::Random(0, vnt)];
                }
                else
                    game.actions[i] = action;
            }
            return game.NextTurn();
        }
    };

    class Navigator
    {
        GameField &game;

       public:
        int dis[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH][FIELD_MAX_HEIGHT]
               [FIELD_MAX_WIDTH];

        Navigator(GameField &g) : game(g)
        {
            for (int i = 0; i < game.height; ++i)
            {
                for (int j = 0; j < game.width; ++j) bfs(i, j);
            }
        }

        void bfs(int i, int j)
        {
            int(*dist)[FIELD_MAX_WIDTH] = dis[i][j];
            dist[i][j] = 1;
            FieldProp q[FIELD_MAX_HEIGHT * FIELD_MAX_WIDTH];
            int l = 0, r = 1;
            q[0].row = i;
            q[0].col = j;
            while (r > l)
            {
                FieldProp u = q[l++];
                for (Direction d = up; d < 4; ++d)
                {
                    if (game.fieldStatic[u.row][u.col] &
                        direction2OpposingWall[d])
                        continue;
                    FieldProp v = game.Front(u, d);
                    if ((game.fieldStatic[v.row][v.col] & generator) ||
                        dist[v.row][v.col])
                        continue;
                    dist[v.row][v.col] = dist[u.row][u.col] + 1;
                    q[r++] = v;
                }
            }
        }
    };

    namespace Greedy
    {
        const double dMix = 8, sMix = 0.9, lMix = 0.9, gMix = 0.9;

        const int depth = 4;
        const int repeat = 1000;

        class Judge
        {
           protected:
            GameField &game;
            Navigator &nav;
            int id;

           public:
            Judge(GameField &g, Navigator &n, int i) : game(g), nav(n), id(i)
            {
            }

            inline double rev(int x)
            {
                return x ? 1.0 / x : 0;
            }

            double FruitVal()
            {
                double ret = 0.0;
                Player &p = game.players[id];
                for (int i = 0; i < game.height; ++i)
                {
                    for (int j = 0; j < game.width; ++j)
                    {
                        if (game.fieldContent[i][j] & smallFruit)
                        {
                            ret += sMix * rev(nav.dis[p.row][p.col][i][j]);
                        }
                        else if (game.fieldContent[i][j] & largeFruit)
                        {
                            ret += lMix * rev(nav.dis[p.row][p.col][i][j]);
                        }
                    }
                }
                return ret;
            }

            double GeneratorVal()
            {
                double ret = 0.0;
                Player &p = game.players[id];
                for (int i = 0; i < game.generatorCount; ++i)
                {
                    FieldProp &g = game.generators[i];
                    for (Direction d = up; d < 8; ++d)
                    {
                        FieldProp v = game.Front(g, d);
                        int t = nav.dis[p.row][p.col][v.row][v.col];
                        ret += gMix * rev(t) * rev(t);
                    }
                }
                return ret / game.generatorTurnLeft;
            }

            double StrengthVal()
            {
                Player &p = game.players[id];
                double ret = p.strength;
                if (p.powerUpLeft > 0) ret -= dMix;
                return ret;
            }

            inline double Val()
            {
                return FruitVal() + GeneratorVal() + StrengthVal();
            }
        };

        class NaiveJudge : Judge
        {
           public:
            NaiveJudge(GameField &g, Navigator &n, int i) : Judge(g, n, i)
            {
            }

            double operator()(Direction d)
            {
                Simulator s(game, id);
                s.RandomMove(d);
                double ret = Val();
                game.PopState();
                return ret;
            }
        };

        class GreedyJudge : Judge
        {
            double ans;
            Direction dir;

           public:
            GreedyJudge(GameField &g, Navigator &n, int i) : Judge(g, n, i)
            {
            }

            void DFS(int i)
            {
                if (i >= 4)
                {
                    game.NextTurn();
                    ans = min(ans, Val());
                    game.PopState();
                    return;
                }
                else if (i == id)
                {
                    game.actions[i] = dir;
                    DFS(i + 1);
                }
                else
                {
                    for (Direction d = stay; d < 4; ++d)
                    {
                        if (!game.ActionValid(i, d)) continue;
                        game.actions[i] = d;
                        DFS(i + 1);
                    }
                }
            }

            double operator()(Direction d)
            {
                dir = d;
                ans = infi;
                DFS(0);
                return ans;
            }
        };

        class MontJudge : Judge
        {
            Direction actions[depth];
            double ans;

           public:
            MontJudge(GameField &g, Navigator &n, int i) : Judge(g, n, i)
            {
            }

            void DFS(int i, FieldProp x)
            {
                if (i == depth)
                {
                    double ret = infi;
                    for (int _ = 0; _ < repeat; ++_)
                    {
                        Simulator s(game, id);
                        int turns = depth;
                        for (int k = 0; k < turns; ++k)
                        {
                            if (!s.RandomMove(actions[k])) turns = k + 1;
                        }
                        double v = Val();
                        for (int k = 0; k < turns; ++k) game.PopState();
                        if (v <= ans) return;
                        ret = min(ret, v);
                    }
                    ans = ret;
                }
                else
                {
                    for (Direction d = stay; d < 4; ++d)
                    {
                        if (game.ActionValid(x, d))
                        {
                            actions[i] = d;
                            DFS(i + 1, game.Front(x, d));
                        }
                    }
                }
            }

            double operator()(Direction d)
            {
                actions[0] = d;
                ans = -infi;
                DFS(1, game.Front(game.players[id], d));
                return ans;
            }
        };
    }

    const Direction turnSearch[] = {up, right, down, left, stay};

    template <class Judge>
    void RunJudge()
    {
        GameField game;
        string data, globalData;
        int id = game.ReadInput(data, globalData);
        Navigator nav(game);
        srand(seed ^ id);

        Judge eval(game, nav, id);

        double ans = -infi;
        Direction best;
        for (int i = 0; i < 5; ++i)
        {
            Direction d = turnSearch[i];
            if (!game.ActionValid(id, d)) continue;
            double t = eval(d);
            if (t > ans)
            {
                ans = t;
                best = d;
            }
        }

        game.WriteOutput(best, "", data, globalData);
    }
}

using Pacman::RunJudge;
using Pacman::Greedy::GreedyJudge;
using Pacman::Greedy::MontJudge;

int main()
{
    RunJudge<MontJudge>();

    return 0;
}
