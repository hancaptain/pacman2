#ifndef UTIL_H
#define UTIL_H

#define INCLUDE_JSONCPP_CPP
#include "pacman.h"
using namespace Pacman;

#define FiELD(a,r,c) *((a)+(r)*width+(c))
#define DISTANCE(a,r1,c1,r2,c2)\
 *((a)+((r1)*width+(c1))*width*height+(r2)*width+(c2))

//array is width*height*width*height
//array[r1][c1][r2][c2] means the shortest distance from (r1,c1) to (r2,c2)
void floyd(GameField& gameField, int* array);

//return the first move from (r1,c1) to (r2,c2)
//return stay when an error occurs
Direction dijkstra(GameField& gameField, int r1, int c1, int r2, int c2);

#endif