#include <graphics.h>
#include <conio.h>
#include <dos.h>

#define MAX 10

typedef struct{float x,y;}Point;

int xmin=200,ymin=150,xmax=500,ymax=350;

int in(Point p,int e){
    if(e==0)return p.x>=xmin;
    if(e==1)return p.x<=xmax;
    if(e==2)return p.y>=ymin;
    return p.y<=ymax;
}

Point inter(Point a,Point b,int e){
    Point p; float t;
    if(e<2){
        p.x=e?xmax:xmin;
        t=(p.x-a.x)/(b.x-a.x);
        p.y=a.y+t*(b.y-a.y);
    }else{
        p.y=e==2?ymin:ymax;
        t=(p.y-a.y)/(b.y-a.y);
        p.x=a.x+t*(b.x-a.x);
    }
    return p;
}

int clip(Point a[],int n,Point b[],int e){
    int i,k=0; Point s=a[n-1],p;
    for(i=0;i<n;i++){
        p=a[i];
        if(in(p,e)){
            if(!in(s,e))b[k++]=inter(s,p,e);
            b[k++]=p;
        }else if(in(s,e))b[k++]=inter(s,p,e);
        s=p;
    }
    return k;
}

void draw(Point p[],int n){
    int i,a[2*MAX];
    if(n<3)return;
    for(i=0;i<n;i++){
        a[2*i]=(int)p[i].x;
        a[2*i+1]=(int)p[i].y;
    }
    setcolor(WHITE);
    drawpoly(n,a);
    setfillstyle(SOLID_FILL,GREEN);
    fillpoly(n,a);
}

void win(){
    setcolor(WHITE);
    setlinestyle(SOLID_LINE,0,1);
    rectangle(xmin,ymin,xmax,ymax);
}

int main(){
    int gd=DETECT,gm,n=3,i;
    Point p[MAX],t[MAX];

    p[0].x=150;p[0].y=250;
    p[1].x=350;p[1].y=100;
    p[2].x=550;p[2].y=250;

    initgraph(&gd,&gm,"");

    win();
    setcolor(YELLOW);
    setlinestyle(DASHED_LINE,0,1);
    line(150,250,350,100);
    line(350,100,550,250);
    line(550,250,150,250);

    delay(1500);

    for(i=0;i<4;i++){
        n=clip(p,n,t,i);
        {
            int j;
            for(j=0;j<n;j++)p[j]=t[j];
        }
        cleardevice();
        win();
        draw(p,n);
        delay(1500);
    }

    getch();
    closegraph();
    return 0;
}
