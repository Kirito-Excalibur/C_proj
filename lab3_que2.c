#include <stdio.h>
#include <graphics.h>
#include <math.h>

#define PI 3.1415

void drawFigure(int x[], int y[], int n){
    for (int i = 0; i < n; i++){
        line(x[i], y[i], x[(i + 1) % n], y[(i + 1) % n]); // connect consecutive vertices
    }
}

void translate(int x[], int y[], int n, int tx, int ty){
    for (int i = 0; i < n; i++){
        x[i] += tx;
        y[i] += ty;
    }
}

void scale(int x[], int y[], int n, float sx, float sy){
    int cx = 0, cy = 0;

    for (int i = 0; i < n; i++){
        cx += x[i];
        cy += y[i];
    }

    cx /= n;
    cy /= n;

    for (int i = 0; i < n; i++){
        x[i] = cx + (int)((x[i] - cx) * sx);
        y[i] = cy + (int)((y[i] - cy) * sy);
    }
}

void rotate(int x[], int y[], int n, float angle){

    int cx = 0, cy = 0;
    float rad, newX, newY;

    for (int i = 0; i < n; i++){
        cx += x[i];
        cy += y[i];
    }

    cx /= n;
    cy /= n;

    rad = angle * PI / 180.0;

    for (int i = 0; i < n; i++){
        newX = (x[i] - cx) * cos(rad) - (y[i] - cy) * sin(rad);
        newY = (x[i] - cx) * sin(rad) + (y[i] - cy) * cos(rad);

        x[i] = cx + (int)newX;
        y[i] = cy + (int)newY;
    }
}

int main()
{
    int gd = DETECT, gm;
    int n;
    int x[20], y[20];
    int tx, ty;
    float sx, sy, angle;

    initgraph(&gd, &gm, "");

    printf("Enter number of vertices: ");
    scanf("%d", &n);

    printf("Enter coordinates of the figure:\n");

    for (int i = 0; i < n; i++){
        printf("Vertex %d: ", i + 1);
        scanf("%d %d", &x[i], &y[i]);
    }

    cleardevice();

    setcolor(WHITE);
    drawFigure(x, y, n);

    printf("\nEnter translation values (tx ty): ");
    scanf("%d %d", &tx, &ty);

    translate(x, y, n, tx, ty);
    drawFigure(x, y, n);

    printf("Enter scaling factors (sx sy): ");
    scanf("%f %f", &sx, &sy);

    scale(x, y, n, sx, sy);

    drawFigure(x, y, n);

    printf("Enter rotation angle: ");
    scanf("%f", &angle);

    rotate(x, y, n, angle);

    drawFigure(x, y, n);

    getch();
    closegraph();

    return 0;
}



/*

3
100 100
200 100
150 50
100 50
2 2
45


pastebin.com/QVdhtjvf

*/

