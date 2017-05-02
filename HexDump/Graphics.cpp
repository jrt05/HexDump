
#include "Graphics.h"
#include "Logger.h"
#include "Time.h"
#include "resource.h"
#include "Global.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#pragma warning (disable : 4996)

GFXs::GFXs() : errorMsg(""),
success(true) {

    create();
}

GFXs::~GFXs() {
    destroy();
}

void GFXs::changeWindowSize() {
    SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    //destroy();
    //create();
}

void GFXs::destroy() {
    SDL_GL_DeleteContext(context);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GFXs::create() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Logger::LogOut.logstream << Time::getTimeString << " SDL could not initialize. SDL Error: " << SDL_GetError() << std::endl;
        success = false;
        return;
    }

    // Create window
    window = SDL_CreateWindow("HexDump - " HEXDUMPVERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (window == NULL) {
        Logger::LogOut.logstream << Time::getTimeString << " SDL could not create window. SDL Error: " << SDL_GetError() << std::endl;
        success = false;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL) {
        Logger::LogOut.logstream << Time::getTimeString << " SDL could not create renderer. SDL Error: " << SDL_GetError() << std::endl;
        success = false;
        return;
    }

    // Enable windows events
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    // Initialize renderer color
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

    // Setup WinAPI
    SDL_VERSION(&wmInfo.version); /* initialize info structure with SDL version info */

    SDL_GetWindowWMInfo(window, &wmInfo);
    mHandle = wmInfo.info.win.window;    //   .window;
    mMenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MENU1));
    SetMenu(mHandle, mMenu);

    // Set drawable width and height
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");        // turn on nearest pixel sampling

    // Load BMPs
    readBMP("./SmallFont.bmp", smallfont);
    readBMP("./MediumFont.bmp", mediumfont);

    context = SDL_GL_CreateContext(window);
}

SDL_Texture *GFXs::getTexture(int w, int h) {
    SDL_Texture *retText;
    retText = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    return retText;
}

SDL_Texture *GFXs::getTextureFromBMP(char *filepath) {
    SDL_Surface *fileImage = NULL;
    SDL_Texture *retText;
    fileImage = SDL_LoadBMP(filepath);
    if (fileImage == NULL)
        return NULL;
    retText = SDL_CreateTextureFromSurface(renderer, fileImage);
    SDL_FreeSurface(fileImage);

    return retText;
}

void GFXs::commit() {
    SDL_RenderPresent(renderer);
    SDL_GL_SwapWindow(window);
    return;
}

void GFXs::clear() {
    // Clear Screen
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x63, 0xFF);
    SDL_RenderClear(renderer);
    return;
}

void GFXs::draw(SDL_Texture *t) {
    SDL_RenderCopy(renderer, t, NULL, NULL);
}

void GFXs::draw(SDL_Texture *t, int x, int y, int w, int h) {
    SDL_Rect rect;
    rect.w = w; rect.h = h;
    rect.x = x; rect.y = y;
    SDL_RenderCopy(renderer, t, NULL, &rect);
}

void GFXs::draw() {

    return;
}

void GFXs::buildString(std::string str, BMP &bmp, int fontsize) {
    BMP *font;
    if (fontsize == SMALLFONT) {
        font = &smallfont;
    }
    else if (fontsize == MEDIUMFONT) {
        font = &mediumfont;
    }
    else {
        return;
    }

    int charwidth = font->width / 16;
    int charheight = font->height / 16;

    int stringsize = (int)str.size();

    bmp.width = charwidth * stringsize + stringsize;
    bmp.height = charheight + 1;

    bmp.pixels = new Uint32[bmp.width * bmp.height * sizeof(Uint32)];
    for (size_t x = 0; x != bmp.width * bmp.height; ++x) {
        bmp.pixels[x] = BACKGROUND;
    }

    //memset(bmp.pixels, 255, bmp.width * bmp.height * sizeof(Uint32));
    // Move each character in the string
    for (size_t x = 0; x != stringsize; ++x) {
        int charint = (unsigned char)str[x];
        // Find first byte of the character we need
        int charloc = (charwidth * (charint % 16)) + (font->width * (charint / 16) * charheight);
        // move each row of a character
        for (int y = 0; y != charheight; ++y) {
            memcpy(bmp.pixels + x * (charwidth + 1) + y * bmp.width, font->pixels + charloc + y * font->width, charwidth * sizeof(Uint32));
            //bmp.pixels[x * (charwidth)]
        }
    }
}

void GFXs::readBMP(const char * filename, BMP &bmp) {
    FILE* f = fopen(filename, "rb");

    if (f == NULL) {
        Logger::LogOut.logstream << "Error: File " << filename << " could not be opened." << std::endl;
        exit(1);
    }
    //throw "Argument Exception";

    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

                                               // extract image height and width from header
    bmp.width = *(int*)&info[18];
    bmp.height = *(int*)&info[22];

    int row_padded = (bmp.width * 3 + 3) & (~3);
    unsigned char* data = new unsigned char[row_padded];
    bmp.pixels = new Uint32[bmp.width * bmp.height * sizeof(Uint32)];
    unsigned char tmp;
    //int x = pixel_width * pixel_height;
    int y = bmp.height - 1;
    for (int i = 0; i < bmp.height; i++, --y)
    {
        fread(data, sizeof(unsigned char), row_padded, f);
        int x = 0;
        for (int j = 0; j < bmp.width * 3; j += 3, ++x)
        {
            // Convert (B, G, R) to (R, G, B)
            tmp = data[j];
            data[j] = data[j + 2];
            data[j + 2] = tmp;
            bmp.pixels[x + y * bmp.width] = 0xFF000000 | (data[j] << 16) | (data[j + 1] << 8) | (data[j + 2]);
        }
    }
    delete data;
    fclose(f);
}