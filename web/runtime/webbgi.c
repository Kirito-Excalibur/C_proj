/* Implementation of graphics.h. Linked into every program the site runs.
   fd 3 streams messages to the server: [type:u8][length:u32le][payload]
     I width,height (u16)   P background colour (u8)   F x,y,w,h (u16) + RLE (count,colour)
     W program is waiting for a key   C closegraph
   fd 4 carries key presses from the page. */
#include <graphics.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "font8x8.h"

enum { GFX_FD = 3, KEY_FD = 4, MAX_W = 1920, MAX_H = 1080, BATCH_MS = 33, IDLE_MS = 40 };

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *fb, *rle;
static int W, H, ready;
static int dx0, dy0, dx1, dy1, dirty;
static int64_t last_flush, last_op;
static int color = WHITE, bkcolor = BLACK, fillcolor = WHITE, fillstyle = SOLID_FILL;
static int linestyle = SOLID_LINE, thickness = 1;
static unsigned linepat = 0xFFFF;
static int cpx, cpy;
static int vl, vt, vr, vb, vclip = 1;
static int txtsize = 1, txtdir = HORIZ_DIR, txth = LEFT_TEXT, txtv = TOP_TEXT;
static struct arccoordstype lastarc;
static int wait_announced;

static const unsigned char patterns[12][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {255, 255, 255, 255, 255, 255, 255, 255},
    {255, 255, 0, 0, 255, 255, 0, 0},
    {1, 2, 4, 8, 16, 32, 64, 128},
    {7, 14, 28, 56, 112, 224, 193, 131},
    {224, 112, 56, 28, 14, 7, 131, 193},
    {128, 64, 32, 16, 8, 4, 2, 1},
    {255, 136, 136, 136, 255, 136, 136, 136},
    {129, 66, 36, 24, 24, 36, 66, 129},
    {204, 51, 204, 51, 204, 51, 204, 51},
    {128, 0, 8, 0, 128, 0, 8, 0},
    {136, 0, 34, 0, 136, 0, 34, 0},
};

static int64_t now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static void write_all(const void *data, size_t n) {
    const char *p = data;
    while (n > 0) {
        ssize_t w = write(GFX_FD, p, n);
        if (w <= 0) _exit(0); /* the page has gone away */
        p += w;
        n -= (size_t)w;
    }
}

static void send_msg(char type, const void *payload, uint32_t len) {
    unsigned char h[5] = {(unsigned char)type, len & 255, (len >> 8) & 255, (len >> 16) & 255, len >> 24};
    write_all(h, 5);
    if (len) write_all(payload, len);
}

static void flush_locked(void) {
    if (!dirty || !ready) return;
    int w = dx1 - dx0 + 1, h = dy1 - dy0 + 1;
    uint32_t n = 8;
    rle[0] = dx0 & 255; rle[1] = dx0 >> 8;
    rle[2] = dy0 & 255; rle[3] = dy0 >> 8;
    rle[4] = w & 255;   rle[5] = w >> 8;
    rle[6] = h & 255;   rle[7] = h >> 8;
    int run = 0, cur = -1;
    for (int y = dy0; y <= dy1; y++) {
        const unsigned char *row = fb + (size_t)y * W;
        for (int x = dx0; x <= dx1; x++) {
            if (row[x] == cur && run < 255) { run++; continue; }
            if (run) { rle[n++] = (unsigned char)run; rle[n++] = (unsigned char)cur; }
            cur = row[x];
            run = 1;
        }
    }
    if (run) { rle[n++] = (unsigned char)run; rle[n++] = (unsigned char)cur; }
    send_msg('F', rle, n);
    dirty = 0;
    last_flush = now_ms();
}

/* Every public drawing call runs between these two. */
static void begin(void) { pthread_mutex_lock(&lock); }
static void end(void) {
    if (dirty) {
        last_op = now_ms();
        if (last_op - last_flush >= BATCH_MS) flush_locked();
    }
    pthread_mutex_unlock(&lock);
}

