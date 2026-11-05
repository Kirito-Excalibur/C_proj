
#include <graphics.h>
#include <conio.h>
#include <math.h>

#define PI 3.14159265

int main(){
    int gd = DETECT, gm;
    int cx, cy, r, i, x, y;
    float angle, a;

    initgraph(&gd, &gm, "C:\\TURBOC3\\BGI");

    cx = getmaxx() / 2;
    cy = getmaxy() / 2;
    r = 100;
    angle = 0;

    while (!kbhit()){
        cleardevice();

        setcolor(WHITE);
        circle(cx, cy, r);

        setcolor(LIGHTCYAN);

        for (i = 0; i < 8; i++){
            a = (angle + i * 45) * PI / 180;

            x = cx + (int)(r * cos(a));
            y = cy + (int)(r * sin(a));

            line(cx, cy, x, y);
        }

        setcolor(YELLOW);
        circle(cx, cy, 12);

        setcolor(WHITE);
        outtextxy(20, 20,"Wheel Rotation - Press any key to stop");

        angle += 5;

        if (angle >= 360) angle -= 360;

        delay(50);
    }

    getch();
    closegraph();

    return 0;
}
