// by cp
// Go to the nearest of beans which the player is the nearest player to them.

#include "pacman.h"
#include "util.h"

int main()
{
    Pacman::GameField gameField;
#ifdef DEFINE_DEBUG_STR
    gameField.DEBUG_STR = false;
#endif

    string data, globalData;  // 这是回合之间可以传递的信息

    int myID = gameField.ReadInput("input.txt", data, globalData);

    // 自己已死
    if (gameField.players[myID].dead)
    {
        gameField.WriteOutput((Pacman::Direction)(-1), "DEAD", data,
                              globalData);
        return 0;
    }

    Direction choice;

    int height = gameField.height;
    int width = gameField.width;
    int* a = (int*)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, a);

    int mindis = 1000;
    int r1, c1;
    for (int r = 0; r < height; r++)
        for (int c = 0; c < width; c++)
            if (gameField.fieldContent[r][c] & (smallFruit | largeFruit))
            {
                int dis = 1000;
                int id;
                for (int i = 0; i < 4; i++)
                    if (!gameField.players[i].dead)
                        if (DISTANCE(a, r, c, gameField.players[i].row,
                                     gameField.players[i].col) < dis)
                        {
                            dis = DISTANCE(a, r, c, gameField.players[i].row,
                                           gameField.players[i].col);
                            id = i;
                        }
                if (id == myID && dis < mindis)
                {
                    mindis = dis;
                    r1 = r;
                    c1 = c;
                }
            }

    if (mindis < 1000)
        choice = routine_floyd(gameField, gameField.players[myID].row,
                               gameField.players[myID].col, r1, c1, a);
    else
        choice = stay;

    gameField.DebugPrint();

    gameField.WriteOutput(choice, "", data, globalData);

    return 0;
}