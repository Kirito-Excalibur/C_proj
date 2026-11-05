
#include <graphics.h>
#include <conio.h>

int main(){
    int gd = DETECT, gm;

    initgraph(&gd, &gm, "");

    ellipse(250, 250, 0, 360, 100, 70);
    ellipse(470, 250, 0, 360, 100, 70);

    ellipse(280, 350, 0, 360, 70, 50);
    ellipse(440, 350, 0, 360, 70, 50);

    ellipse(360, 300, 0, 360, 20, 100);
    circle(360, 190, 25);

    line(345, 170, 320, 140);
    line(375, 170, 400, 140);

    getch();
    closegraph();

    return 0;
}
