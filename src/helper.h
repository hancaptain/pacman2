#include "pacman.h"

namespace Helpers
{
double actionScore[MAX_DIRECTION];

inline int RandBetween(int a, int b)
{
    if (a > b) swap(a, b);
    return rand() % (b - a) + a;
}

void RandomInit()
{
    memset(&actionScore, 0, sizeof(actionScore));
}

void RandomPlay(Pacman::GameField &gameField, int myID)
{
    int count = 0, myAct = -1;
    while (true)
    {
        // 对每个玩家生成随机的合法动作
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        {
            if (gameField.players[i].dead) continue;
            Pacman::Direction valid[MAX_DIRECTION];
            int vCount = 0;
            for (Pacman::Direction d = Pacman::stay; d < Pacman::shootLeft; ++d)
                if (gameField.ActionValid(i, d)) valid[vCount++] = d;
            gameField.actions[i] = valid[RandBetween(0, vCount)];
        }

        if (count == 0) myAct = gameField.actions[myID];

        // 演算一步局面变化
        // NextTurn返回true表示游戏没有结束
        bool hasNext = gameField.NextTurn();
        count++;

        if (!hasNext) break;
    }

    // 计算分数

    int total = 0;
    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
        total += gameField.players[_].strength;

    if (total != 0)
        actionScore[myAct + 1] +=
            1.0 * gameField.players[myID].strength / total;

    // 恢复游戏状态到最初（就是本回合）
    while (count-- > 0) gameField.PopState();
}
}
