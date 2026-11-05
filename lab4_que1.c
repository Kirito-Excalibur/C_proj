
#include <graphics.h>
#include <stdio.h>
#include <conio.h>

void plot4(int xc, int yc, int x, int y){
    putpixel(xc + x, yc + y, WHITE);
    putpixel(xc - x, yc + y, WHITE);
    delay(20);
    putpixel(xc + x, yc - y, WHITE);
    putpixel(xc - x, yc - y, WHITE);
}

void midpointCircle(int xc, int yc, int r){
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y){
        plot4(xc, yc, x, y);
        plot4(xc, yc, y, x);
        x++;

        if (p<0) p = p + 2*x + 1;
        else{
            y--;
            p = p + 2*x + 1 - 2*y;
        }
    }
}

int main(){
    int gd = DETECT, gm;
    int xc, yc, r;

    initgraph(&gd, &gm, "");

    printf("Enter center (xc yc): ");
    scanf("%d %d", &xc, &yc);

    printf("Enter radius: ");
    scanf("%d", &r);

    midpointCircle(xc, yc, r);

    getch();
    closegraph();

    return 0;
}

/*


INPUT :
Enter Centre (xc yc) : 350 250
Enter radius : 100

350 250
100



*/
