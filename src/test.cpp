#include "utils.h"

#define TEST_ROW 6
#define TEST_COL 2

int main()
{
    GameField gameField;
    string data, globaldata;
    gameField.ReadInput("input.txt", data, globaldata);
    int height = gameField.height;
    int width = gameField.width;

    cout << "test floyd" << endl;
    int *a = (int *)malloc(sizeof(int) * height * height * width * width);
    floyd(gameField, a);
    for (int r = 0; r < height; r++)
    {
        for (int c = 0; c < width; c++)
            cout << DISTANCE(a, TEST_ROW, TEST_COL, r, c) << "\t";
        cout << endl;
    }
    cout << endl;

    // DEPRECATED
    // cout<<"test dijkstra"<<endl;
    // for (int r = 0; r < gameField.height; r++)
    // for (int c = 0; c < gameField.width; c++)
    // if (routine_floyd(gameField, r, c, TEST_ROW, TEST_COL, a) !=
    // dijkstra(gameField, r, c, TEST_ROW, TEST_COL))
    // cout << "error on (" << r << ", " << c << ")" << endl;

    cout << "test scanAllDeadRoad" << endl;
    scanAllDeadRoad(gameField);
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << isDeadRoad[i][j];
        cout << endl;
    }
    cout << endl;
    for (int i = 0; i < gameField.height; ++i)
    {
        for (int j = 0; j < gameField.width; ++j) cout << wallCount[i][j];
        cout << endl;
    }
    cout << endl;

    cout << "test shootDirection" << endl;
    for (int r = 0; r < height; r++)
    {
        for (int c = 0; c < width; c++)
            cout << shootDirection(gameField, TEST_ROW, TEST_COL, r, c) << "\t";
        cout << endl;
    }
    cout << endl;

    cout << "test shootMustHit" << endl;
    for (int r = 0; r < height; r++)
    {
        for (int c = 0; c < width; c++)
            cout << shootMustHit(gameField, TEST_ROW, TEST_COL, r, c) << "\t";
        cout << endl;
    }
    cout << endl;

    cout << "test scanAllFruits" << endl;
    scanAllFruits(gameField);
    for (int i = 0; i < allFruitsCount; ++i)
        cout << allFruits[i].row << " " << allFruits[i].col << endl;
    cout << endl;

    cout << "test scanAllFruits" << endl;
    scanFruitGenPlaces(gameField);
    for (int i = 0; i < fruitGenPlacesCount; ++i)
        cout << fruitGenPlaces[i].row << " " << fruitGenPlaces[i].col << endl;
    cout << endl;

    cout << "test end" << endl;
}
