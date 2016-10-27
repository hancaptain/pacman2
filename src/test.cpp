#include "util.h"

int main() {
	GameField gameField;
	string data, globaldata;//dummy
	gameField.ReadInput("input.txt", data, globaldata);
	int height = gameField.height; //r
	int width = gameField.width; //c
	int *a = (int *)malloc(sizeof(int)*height*height*width*width);
	floyd(gameField, a);
	/*for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++)
			cout << DISTANCE(a, 2, 3, r, c) << '\t';
		cout << endl;
	}*/
	for (int r = 0; r < gameField.height; r++)
		for (int c = 0; c < gameField.width; c++)
			if (routine_floyd(gameField, r, c, 3, 3, a) != dijkstra(gameField, r, c, 3, 3)) cout << "error on (" << r << ", " << c << ")" << endl;
	cout << "test end" << endl;
}