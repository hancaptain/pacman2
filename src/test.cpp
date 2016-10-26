#include "util.h"

int main() {
	GameField gameField;
	string data, globaldata;//dummy
	gameField.ReadInput("input.txt", data, globaldata);
	int height = gameField.height; //r
	int width = gameField.width; //c
	int *a = (int *)malloc(sizeof(int)*height*height*width*width);
	floyd(gameField, a);
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++)
			cout << DISTANCE(a, 2, 3, r, c) << '\t';
		cout << endl;
	}
	cout << dijkstra(gameField, 5, 3, 0, 0) << endl;
	cout << dijkstra(gameField, 5, 3, 10, 0) << endl;
}