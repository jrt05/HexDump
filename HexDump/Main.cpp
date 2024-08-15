
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
        unsigned int start_of_loop = SDL_GetTicks();
        input->update();

        graphics->clear();

        dump->update();

        //graphics->draw();
        graphics->commit();

        unsigned int end_of_loop = SDL_GetTicks();
        unsigned int elapsed_time = end_of_loop - start_of_loop;
        if (elapsed_time < (1000 / 60.0)) {
            int sleep_time = (int) ((1000 / 60.0) - (end_of_loop - start_of_loop));
            Time::sleep(sleep_time);
            //SDL_Delay(sleep_time);
        }
    }

    delete graphics;
    delete input;
	delete dump;
    return 0;
}