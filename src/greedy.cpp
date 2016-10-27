#define INCLUDE_JSONCPP_CPP


#include "pacman.h"
#include "util.h"

int main()
{
	Pacman::GameField gameField;
	gameField.DEBUG_STR = false;
	string data, globalData;  // ���ǻغ�֮����Դ��ݵ���Ϣ

							  // ����ڱ��ص��ԣ���input.txt����ȡ�ļ�������Ϊ����
							  // �����ƽ̨�ϣ��򲻻�ȥ�������input.txt
	int myID = gameField.ReadInput("input.txt", data,
		globalData);  // ���룬������Լ�ID

					  // �Լ�����
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
	// �����ǰ��Ϸ����״̬�Թ����ص��ԡ�ע���ύ��ƽ̨�ϻ��Զ��Ż��������ص��ġ�
	gameField.DebugPrint();

	// �������
	// ע���������(maxD - 1)��
	// ����Ϊ֮ǰ���������ʱ�������±꣨actionScore���±꣩��0~8��
	// ����ϷҪ��������-1~7
	// �����ʹ��������ʽ�ó��Լ��������벻Ҫ�������-1��
	gameField.WriteOutput(choice, "", data, globalData);

	return 0;
}