/* Shows drawing that is followed by a long pause with no delay/getch, e.g. scanf. */
static void *idle_flusher(void *arg) {
    (void)arg;
    for (;;) {
        usleep(IDLE_MS * 1000);
        pthread_mutex_lock(&lock);
        if (dirty && now_ms() - last_op >= IDLE_MS) flush_locked();
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static void at_exit_flush(void) {
    pthread_mutex_lock(&lock);
    flush_locked();
    pthread_mutex_unlock(&lock);
}

/* Prompts must reach the console before the program blocks on input. */
__attribute__((constructor)) static void runtime_init(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
}

static void open_window(int w, int h) {
    begin();
    if (ready) { pthread_mutex_unlock(&lock); return; }
    if (w < 1 || w > MAX_W) w = 800;
    if (h < 1 || h > MAX_H) h = 600;
    W = w; H = h;
    fb = calloc((size_t)W * H, 1);
    rle = malloc((size_t)W * H * 2 + 16);
    if (!fb || !rle) { fputs("graphics: out of memory\n", stderr); exit(1); }
    vl = vt = 0; vr = W - 1; vb = H - 1; vclip = 1;
    ready = 1;
    unsigned char m[4] = {W & 255, W >> 8, H & 255, H >> 8};
    send_msg('I', m, 4);
    static int started;
    if (!started) {
        pthread_t t;
        started = 1;
        pthread_create(&t, NULL, idle_flusher, NULL);
        pthread_detach(t);
        atexit(at_exit_flush);
    }
    pthread_mutex_unlock(&lock);
}

void initgraph(int *driver, int *mode, const char *path) {
    (void)path;
    if (driver) *driver = VGA;
    if (mode) *mode = VGAHI;
    open_window(800, 600);
}
int initwindow(int width, int height, ...) { open_window(width, height); return 1; }
void closegraph(void) {
    begin();
    if (ready) {
        flush_locked();
        send_msg('C', NULL, 0);
        ready = 0;
        free(fb); free(rle);
        fb = rle = NULL;
    }
    pthread_mutex_unlock(&lock);
}
int graphresult(void) { return grOk; }
const char *grapherrormsg(int code) { (void)code; return "No error"; }
int getmaxx(void) { return W ? W - 1 : 799; }
int getmaxy(void) { return H ? H - 1 : 599; }
int getmaxcolor(void) { return 15; }

/* ---- pixels ---- */
static void mark(int x, int y) {
    if (!dirty) { dx0 = dx1 = x; dy0 = dy1 = y; dirty = 1; return; }
    if (x < dx0) dx0 = x;
    if (x > dx1) dx1 = x;
    if (y < dy0) dy0 = y;
    if (y > dy1) dy1 = y;
}
/* x, y are absolute window coordinates. */
static int visible(int x, int y) {
    if (x < 0 || y < 0 || x >= W || y >= H) return 0;
    return !vclip || (x >= vl && x <= vr && y >= vt && y <= vb);
}
static void put_abs(int x, int y, int c) {
    if (!ready || !visible(x, y)) return;
    fb[(size_t)y * W + x] = (unsigned char)(c & 15);
    mark(x, y);
}
static void put_fill_abs(int x, int y) {
    unsigned char row = patterns[fillstyle][y & 7];
    put_abs(x, y, ((row >> (7 - (x & 7))) & 1) ? fillcolor : 0);
}
static void pen(int x, int y) { /* relative, honours line thickness */
    x += vl; y += vt;
    put_abs(x, y, color);
    if (thickness >= 3) {
        put_abs(x - 1, y, color); put_abs(x + 1, y, color);
        put_abs(x, y - 1, color); put_abs(x, y + 1, color);
    }
}
static void fill_span(int x0, int x1, int y) { /* relative */
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y + vt < 0 || y + vt >= H) return;
    if (x0 + vl < 0) x0 = -vl;
    if (x1 + vl >= W) x1 = W - 1 - vl;
    for (int x = x0; x <= x1; x++) put_fill_abs(x + vl, y + vt);
}

void putpixel(int x, int y, int c) { begin(); put_abs(x + vl, y + vt, c); end(); }
int getpixel(int x, int y) {
    begin();
    x += vl; y += vt;
    int c = (ready && x >= 0 && y >= 0 && x < W && y < H) ? fb[(size_t)y * W + x] : 0;
    pthread_mutex_unlock(&lock);
    return c;
}

/* ---- state ---- */
void setcolor(int c) { color = c & 15; }
int getcolor(void) { return color; }
void setbkcolor(int c) {
    begin();
    bkcolor = c & 15;
    if (ready) {
        unsigned char b = (unsigned char)bkcolor;
        flush_locked();
        send_msg('P', &b, 1);
    }
    pthread_mutex_unlock(&lock);
}
int getbkcolor(void) { return bkcolor; }
void setfillstyle(int style, int c) {
    fillstyle = (style >= 0 && style < 12) ? style : SOLID_FILL;
    fillcolor = c & 15;
}
void setlinestyle(int style, unsigned pattern, int thick) {
    static const unsigned styles[] = {0xFFFF, 0xCCCC, 0xF8F8, 0xF0F0};
    linestyle = style;
    linepat = (style >= 0 && style < 4) ? styles[style] : (pattern & 0xFFFF);
    thickness = thick;
}
void moveto(int x, int y) { cpx = x; cpy = y; }
void moverel(int dx, int dy) { cpx += dx; cpy += dy; }
int getx(void) { return cpx; }
int gety(void) { return cpy; }
void setviewport(int l, int t, int r, int b, int clip) {
    vl = l; vt = t; vr = r; vb = b; vclip = clip;
    cpx = cpy = 0;
}
void cleardevice(void) {
    begin();
    if (ready) {
        memset(fb, 0, (size_t)W * H);
        dx0 = dy0 = 0; dx1 = W - 1; dy1 = H - 1; dirty = 1;
    }
    cpx = cpy = 0;
    end();
}
void clearviewport(void) {
    begin();
    for (int y = vt; y <= vb; y++)
        for (int x = vl; x <= vr; x++) put_abs(x, y, 0);
    cpx = cpy = 0;
    end();
}

/* ---- lines ---- */
static void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy, bit = 0;
    /* Bail out of absurd coordinates instead of walking millions of off-screen pixels. */
    if (dx > 100000 || dy < -100000) return;
    for (;;) {
        if (linepat & (0x8000u >> (bit & 15))) pen(x0, y0);
        bit++;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void line(int x1, int y1, int x2, int y2) { begin(); draw_line(x1, y1, x2, y2); end(); }
void lineto(int x, int y) { begin(); draw_line(cpx, cpy, x, y); cpx = x; cpy = y; end(); }
void linerel(int dx, int dy) { lineto(cpx + dx, cpy + dy); }
static void draw_rect(int l, int t, int r, int b) {
    draw_line(l, t, r, t); draw_line(r, t, r, b);
    draw_line(r, b, l, b); draw_line(l, b, l, t);
}
void rectangle(int l, int t, int r, int b) { begin(); draw_rect(l, t, r, b); end(); }
static void fill_rect(int l, int t, int r, int b) {
    if (t > b) { int s = t; t = b; b = s; }
    if (t + vt < 0) t = -vt;
    if (b + vt >= H) b = H - 1 - vt;
    for (int y = t; y <= b; y++) fill_span(l, r, y);
}
void bar(int l, int t, int r, int b) { begin(); fill_rect(l, t, r, b); end(); }
void bar3d(int l, int t, int r, int b, int d, int top) {
    begin();
    fill_rect(l, t, r, b);
    draw_rect(l, t, r, b);
    draw_line(r, b, r + d, b - d);
    draw_line(r + d, b - d, r + d, t - d);
    draw_line(r, t, r + d, t - d);
    if (top) { draw_line(l, t, l + d, t - d); draw_line(l + d, t - d, r + d, t - d); }
    end();
}

/* ---- curves: angles are degrees, counter-clockwise, 0 at 3 o'clock ---- */
static const double RAD = 3.14159265358979323846 / 180.0;
static void ang_point(int x, int y, double a, int rx, int ry, int *px, int *py) {
    *px = x + (int)lround(rx * cos(a * RAD));
    *py = y - (int)lround(ry * sin(a * RAD));
}
static void draw_arc(int x, int y, int s, int e, int rx, int ry) {
    if (rx < 0) rx = -rx;
    if (ry < 0) ry = -ry;
    if (rx > 20000 || ry > 20000) return;
    s %= 360; e %= 360;
    if (s < 0) s += 360;
    if (e < 0) e += 360;
    if (e <= s) e += 360;
    int big = (rx > ry ? rx : ry) + 1;
    int steps = (int)(big * (e - s) * RAD * 2) + 2;
    int px, py;
    for (int i = 0; i <= steps; i++) {
        ang_point(x, y, s + (double)(e - s) * i / steps, rx, ry, &px, &py);
        pen(px, py);
    }
    lastarc.x = x; lastarc.y = y;
    ang_point(x, y, s, rx, ry, &lastarc.xstart, &lastarc.ystart);
    ang_point(x, y, e, rx, ry, &lastarc.xend, &lastarc.yend);
}
void circle(int x, int y, int r) { begin(); draw_arc(x, y, 0, 360, r, r); end(); }
void arc(int x, int y, int s, int e, int r) { begin(); draw_arc(x, y, s, e, r, r); end(); }
void ellipse(int x, int y, int s, int e, int rx, int ry) { begin(); draw_arc(x, y, s, e, rx, ry); end(); }
void getarccoords(struct arccoordstype *c) { *c = lastarc; }
void fillellipse(int x, int y, int rx, int ry) {
    begin();
    if (rx < 0) rx = -rx;
    if (ry < 0) ry = -ry;
    for (int dy = -ry; dy <= ry; dy++) {
        double k = ry ? (double)dy / ry : 0;
        int half = (int)lround(rx * sqrt(1.0 - k * k));
        fill_span(x - half, x + half, y + dy);
    }
    draw_arc(x, y, 0, 360, rx, ry);
    end();
}

/* ---- polygons ---- */
static void fill_polygon(int n, const int *p) {
    if (n < 3) return;
    int ymin = p[1], ymax = p[1];
    for (int i = 1; i < n; i++) {
        if (p[2 * i + 1] < ymin) ymin = p[2 * i + 1];
        if (p[2 * i + 1] > ymax) ymax = p[2 * i + 1];
    }
    if (ymin + vt < 0) ymin = -vt;
    if (ymax + vt >= H) ymax = H - 1 - vt;
    double *xs = malloc(sizeof(double) * (size_t)(n + 1));
    if (!xs) return;
    for (int y = ymin; y <= ymax; y++) {
        int k = 0;
        double sy = y + 0.5;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            double y0 = p[2 * i + 1], y1 = p[2 * j + 1], x0 = p[2 * i], x1 = p[2 * j];
            if ((y0 <= sy && y1 > sy) || (y1 <= sy && y0 > sy))
                xs[k++] = x0 + (sy - y0) * (x1 - x0) / (y1 - y0);
        }
        for (int a = 1; a < k; a++) { /* insertion sort: k is tiny */
            double v = xs[a];
            int b = a - 1;
            while (b >= 0 && xs[b] > v) { xs[b + 1] = xs[b]; b--; }
            xs[b + 1] = v;
        }
        for (int a = 0; a + 1 < k; a += 2)
            fill_span((int)lround(xs[a]), (int)lround(xs[a + 1]), y);
    }
    free(xs);
}
void drawpoly(int n, const int *p) {
    begin();
    for (int i = 0; i + 1 < n; i++) draw_line(p[2 * i], p[2 * i + 1], p[2 * i + 2], p[2 * i + 3]);
    end();
}
void fillpoly(int n, const int *p) {
    begin();
    fill_polygon(n, p);
    for (int i = 0; i < n && n > 1; i++) {
        int j = (i + 1) % n;
        draw_line(p[2 * i], p[2 * i + 1], p[2 * j], p[2 * j + 1]);
    }
    end();
}
void sector(int x, int y, int s, int e, int rx, int ry) {
    begin();
    s %= 360; e %= 360;
    if (s < 0) s += 360;
    if (e < 0) e += 360;
    if (e <= s) e += 360;
    enum { STEPS = 180 };
    int pts[2 * (STEPS + 2)];
    pts[0] = x; pts[1] = y;
    for (int i = 0; i <= STEPS; i++)
        ang_point(x, y, s + (double)(e - s) * i / STEPS, rx, ry, &pts[2 * i + 2], &pts[2 * i + 3]);
    fill_polygon(STEPS + 2, pts);
    draw_line(x, y, pts[2], pts[3]);
    draw_line(x, y, pts[2 * STEPS + 2], pts[2 * STEPS + 3]);
    draw_arc(x, y, s, e, rx, ry);
    end();
}
void pieslice(int x, int y, int s, int e, int r) { sector(x, y, s, e, r, r); }

void floodfill(int x, int y, int border) {
    begin();
    x += vl; y += vt;
    border &= 15;
    if (!ready || !visible(x, y) || fb[(size_t)y * W + x] == border) { end(); return; }
    unsigned char *seen = calloc((size_t)W * H, 1);
    size_t cap = 4096, top = 0;
    int *stack = malloc(cap * 2 * sizeof(int));
    if (!seen || !stack) { free(seen); free(stack); end(); return; }
#define OPEN(px, py) (visible((px), (py)) && !seen[(size_t)(py) * W + (px)] && fb[(size_t)(py) * W + (px)] != border)
    stack[0] = x; stack[1] = y; top = 1;
    while (top) {
        top--;
        int cx = stack[2 * top], cy = stack[2 * top + 1];
        if (!OPEN(cx, cy)) continue;
        int l = cx, r = cx;
        while (OPEN(l - 1, cy)) l--;
        while (OPEN(r + 1, cy)) r++;
        for (int i = l; i <= r; i++) seen[(size_t)cy * W + i] = 1;
        for (int ny = cy - 1; ny <= cy + 1; ny += 2) {
            int inrun = 0;
            for (int i = l; i <= r; i++) {
                int open = OPEN(i, ny);
                if (open && !inrun) {
                    if (top == cap) {
                        cap *= 2;
                        int *grown = realloc(stack, cap * 2 * sizeof(int));
                        if (!grown) { free(seen); free(stack); end(); return; }
                        stack = grown;
                    }
                    stack[2 * top] = i; stack[2 * top + 1] = ny; top++;
                }
                inrun = open;
            }
        }
        for (int i = l; i <= r; i++) put_fill_abs(i, cy);
    }
#undef OPEN
    free(seen);
    free(stack);
    end();
}

/* ---- text ---- */
void settextstyle(int font, int dir, int size) {
    (void)font; /* every font renders with the one 8x8 bitmap */
    txtdir = dir;
    txtsize = size < 1 ? 1 : size;
}
void settextjustify(int h, int v) { txth = h; txtv = v; }
int textwidth(const char *s) { return (int)strlen(s) * 8 * txtsize; }
int textheight(const char *s) { (void)s; return 8 * txtsize; }
static void draw_text(int x, int y, const char *s) {
    int tw = (int)strlen(s) * 8 * txtsize, th = 8 * txtsize;
    if (txtdir == HORIZ_DIR) {
        if (txth == CENTER_TEXT) x -= tw / 2; else if (txth == RIGHT_TEXT) x -= tw;
        if (txtv == CENTER_TEXT) y -= th / 2; else if (txtv == BOTTOM_TEXT) y -= th;
    } else {
        if (txth == CENTER_TEXT) x -= th / 2; else if (txth == RIGHT_TEXT) x -= th;
        if (txtv == CENTER_TEXT) y += tw / 2; else if (txtv == TOP_TEXT) y += tw;
    }
    for (int i = 0; s[i]; i++) {
        const unsigned char *g = font8x8[(unsigned char)s[i]];
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) {
                if (!((g[r] >> (7 - c)) & 1)) continue;
                for (int a = 0; a < txtsize; a++)
                    for (int b = 0; b < txtsize; b++) {
                        int u = (i * 8 + c) * txtsize + a, v = r * txtsize + b;
                        if (txtdir == HORIZ_DIR) put_abs(x + u + vl, y + v + vt, color);
                        else put_abs(x + v + vl, y - u + vt, color);
                    }
            }
    }
}
void outtextxy(int x, int y, const char *s) { begin(); draw_text(x, y, s); end(); }
void outtext(const char *s) {
    begin();
    draw_text(cpx, cpy, s);
    if (txth == LEFT_TEXT && txtdir == HORIZ_DIR) cpx += (int)strlen(s) * 8 * txtsize;
    end();
}

