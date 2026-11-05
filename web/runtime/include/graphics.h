/* Browser-backed implementation of the Turbo C / WinBGI graphics API.
   Drawing goes to an in-memory framebuffer that is streamed to the page's canvas. */
#ifndef WEBBGI_GRAPHICS_H
#define WEBBGI_GRAPHICS_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum colors { BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY, DARKGRAY,
              LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };
enum fill_styles { EMPTY_FILL, SOLID_FILL, LINE_FILL, LTSLASH_FILL, SLASH_FILL, BKSLASH_FILL,
                   LTBKSLASH_FILL, HATCH_FILL, XHATCH_FILL, INTERLEAVE_FILL, WIDE_DOT_FILL,
                   CLOSE_DOT_FILL, USER_FILL };
enum line_styles { SOLID_LINE, DOTTED_LINE, CENTER_LINE, DASHED_LINE, USERBIT_LINE };
enum line_widths { NORM_WIDTH = 1, THICK_WIDTH = 3 };
enum font_names { DEFAULT_FONT, TRIPLEX_FONT, SMALL_FONT, SANS_SERIF_FONT, GOTHIC_FONT };
enum text_dirs { HORIZ_DIR, VERT_DIR };
enum text_just { LEFT_TEXT = 0, CENTER_TEXT = 1, RIGHT_TEXT = 2, BOTTOM_TEXT = 0, TOP_TEXT = 2 };
enum put_ops { COPY_PUT, XOR_PUT, OR_PUT, AND_PUT, NOT_PUT };
#define DETECT 0
#define VGA 9
#define VGAHI 2
#define grOk 0
#define MAXCOLORS 15

struct arccoordstype { int x, y, xstart, ystart, xend, yend; };

void initgraph(int *driver, int *mode, const char *path);
int  initwindow(int width, int height, ...);
void closegraph(void);
int  graphresult(void);
const char *grapherrormsg(int code);
int  getmaxx(void);
int  getmaxy(void);
int  getmaxcolor(void);
void cleardevice(void);
void setviewport(int left, int top, int right, int bottom, int clip);
void clearviewport(void);

void setcolor(int color);
int  getcolor(void);
void setbkcolor(int color);
int  getbkcolor(void);
void setfillstyle(int pattern, int color);
void setlinestyle(int style, unsigned pattern, int thickness);

void putpixel(int x, int y, int color);
int  getpixel(int x, int y);
void moveto(int x, int y);
void moverel(int dx, int dy);
int  getx(void);
int  gety(void);
void line(int x1, int y1, int x2, int y2);
void lineto(int x, int y);
void linerel(int dx, int dy);
void rectangle(int left, int top, int right, int bottom);
void bar(int left, int top, int right, int bottom);
void bar3d(int left, int top, int right, int bottom, int depth, int topflag);
void circle(int x, int y, int radius);
void arc(int x, int y, int start, int end, int radius);
void ellipse(int x, int y, int start, int end, int xradius, int yradius);
void fillellipse(int x, int y, int xradius, int yradius);
void pieslice(int x, int y, int start, int end, int radius);
void sector(int x, int y, int start, int end, int xradius, int yradius);
void getarccoords(struct arccoordstype *coords);
void drawpoly(int count, const int *points);
void fillpoly(int count, const int *points);
void floodfill(int x, int y, int border);

void settextstyle(int font, int direction, int size);
void settextjustify(int horizontal, int vertical);
void outtext(const char *text);
void outtextxy(int x, int y, const char *text);
int  textwidth(const char *text);
int  textheight(const char *text);

unsigned imagesize(int left, int top, int right, int bottom);
void getimage(int left, int top, int right, int bottom, void *buffer);
void putimage(int left, int top, const void *buffer, int op);

/* Keyboard input comes from the page's graphics panel. */
int  getch(void);
int  getche(void);
int  kbhit(void);
void clrscr(void);
void delay(int milliseconds);
void sound(unsigned frequency);
void nosound(void);
#endif
