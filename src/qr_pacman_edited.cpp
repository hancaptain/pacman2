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
#include "helpers.h"
#include "pacman.h"

#define pb push_back
#define INF 10000000

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
    
    int Dfs(GameField &G, II now, II target, int step)
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
    
    int get_neighborhood(GameField &G, const II st)
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

    int Find_road(GameField &G, II now, II target)
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
    
    int calc_power(GameField &G, int id, int turn)
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
    
    int calc_shortest_near(GameField &G, int nowx, int nowy)
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
    
    int my_strategy(GameField &G, int myID)
    {
        height = G.height;
        width = G.width;
        
        // printf("%d %d\n", G.players[myID].row, G.players[myID].col);
        // printf("%d\n", G.turnID);
        
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

using namespace Exia;

int main()
{
    GameField gameField;
    string data, globalData;

    int myID = gameField.ReadInput("input.txt", data, globalData);

    if (gameField.players[myID].dead)
    {
        gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    srand(seed + myID);
    int dir = my_strategy(gameField, myID);

    gameField.DebugPrint();
    gameField.WriteOutput((Direction)(dir), "", data, globalData);
    return 0;
}
