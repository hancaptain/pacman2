#include <deque>
#include "util.h"

void floyd(GameField& gameField, int* array){
    int height=gameField.height; //r
    int width=gameField.width; //c
    int r1,c1,r2,c2,rt,ct,r,c;
    memset(array,0x33,sizeof(int)*height*height*width*width); //set to infinity
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

Direction dijkstra(GameField& gameField, int r1, int c1, int r2, int c2) {
	int height = gameField.height;
	int width = gameField.width;
	if (gameField.fieldStatic[r1][c1] & generator || gameField.fieldStatic[r2][c2] & generator||(r1==c1&&r2==c2)) return stay;
	int predirection[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
	int dis[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
	memset(dis, 0x33, sizeof(dis));
	dis[r1][c1] = 0;
	deque<int> row = { r1 }, col = { c1 };
	while (row.size()) {
		int r = row.front(), c = col.front();
		row.pop_front(); col.pop_front();
		for (int d = 0; d < 4; d++) 
			if (!(gameField.fieldStatic[r][c] & direction2OpposingWall[d])) {
				int nr = (r + dy[d] + height) % height, nc = (c + dx[d] + width) % width;
				if (dis[nr][nc] > 1000) {
					dis[nr][nc] = dis[r][c] + 1;
					predirection[nr][nc] = d;
					row.push_back(nr); col.push_back(nc);
					if (nr == r2&&nc == c2) goto done;
				}
			}
	}
	return stay;
done:
	int r = r2, c = c2;
	while (dis[r][c] > 1) {
		int rr = (r - dy[predirection[r][c]] + height) % height;
		int cc = (c - dx[predirection[r][c]] + width) % width;
		r = rr; c = cc;
	}
	return (Direction)predirection[r][c];
}

Direction routine_floyd(GameField& gameField, int r1, int c1, int r2, int c2, int *array) {
	if (gameField.fieldStatic[r1][c1] & generator || gameField.fieldStatic[r2][c2] & generator || (r1 == r2&&c1 == c2)) return stay;
	int height = gameField.height; //r
	int width = gameField.width; //c
	int d;
	for (d = 0; d < 4; d++)
		if (!(gameField.fieldStatic[r1][c1] & direction2OpposingWall[d])) {
			int nr = (r1 + dy[d] + height) % height, nc = (c1 + dx[d] + width) % width;
			if (DISTANCE(array, r2, c2, r1, c1) == DISTANCE(array, r2, c2, nr, nc) + 1) break;
		}
	return Direction(d);
}