/* ---- images ---- */
unsigned imagesize(int l, int t, int r, int b) {
    return (unsigned)(2 * sizeof(int) + (size_t)(abs(r - l) + 1) * (size_t)(abs(b - t) + 1));
}
void getimage(int l, int t, int r, int b, void *buffer) {
    begin();
    int w = abs(r - l) + 1, h = abs(b - t) + 1;
    int *head = buffer;
    unsigned char *d = (unsigned char *)(head + 2);
    head[0] = w; head[1] = h;
    if (l > r) l = r;
    if (t > b) t = b;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int ax = l + x + vl, ay = t + y + vt;
            d[(size_t)y * w + x] = (ready && ax >= 0 && ay >= 0 && ax < W && ay < H) ? fb[(size_t)ay * W + ax] : 0;
        }
    pthread_mutex_unlock(&lock);
}
void putimage(int l, int t, const void *buffer, int op) {
    begin();
    const int *head = buffer;
    int w = head[0], h = head[1];
    const unsigned char *d = (const unsigned char *)(head + 2);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int ax = l + x + vl, ay = t + y + vt;
            if (!ready || ax < 0 || ay < 0 || ax >= W || ay >= H) continue;
            int s = d[(size_t)y * w + x], o = fb[(size_t)ay * W + ax], v = s;
            if (op == XOR_PUT) v = o ^ s;
            else if (op == OR_PUT) v = o | s;
            else if (op == AND_PUT) v = o & s;
            else if (op == NOT_PUT) v = ~s & 15;
            put_abs(ax, ay, v);
        }
    end();
}

