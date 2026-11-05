#include<bits/stdc++.h>
#include<graphics.h>
#include<conio.h>

int main(){
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "");

    for(int i=1; i<=8; i++){
        rectangle(50, 50, 70, 70);
    }

    getch();
    closegraph();
    return 0;
}

