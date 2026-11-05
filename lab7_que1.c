#include <graphics.h>
#include <conio.h>
#include <math.h>

#define PI 3.14159265

void getPoint(int type, float u, float v,float *x, float *y, float *z){
    if (type == 0){
        *x = cos(u) * sin(v);
        *y = sin(u) * sin(v);
        *z = cos(v);
    }
    else if (type == 1){
        *x = cos(u);
        *y = sin(u);
        *z = v;
    }
    else if (type == 2){
        *x = ((1 - v) / 2) * cos(u);
        *y = ((1 - v) / 2) * sin(u);
        *z = v;
    }
    else{
        *x = u;
        *y = v;
        *z = 0.6 * (u * u - v * v);
    }
}

void project(float x, float y, float z,int cx, int cy, int scale,int *sx, int *sy){
    *sx = cx + (int)(scale * 0.65 * (x - y));
    *sy = cy - (int)(scale *(0.35 * (x + y) + 0.8 * z));
}

void drawSurface(int type, int cx, int cy,int scale, int color){
    int i, j, sx, sy, ox, oy;
    float u, v, x, y, z;
    float u0, u1, v0, v1;

    u0 = (type == 3) ? -1 : 0;
    u1 = (type == 3) ? 1 : 2 * PI;
    v0 = (type == 0) ? 0 : -1;
    v1 = (type == 0) ? PI : 1;

    setcolor(color);

    for (i = 0; i <= 18; i++){
        u = u0 + (u1 - u0) * i / 18.0;

        for (j = 0; j <= 12; j++){
            v = v0 + (v1 - v0) * j / 12.0;

            getPoint(type, u, v, &x, &y, &z);
            project(x, y, z, cx, cy, scale, &sx, &sy);

            if (j > 0){
                line(ox, oy, sx, sy);
                delay(10);
            }

            ox = sx;
            oy = sy;
        }
    }

    for (j = 0; j <= 12; j++){
        v = v0 + (v1 - v0) * j / 12.0;

        for (i = 0; i <= 18; i++){
            u = u0 + (u1 - u0) * i / 18.0;

            getPoint(type, u, v, &x, &y, &z);
            project(x, y, z, cx, cy, scale, &sx, &sy);

            if (i > 0){
                line(ox, oy, sx, sy);
                delay(10);
            }

            ox = sx;
            oy = sy;
        }
    }
}

int main(){
    int gd = DETECT, gm;
    int w, h, scale;
    int cx1, cx2, cy1, cy2;

    initgraph(&gd, &gm, "C:\\TURBOC3\\BGI");

    w = getmaxx() + 1;
    h = getmaxy() + 1;

    cx1 = w / 4;
    cx2 = 3 * w / 4;
    cy1 = h / 4;
    cy2 = 3 * h / 4;

    scale = (h / 9 < w / 12) ? h / 9 : w / 12;

    setcolor(WHITE);
    outtextxy(cx1 - 25, 15, "Sphere");
    outtextxy(cx2 - 30, 15, "Cylinder");
    outtextxy(cx1 - 25, h / 2 + 5, "Cone");
    outtextxy(cx2 - 25, h / 2 + 5, "Saddle");

    drawSurface(0, cx1, cy1, scale, LIGHTCYAN);
    drawSurface(1, cx2, cy1, scale, YELLOW);
    drawSurface(2, cx1, cy2, scale, LIGHTGREEN);
    drawSurface(3, cx2, cy2, scale, LIGHTMAGENTA);

    getch();
    closegraph();

    return 0;
}
