
#include <graphics.h>
#include <conio.h>

int main()
{
    int gd = DETECT, gm;
    int i, j, k, cx, cy, scale;
    int sx[8], sy[8];

    float vertex[8][4] = {
        {-1, -1, -1, 1},
        { 1, -1, -1, 1},
        { 1,  1, -1, 1},
        {-1,  1, -1, 1},
        {-1, -1,  1, 1},
        { 1, -1,  1, 1},
        { 1,  1,  1, 1},
        {-1,  1,  1, 1}
    };

    float P[4][4] = {
        {1, 0, 0.5, 0},
        {0, 1, 0.5, 0},
        {0, 0, 0,   0},
        {0, 0, 0,   1}
    };

    float projected[8][4];

    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    initgraph(&gd, &gm, "C:\\TURBOC3\\BGI");

    cx = getmaxx() / 2;
    cy = getmaxy() / 2;
    scale = (getmaxy() + 1) / 5;

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            projected[i][j] = 0;

            for (k = 0; k < 4; k++)
                projected[i][j] +=
                    P[j][k] * vertex[i][k];
        }

        sx[i] = cx + (int)(scale * projected[i][0]);
        sy[i] = cy - (int)(scale * projected[i][1]);
    }

    setcolor(LIGHTCYAN);

    for (i = 0; i < 12; i++)
    {
        line(sx[edges[i][0]], sy[edges[i][0]],
             sx[edges[i][1]], sy[edges[i][1]]);

             delay(200);
    }

    setcolor(WHITE);
    outtextxy(20, 20, "Parallel Projection of a Cube");

    getch();
    closegraph();

    return 0;
}

