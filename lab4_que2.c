
#include <graphics.h>
#include <stdio.h>
#include <conio.h>

#define LEFT   1
#define RIGHT  2
#define BOTTOM 4
#define TOP    8

int xmin = 150, ymin = 100;
int xmax = 500, ymax = 350;

int getCode(float x, float y){
    int code = 0;

    if (x<xmin) code |= LEFT;
    else if (x>xmax) code |= RIGHT;

    if (y<ymin) code |= BOTTOM;
    else if (y>ymax) code |= TOP;

    return code;
}

void drawWindow(){
    rectangle(xmin, ymin, xmax, ymax);
}

void clipLine(float x1, float y1, float x2, float y2){
    int code1 = getCode(x1, y1);
    int code2 = getCode(x2, y2);
    int accept = 0;
    float x, y;

    while (1){
        if ((code1 == 0) && (code2 == 0)){
            accept = 1;
            break;
        }
        else if (code1 & code2) break;
        else{
            int codeOut;

            if (code1 != 0) codeOut = code1;
            else codeOut = code2;

            if (codeOut & TOP){
                x = x1 + (x2-x1) * (ymax-y1)/(y2-y1);
                y = ymax;
            }
            else if (codeOut & BOTTOM){
                x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
                y = ymin;
            }
            else if (codeOut & RIGHT){
                y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
                x = xmax;
            }
            else{
                y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
                x = xmin;
            }

            if (codeOut == code1){
                x1 = x;
                y1 = y;
                code1 = getCode(x1, y1);
            }
            else{
                x2 = x;
                y2 = y;
                code2 = getCode(x2, y2);
            }
        }
    }

    if (accept){
        printf("Line is visible / partially visible after clipping.\n");

        setcolor(GREEN);
        line((int)x1, (int)y1, (int)x2, (int)y2);
    }
    else printf("Line is completely outside the window.\n");
}

int main(){
    int gd = DETECT, gm;
    float x1, y1, x2, y2;

    initgraph(&gd, &gm, "");

    printf("Clipping Window: (%d,%d) to (%d,%d)\n", xmin, ymin, xmax, ymax);

    printf("Enter line coordinates x1 y1 x2 y2: ");
    scanf("%f %f %f %f", &x1, &y1, &x2, &y2);

    setcolor(WHITE);
    drawWindow();

    setcolor(RED);
    line((int)x1, (int)y1, (int)x2, (int)y2);

    clipLine(x1, y1, x2, y2);

    getch();
    closegraph();

    return 0;
}

/*


INPUT :
1. completely visible : 200 150 450 300
2. Partially visible : 100 200 400 300
3. Completely invisible : 50 50 100 80

200 150 450 300
100 200 400 300
50 50 100 80



*/
