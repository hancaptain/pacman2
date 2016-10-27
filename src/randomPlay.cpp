#include "helpers.h"
#include "pacman.h"

int main()
{
    Pacman::GameField gameField;
#ifdef DEFINE_DEBUG_STR
    gameField.DEBUG_STR = false;
#endif

    string data, globalData;  // 这是回合之间可以传递的信息

    // 输入，并获得自己ID
    // 如果在本地调试，有input.txt则会读取文件内容作为输入
    // 如果在平台上，则不会去检查有无input.txt
    int myID = gameField.ReadInput("input.txt", data, globalData);

    // 自己已死
    if (gameField.players[myID].dead)
    {
        gameField.WriteOutput((Pacman::Direction)(-1), "DEAD", data,
                              globalData);
        return 0;
    }

    srand(Pacman::seed + myID + gameField.turnID);
    // 简单随机，看哪个动作随机赢得最多
    Helpers::RandomInit();
    for (int i = 0; i < 100; i++) Helpers::RandomPlay(gameField, myID);

    int maxD = -2;
    for (int i = 0; i < MAX_DIRECTION; i++)
    {
        if (maxD == -2 || Helpers::actionScore[i] > Helpers::actionScore[maxD])
            maxD = i;
        // cout << i << " " << Helpers::actionScore[i] << endl;
    }

    // 输出当前游戏局面状态以供本地调试。注意提交到平台上会自动优化掉，不必担心。
    gameField.DebugPrint();

    // 输出动作
    // 注意这里输出(maxD - 1)，
    // 是因为之前进行随机的时候数组下标（actionScore的下标）是0~8，
    // 而游戏要求的输出是-1~7
    // 如果不使用上述方式得出自己动作，请不要保留这个-1。
    gameField.WriteOutput((Pacman::Direction)(maxD - 1), "", data, globalData);

    return 0;
}
