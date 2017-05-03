#pragma once
#include "Logger.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <string>

class BMP {
public:
    BMP() { pixels = NULL; width = 0; height = 0; }
    BMP(int w, int h) { pixels = new Uint32[w * h]; width = w; height = h; }
    ~BMP() { if (pixels != NULL) delete pixels; pixels = NULL; }
    Uint32 *pixels;
    int width;
    int height;
};

//namespace Graphics {
class GFXs {
public:
    enum {
        SMALLFONT,
        MEDIUMFONT,
        BITFONT,
        NATURALFONT
    };
    //const Uint32 BACKGROUND = 0xFF000063;
    const Uint32 BACKGROUND = 0xFF000000;
    GFXs();
    ~GFXs();

    void changeWindowSize();

    void setFont(int font);

    void draw();
    void draw(SDL_Texture *t, int x, int y, int w, int h);
    void draw(SDL_Texture *t);
    void clear();
    void commit();

    bool initialized() { return success; }
    void buildString(std::string s, BMP &b, int fontsize);
    SDL_Texture *getTextureFromBMP(char *);
    SDL_Texture *getTexture(int w, int h);
    //SDL_Window *getWindow() { return window; }
    HMENU *getMenu() { return &mMenu; }

    int getWidth() { return drawableWidth; }
    int getHeight() { return drawableHeight; }

private:
    int drawableWidth;
    int drawableHeight;
    const unsigned int SCREEN_WIDTH = 1024;
    const unsigned int SCREEN_HEIGHT = 768;
    int currentGraphicsMode;
    void readBMP(const char *, BMP &b);
    void destroy();
    void create();
    SDL_Window *window;     // Create window
    SDL_Renderer *renderer; // Window renderer
    SDL_GLContext context;

    std::string errorMsg;
    bool success;
    const Uint32 clearNum = 0xFF6496C8;

    SDL_SysWMinfo wmInfo;
    HWND mHandle;
    HMENU mMenu;

    BMP smallfont;
    BMP mediumfont;
};

