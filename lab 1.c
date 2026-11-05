#include<bits/stdc++.h>
#include<graphics.h>
#include<conio.h>

int main(){
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "");

    rectangle(190, 130, 430, 70);
    rectangle(190, 190, 430, 130);
    rectangle(190, 250, 430, 190);

    line(190, 250, 190, 450);
    line(100, 450, 280, 450);

    setfillstyle(SOLID_FILL, LIGHTRED);
    floodfill(200, 80, WHITE);

    setfillstyle(SOLID_FILL, WHITE);
    floodfill(200, 140, WHITE);

    setfillstyle(SOLID_FILL, GREEN);
    floodfill(200,200, WHITE);

    setcolor(BLUE);
    circle(310, 160, 30);

    for(int i=1; i<=24; i++){
        line(310, 160, 310 + 30*cos(15*i*(3.14159/180)), 160 + 30*sin(15*i*(3.14159/180)));
    }

    getch();
    closegraph();
    return 0;
}

