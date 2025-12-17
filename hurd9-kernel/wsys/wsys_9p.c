/*
 * Pure 9P Window System - No Wayland, No X11, Just Files
 * Based on Plan 9's rio/draw model
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/9p.h"
#include "../include/balanced_ternary.h"

/* Window system namespace */
#define WSYS_ROOT "/dev/wsys"
#define MAX_WINDOWS 256
#define MAX_PATH 256

/* Drawing operations (Plan 9 style) */
typedef enum {
    DRAW_CLEAR,
    DRAW_POINT,
    DRAW_LINE,
    DRAW_RECT,
    DRAW_FILLRECT,
    DRAW_ELLIPSE,
    DRAW_FILLELLIPSE,
    DRAW_TEXT,
    DRAW_IMAGE,
    DRAW_COMPOSITE
} DrawOp;

/* Point structure */
typedef struct {
    int32_t x;
    int32_t y;
} Point;

/* Rectangle structure */
typedef struct {
    Point min;
    Point max;
} Rectangle;

/* Color in RGBA8888 */
typedef uint32_t Color;

/* Image structure */
typedef struct {
    uint32_t id;
    Rectangle r;
    uint32_t *data;  /* Raw pixel data */
    size_t stride;
    bool dirty;
} Image;

/* Window structure */
typedef struct Window {
    uint32_t id;
    char name[64];
    Rectangle rect;
    Image *framebuffer;
    
    /* Balanced ternary capabilities */
    trit_t read_cap;   /* -1: deny, 0: inherit, +1: allow */
    trit_t write_cap;
    trit_t resize_cap;
    
    /* Window hierarchy */
    struct Window *parent;
    struct Window *children[32];
    uint32_t nchildren;
    
    /* Event queue */
    struct {
        uint32_t type;
        union {
            Point mouse;
            uint32_t key;
            Rectangle resize;
        } data;
    } events[256];
    uint32_t event_head;
    uint32_t event_tail;
    
    /* Draw state */
    Color fg_color;
    Color bg_color;
    Point cursor;
    
    /* 9P file handles */
    int ctl_fd;
    int draw_fd;
    int events_fd;
    int cons_fd;
    
    pthread_mutex_t lock;
} Window;

/* Window system state */
typedef struct {
    Window *windows[MAX_WINDOWS];
    uint32_t nwindows;
    Window *focused;
    
    /* Screen info */
    Rectangle screen;
    Image *screen_fb;
    
    /* Mouse state */
    Point mouse_pos;
    uint32_t mouse_buttons;
    
    /* Compositor thread */
    pthread_t compositor;
    bool running;
    
    pthread_mutex_t lock;
} WindowSystem;

static WindowSystem *wsys = NULL;

/* 9P filesystem operations */
static int wsys_create_window_files(Window *w) {
    char path[MAX_PATH];
    
    /* Create window directory */
    snprintf(path, MAX_PATH, "%s/%d", WSYS_ROOT, w->id);
    mkdir(path, 0755);
    
    /* Create control file */
    snprintf(path, MAX_PATH, "%s/%d/ctl", WSYS_ROOT, w->id);
    w->ctl_fd = open(path, O_RDWR | O_CREAT, 0644);
    
    /* Create draw file */
    snprintf(path, MAX_PATH, "%s/%d/draw", WSYS_ROOT, w->id);
    w->draw_fd = open(path, O_RDWR | O_CREAT, 0644);
    
    /* Create events file */
    snprintf(path, MAX_PATH, "%s/%d/events", WSYS_ROOT, w->id);
    w->events_fd = open(path, O_RDWR | O_CREAT, 0644);
    
    /* Create console file */
    snprintf(path, MAX_PATH, "%s/%d/cons", WSYS_ROOT, w->id);
    w->cons_fd = open(path, O_RDWR | O_CREAT, 0644);
    
    return 0;
}

