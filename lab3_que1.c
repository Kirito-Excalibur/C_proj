#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>

void midPointLine(int x1, int y1, int x2, int y2){

    int dx = x2 - x1;
    int dy = y2 - y1;

    int x = x1;
    int y = y1;

    putpixel(x, y, WHITE);

    int d = dy - (dx / 2);

    while (x < x2){
        x++;

        if (d < 0) d = d + dy;
        else{
            d = d + dy - dx;
            y++;
        }
        delay(20);
        putpixel(x, y, WHITE);
    }
}

int main()
{
    int gd = DETECT, gm;
    int x1, y1, x2, y2;

    initgraph(&gd, &gm, "");

    printf("Enter starting point -> x1 and y1: ");
    scanf("%d %d", &x1, &y1);

    printf("Enter ending point -> x2 and y2: ");
    scanf("%d %d", &x2, &y2);

    midPointLine(x1, y1, x2, y2);

    getch();
    closegraph();

    return 0;
}


/*

100 100
400 300

*/

