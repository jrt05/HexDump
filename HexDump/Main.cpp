
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#include <Windows.h>

#include "Time.h"
#include "Logger.h"
#include "Graphics.h"
#include "InputHandler.h"
#include "Dump.h"

int SDL_main(int argc, char *argv[]) {
    Time::start();
    GFXs *graphics = new GFXs();

    if (!graphics->initialized()) {
        Logger::LogOut.logstream << Time::getTimeString() << " Error: Graphics unable to initialize." << std::endl;
        return 1;
    }

    InputHandler *input = new InputHandler();

    Dump *dump = new Dump(graphics, input);

    while (!input->quit()) {
        input->update();

        graphics->clear();

        dump->update();

        //graphics->draw();
        graphics->commit();
    }

    delete graphics;
    delete input;
    return 0;
}