/* Create new window */
static Window *wsys_new_window(const char *name, Rectangle r) {
    Window *w = calloc(1, sizeof(Window));
    if (!w) return NULL;
    
    pthread_mutex_lock(&wsys->lock);
    
    w->id = wsys->nwindows++;
    strncpy(w->name, name, sizeof(w->name) - 1);
    w->rect = r;
    
    /* Initialize capabilities with balanced ternary */
    w->read_cap = TRIT_TRUE;    /* +1: allowed */
    w->write_cap = TRIT_TRUE;   /* +1: allowed */
    w->resize_cap = TRIT_ZERO;  /* 0: inherit from parent */
    
    /* Allocate framebuffer */
    size_t fb_size = (r.max.x - r.min.x) * (r.max.y - r.min.y) * sizeof(uint32_t);
    w->framebuffer = malloc(sizeof(Image));
    w->framebuffer->data = malloc(fb_size);
    w->framebuffer->r = r;
    w->framebuffer->stride = (r.max.x - r.min.x) * sizeof(uint32_t);
    
    /* Default colors */
    w->fg_color = 0xFF000000;  /* Black */
    w->bg_color = 0xFFFFFFFF;  /* White */
    
    pthread_mutex_init(&w->lock, NULL);
    
    /* Create 9P files for this window */
    wsys_create_window_files(w);
    
    wsys->windows[w->id] = w;
    
    pthread_mutex_unlock(&wsys->lock);
    
    return w;
}

/* Handle draw commands via 9P write to draw file */
static ssize_t wsys_draw_write(Window *w, const char *cmd, size_t len) {
    char op[32];
    int args[8];
    
    if (sscanf(cmd, "%31s", op) != 1) {
        return -EINVAL;
    }
    
    pthread_mutex_lock(&w->lock);
    
    if (strcmp(op, "clear") == 0) {
        uint32_t color;
        if (sscanf(cmd, "clear %x", &color) == 1) {
            /* Clear entire window */
            size_t pixels = (w->rect.max.x - w->rect.min.x) * 
                          (w->rect.max.y - w->rect.min.y);
            for (size_t i = 0; i < pixels; i++) {
                w->framebuffer->data[i] = color;
            }
            w->framebuffer->dirty = true;
        }
    }
    else if (strcmp(op, "line") == 0) {
        Point p1, p2;
        uint32_t color;
        if (sscanf(cmd, "line %d %d %d %d %x", 
                   &p1.x, &p1.y, &p2.x, &p2.y, &color) == 5) {
            /* Bresenham line drawing */
            int dx = abs(p2.x - p1.x);
            int dy = abs(p2.y - p1.y);
            int sx = p1.x < p2.x ? 1 : -1;
            int sy = p1.y < p2.y ? 1 : -1;
            int err = dx - dy;
            
            while (1) {
                /* Plot point */
                int width = w->rect.max.x - w->rect.min.x;
                if (p1.x >= 0 && p1.x < width && 
                    p1.y >= 0 && p1.y < (w->rect.max.y - w->rect.min.y)) {
                    w->framebuffer->data[p1.y * width + p1.x] = color;
                }
                
                if (p1.x == p2.x && p1.y == p2.y) break;
                
                int e2 = 2 * err;
                if (e2 > -dy) {
                    err -= dy;
                    p1.x += sx;
                }
                if (e2 < dx) {
                    err += dx;
                    p1.y += sy;
                }
            }
            w->framebuffer->dirty = true;
        }
    }
    else if (strcmp(op, "rect") == 0) {
        Rectangle r;
        uint32_t color;
        if (sscanf(cmd, "rect %d %d %d %d %x",
                   &r.min.x, &r.min.y, &r.max.x, &r.max.y, &color) == 5) {
            /* Draw rectangle outline */
            int width = w->rect.max.x - w->rect.min.x;
            
            /* Top and bottom edges */
            for (int x = r.min.x; x <= r.max.x; x++) {
                if (x >= 0 && x < width) {
                    w->framebuffer->data[r.min.y * width + x] = color;
                    w->framebuffer->data[r.max.y * width + x] = color;
                }
            }
            
            /* Left and right edges */
            for (int y = r.min.y; y <= r.max.y; y++) {
                if (y >= 0 && y < (w->rect.max.y - w->rect.min.y)) {
                    w->framebuffer->data[y * width + r.min.x] = color;
                    w->framebuffer->data[y * width + r.max.x] = color;
                }
            }
            w->framebuffer->dirty = true;
        }
    }
    else if (strcmp(op, "fillrect") == 0) {
        Rectangle r;
        uint32_t color;
        if (sscanf(cmd, "fillrect %d %d %d %d %x",
                   &r.min.x, &r.min.y, &r.max.x, &r.max.y, &color) == 5) {
            /* Fill rectangle */
            int width = w->rect.max.x - w->rect.min.x;
            
            for (int y = r.min.y; y <= r.max.y; y++) {
                for (int x = r.min.x; x <= r.max.x; x++) {
                    if (x >= 0 && x < width &&
                        y >= 0 && y < (w->rect.max.y - w->rect.min.y)) {
                        w->framebuffer->data[y * width + x] = color;
                    }
                }
            }
            w->framebuffer->dirty = true;
        }
    }
    else if (strcmp(op, "text") == 0) {
        int x, y;
        char text[256];
        uint32_t color;
        if (sscanf(cmd, "text %d %d %x %255[^\n]", &x, &y, &color, text) == 4) {
            /* Simple text rendering (would use bitmap font in real implementation) */
            /* For now, just draw a marker where text would be */
            int width = w->rect.max.x - w->rect.min.x;
            if (x >= 0 && x < width && 
                y >= 0 && y < (w->rect.max.y - w->rect.min.y)) {
                /* Draw small cross to indicate text position */
                for (int i = -2; i <= 2; i++) {
                    if (x + i >= 0 && x + i < width) {
                        w->framebuffer->data[y * width + x + i] = color;
                    }
                    if (y + i >= 0 && y + i < (w->rect.max.y - w->rect.min.y)) {
                        w->framebuffer->data[(y + i) * width + x] = color;
                    }
                }
            }
            w->framebuffer->dirty = true;
        }
    }
    
    pthread_mutex_unlock(&w->lock);
    
    return len;
}

