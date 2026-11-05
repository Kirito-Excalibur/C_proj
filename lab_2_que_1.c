#include <graphics.h>
#include <conio.h>

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "");

    int kite[] = {
        200,100,
        280,180,
        200,300,
        120,180,
        200,100
    };

    // Fill kite body
    setfillstyle(SOLID_FILL, YELLOW);
    fillpoly(5, kite);

    // curved strip
    setcolor(BLUE);
    arc(200, 300, 56, 124, 144);

    setcolor(RED);  // Red outline
    drawpoly(5, kite);

    line(200, 100, 200, 300); // Center stick

    // Thread
    line(200, 156, 260, 350);

    int tail[] = {
        200,300,
        210,320,
        190,320,
        200,300
    };

    drawpoly(4, tail);

    // Fill tail body
    setfillstyle(SOLID_FILL, GREEN);
    fillpoly(4, tail);

    getch();
    closegraph();
    return 0;
}