/* ---- keyboard, timing, console ---- */
static void announce_wait(void) {
    pthread_mutex_lock(&lock);
    flush_locked();
    send_msg('W', NULL, 0);
    pthread_mutex_unlock(&lock);
}
int getch(void) {
    unsigned char c;
    announce_wait();
    wait_announced = 1;
    return read(KEY_FD, &c, 1) == 1 ? c : -1;
}
int getche(void) {
    int c = getch();
    if (c > 0) putchar(c);
    return c;
}
int kbhit(void) {
    struct pollfd p = {KEY_FD, POLLIN, 0};
    if (!wait_announced) { announce_wait(); wait_announced = 1; }
    pthread_mutex_lock(&lock);
    flush_locked();
    pthread_mutex_unlock(&lock);
    /* Waits up to 1ms so a bare while(!kbhit()) loop does not burn the CPU limit. */
    return poll(&p, 1, 1) > 0 && (p.revents & POLLIN);
}
void clrscr(void) { fputs("\f", stdout); }
void delay(int ms) {
    pthread_mutex_lock(&lock);
    flush_locked();
    pthread_mutex_unlock(&lock);
    if (ms > 0) {
        struct timespec t = {ms / 1000, (long)(ms % 1000) * 1000000L};
        nanosleep(&t, NULL);
    }
    /* Restart the batching clock so the next frame is sent whole, without flicker. */
    pthread_mutex_lock(&lock);
    last_flush = last_op = now_ms();
    pthread_mutex_unlock(&lock);
}
void sound(unsigned f) { (void)f; }
void nosound(void) {}