/* Handle control commands via 9P write to ctl file */
static ssize_t wsys_ctl_write(Window *w, const char *cmd, size_t len) {
    char op[32];
    
    if (sscanf(cmd, "%31s", op) != 1) {
        return -EINVAL;
    }
    
    pthread_mutex_lock(&w->lock);
    
    if (strcmp(op, "resize") == 0) {
        Rectangle r;
        if (sscanf(cmd, "resize %d %d %d %d",
                   &r.min.x, &r.min.y, &r.max.x, &r.max.y) == 4) {
            if (w->resize_cap == TRIT_TRUE) {
                /* Resize window and reallocate framebuffer */
                w->rect = r;
                
                free(w->framebuffer->data);
                size_t fb_size = (r.max.x - r.min.x) * (r.max.y - r.min.y) * sizeof(uint32_t);
                w->framebuffer->data = malloc(fb_size);
                w->framebuffer->r = r;
                w->framebuffer->stride = (r.max.x - r.min.x) * sizeof(uint32_t);
                w->framebuffer->dirty = true;
            }
        }
    }
    else if (strcmp(op, "show") == 0) {
        /* Make window visible */
        w->framebuffer->dirty = true;
    }
    else if (strcmp(op, "hide") == 0) {
        /* Hide window (compositor will skip it) */
        /* In real implementation, would set visibility flag */
    }
    else if (strcmp(op, "focus") == 0) {
        /* Request focus */
        pthread_mutex_lock(&wsys->lock);
        wsys->focused = w;
        pthread_mutex_unlock(&wsys->lock);
    }
    else if (strcmp(op, "capability") == 0) {
        char cap[32];
        int value;
        if (sscanf(cmd, "capability %31s %d", cap, &value) == 2) {
            /* Set balanced ternary capability */
            trit_t trit_val = (value < 0) ? TRIT_FALSE : 
                             (value > 0) ? TRIT_TRUE : TRIT_ZERO;
            
            if (strcmp(cap, "read") == 0) {
                w->read_cap = trit_val;
            } else if (strcmp(cap, "write") == 0) {
                w->write_cap = trit_val;
            } else if (strcmp(cap, "resize") == 0) {
                w->resize_cap = trit_val;
            }
        }
    }
    
    pthread_mutex_unlock(&w->lock);
    
    return len;
}

