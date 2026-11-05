
#include <graphics.h>
#include <conio.h>

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "");

    // Basket handle
    setcolor(WHITE);
    arc(270, 220, 0, 180, 20);
    setfillstyle(SOLID_FILL, RED);
    floodfill(280, 215, WHITE);


    arc(220, 220, 19, 180, 40);
    arc(350, 220, 0, 180, 50);

    // Basket body
    int basket[] = {
        170,220,
        430,220,
        390,330,
        210,330,
        170,220
    };

    // Fill basket
    setfillstyle(SOLID_FILL, BROWN);
    fillpoly(5, basket);

    // Basket outline
    setcolor(WHITE);
    drawpoly(5, basket);

    getch();
    closegraph();
    return 0;
}

/*


pastebin.com/JVwT4FXJ
pastebin.com/en7TuAHp

*/



