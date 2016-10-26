#define INCLUDE_CPP
#include "pacman.h"
using namespace Pacman;

#define FiELD(a,r,c) *((a)+(r)*width+(c))
#define DISTANCE(a,r1,c1,r2,c2)\
 *((a)+((r1)*width+(c1))*width*height+(r2)*width+(c2))

void floyd(GameField& gameField, int* array){
    int height=gameField.height; //r
    int width=gameField.width; //c
    int r1,c1,r2,c2,rt,ct,r,c;
    memset(array,0x33,sizeof(int)*height*height*width*width);
    for(r=0;r<height;r++) for(c=0;c<width;c++){
        GridStaticType& type=gameField.fieldStatic[r][c];
        DISTANCE(array,r,c,r,c)=0;
        for(int i=0;i<4;i++)
            if(!(type&direction2OpposingWall[i]))
                DISTANCE(array,r,c,(r+dy[i]+height)%height,(c+dx[i]+width)%width)=1;
    }

    for(rt=0;rt<height;rt++) for(ct=0;ct<width;ct++)
        for(r1=0;r1<height;r1++) for(c1=0;c1<width;c1++)
            for(r2=0;r2<height;r2++) for(c2=0;c2<width;c2++){
                int temp=DISTANCE(array,rt,ct,r1,c1)+DISTANCE(array,rt,ct,r2,c2);
                if(temp<DISTANCE(array,r1,c1,r2,c2)) DISTANCE(array,r1,c1,r2,c2)=temp;
            }   
}

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
			cout << DISTANCE(a, 2, 3, r, c)<<'\t';
		cout << endl;
	}
		
}