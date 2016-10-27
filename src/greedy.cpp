#define INCLUDE_JSONCPP_CPP


#include "pacman.h"
#include "util.h"

int main()
{
	Pacman::GameField gameField;
	gameField.DEBUG_STR = false;
	string data, globalData;  // 这是回合之间可以传递的信息

							  // 如果在本地调试，有input.txt则会读取文件内容作为输入
							  // 如果在平台上，则不会去检查有无input.txt
	int myID = gameField.ReadInput("input.txt", data,
		globalData);  // 输入，并获得自己ID

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
	int* a = (int *)malloc(sizeof(int)*height*height*width*width);
	floyd(gameField, a);

	int mindis = 1000;
	int r1, c1;
	for(int r=0;r<height;r++)
		for(int c=0;c<width;c++)
			if (gameField.fieldContent[r][c] & (smallFruit | largeFruit)) {
				int dis = 1000;
				int id;
				for(int i=0;i<4;i++)
					if (!gameField.players[i].dead) 
						if (DISTANCE(a, r, c, gameField.players[i].row, gameField.players[i].col) < dis) {
							dis = DISTANCE(a, r, c, gameField.players[i].row, gameField.players[i].col);
							id = i;
						}
				if (id == myID&&dis < mindis) {
					mindis = dis;
					r1 = r; c1 = c;
				}
			}
	if (mindis < 1000) 
		choice = routine_floyd(gameField, gameField.players[myID].row, gameField.players[myID].col, r1, c1,a);
	else choice = stay;
	// 输出当前游戏局面状态以供本地调试。注意提交到平台上会自动优化掉，不必担心。
	gameField.DebugPrint();

	// 输出动作
	// 注意这里输出(maxD - 1)，
	// 是因为之前进行随机的时候数组下标（actionScore的下标）是0~8，
	// 而游戏要求的输出是-1~7
	// 如果不使用上述方式得出自己动作，请不要保留这个-1。
	gameField.WriteOutput(choice, "", data, globalData);

	return 0;
}