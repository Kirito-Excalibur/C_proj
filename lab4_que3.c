
#include <graphics.h>
#include <conio.h>

int main()
{
    int gd = DETECT, gm;

    initgraph(&gd, &gm, "");

    setcolor(WHITE);

    // Left upper wing
    setfillstyle(SOLID_FILL, RED);
    fillellipse(280, 220, 100, 70);

    // Right upper wing
    setfillstyle(SOLID_FILL, RED);
    fillellipse(440, 220, 100, 70);

    // Left lower wing
    setfillstyle(SOLID_FILL, LIGHTBLUE);
    fillellipse(290, 320, 80, 60);

    // Right lower wing
    setfillstyle(SOLID_FILL, LIGHTBLUE);
    fillellipse(430, 320, 80, 60);

    // Body
    setfillstyle(SOLID_FILL, YELLOW);
    fillellipse(360, 280, 20, 100);

    // Head
    setfillstyle(SOLID_FILL, WHITE);
    fillellipse(360, 175, 25, 25);

    // Antennae
    setcolor(WHITE);
    line(350, 160, 320, 120);
    line(370, 160, 400, 120);

    // Antennae ends
    circle(318, 118, 4);
    circle(402, 118, 4);

    // Wing details
    setcolor(WHITE);

    line(280, 220, 330, 270);
    line(440, 220, 390, 270);

    line(290, 320, 335, 290);
    line(430, 320, 385, 290);

    getch();
    closegraph();

    return 0;
}

/*


// pastebin.com/ep5EkPru


*/