/* Compositor thread - combines all windows to screen */
static void *wsys_compositor_thread(void *arg) {
    while (wsys->running) {
        pthread_mutex_lock(&wsys->lock);
        
        /* Clear screen buffer */
        size_t screen_pixels = (wsys->screen.max.x - wsys->screen.min.x) *
                              (wsys->screen.max.y - wsys->screen.min.y);
        memset(wsys->screen_fb->data, 0xFF, screen_pixels * sizeof(uint32_t));
        
        /* Composite each window in order */
        for (uint32_t i = 0; i < wsys->nwindows; i++) {
            Window *w = wsys->windows[i];
            if (!w || !w->framebuffer->dirty) continue;
            
            pthread_mutex_lock(&w->lock);
            
            /* Copy window framebuffer to screen */
            int w_width = w->rect.max.x - w->rect.min.x;
            int w_height = w->rect.max.y - w->rect.min.y;
            int s_width = wsys->screen.max.x - wsys->screen.min.x;
            
            for (int y = 0; y < w_height; y++) {
                for (int x = 0; x < w_width; x++) {
                    int sx = w->rect.min.x + x;
                    int sy = w->rect.min.y + y;
                    
                    if (sx >= 0 && sx < s_width &&
                        sy >= 0 && sy < (wsys->screen.max.y - wsys->screen.min.y)) {
                        uint32_t pixel = w->framebuffer->data[y * w_width + x];
                        /* Simple alpha blending if needed */
                        wsys->screen_fb->data[sy * s_width + sx] = pixel;
                    }
                }
            }
            
            w->framebuffer->dirty = false;
            pthread_mutex_unlock(&w->lock);
        }
        
        /* Draw mouse cursor */
        if (wsys->mouse_pos.x >= 0 && wsys->mouse_pos.x < wsys->screen.max.x &&
            wsys->mouse_pos.y >= 0 && wsys->mouse_pos.y < wsys->screen.max.y) {
            /* Simple crosshair cursor */
            int s_width = wsys->screen.max.x - wsys->screen.min.x;
            for (int i = -10; i <= 10; i++) {
                if (wsys->mouse_pos.x + i >= 0 && wsys->mouse_pos.x + i < s_width) {
                    wsys->screen_fb->data[wsys->mouse_pos.y * s_width + wsys->mouse_pos.x + i] = 0xFF000000;
                }
                if (wsys->mouse_pos.y + i >= 0 && wsys->mouse_pos.y + i < wsys->screen.max.y) {
                    wsys->screen_fb->data[(wsys->mouse_pos.y + i) * s_width + wsys->mouse_pos.x] = 0xFF000000;
                }
            }
        }
        
        pthread_mutex_unlock(&wsys->lock);
        
        /* In real implementation, would blit screen_fb to actual display */
        /* For now, just sleep to control frame rate */
        usleep(16666);  /* ~60 FPS */
    }
    
    return NULL;
}

/* Initialize window system */
int wsys_init(void) {
    wsys = calloc(1, sizeof(WindowSystem));
    if (!wsys) return -ENOMEM;
    
    /* Create wsys directory */
    mkdir(WSYS_ROOT, 0755);
    
    /* Initialize screen (would get from hardware in real implementation) */
    wsys->screen.min.x = 0;
    wsys->screen.min.y = 0;
    wsys->screen.max.x = 1920;
    wsys->screen.max.y = 1080;
    
    /* Allocate screen framebuffer */
    size_t screen_size = 1920 * 1080 * sizeof(uint32_t);
    wsys->screen_fb = malloc(sizeof(Image));
    wsys->screen_fb->data = malloc(screen_size);
    wsys->screen_fb->r = wsys->screen;
    wsys->screen_fb->stride = 1920 * sizeof(uint32_t);
    
    pthread_mutex_init(&wsys->lock, NULL);
    
    /* Start compositor thread */
    wsys->running = true;
    pthread_create(&wsys->compositor, NULL, wsys_compositor_thread, NULL);
    
    /* Create root window */
    Rectangle root_rect = {
        .min = {0, 0},
        .max = {1920, 1080}
    };
    wsys_new_window("root", root_rect);
    
    return 0;
}

/* Example application using the window system */
void example_app(void) {
    /* Create a window by writing to /dev/wsys/new */
    int fd = open("/dev/wsys/new", O_RDWR);
    write(fd, "name hello\nsize 400 300\n", 24);
    
    char winid[32];
    read(fd, winid, sizeof(winid));
    close(fd);
    
    /* Open the window's draw file */
    char draw_path[256];
    snprintf(draw_path, sizeof(draw_path), "/dev/wsys/%s/draw", winid);
    int draw_fd = open(draw_path, O_WRONLY);
    
    /* Draw something */
    write(draw_fd, "clear FFFFFFFF\n", 15);
    write(draw_fd, "fillrect 50 50 350 250 FF0000FF\n", 33);
    write(draw_fd, "text 100 150 FF000000 Hello from 9P!\n", 38);
    
    close(draw_fd);
}