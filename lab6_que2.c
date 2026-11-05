#include <graphics.h>
#include <conio.h>

void fullMoon(int x, int y){
    setfillstyle(SOLID_FILL, WHITE);
    fillellipse(x, y, 35, 35);
}

void crescent(int x, int y, int side){
    setcolor(WHITE);
    setfillstyle(SOLID_FILL, WHITE);
    fillellipse(x, y, 35, 35);

    setcolor(BLACK);
    setfillstyle(SOLID_FILL, BLACK);

    if (side == 0)
        fillellipse(x + 18, y, 35, 35);
    else
        fillellipse(x - 18, y, 35, 35);
}

void halfMoon(int x, int y, int side){
    setfillstyle(SOLID_FILL, WHITE);
    fillellipse(x, y, 35, 35);

    setfillstyle(SOLID_FILL, BLACK);

    if (side == 0)
        bar(x, y - 35, x + 35, y + 35);
    else
        bar(x - 35, y - 35, x, y + 35);
}

void gibbous(int x, int y, int side){
    setcolor(WHITE);
    setfillstyle(SOLID_FILL, WHITE);
    fillellipse(x, y, 35, 35);

    setcolor(BLACK);
    setfillstyle(SOLID_FILL, BLACK);

    if (side == 0)
        fillellipse(x + 25, y, 18, 35);
    else
        fillellipse(x - 25, y, 18, 35);
}

int main()
{
    int gd = DETECT, gm;

    initgraph(&gd, &gm, "");

    setfillstyle(SOLID_FILL, BLACK);
    fillellipse(70, 100, 35, 35);
    setcolor(WHITE);
    circle(70, 100, 35);

    crescent(190, 100, 0);

    halfMoon(345, 100, 0);

    gibbous(500, 100, 0);

    fullMoon(70, 300);

    gibbous(190, 300, 1);

    halfMoon(345, 300, 1);

    crescent(500, 300, 1);

    getch();
    closegraph();

    return 0;
}

/*


https://pastebin.com/ixuUbvLV


*/


