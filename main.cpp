//
// Created by gungu on 4/5/26.
//
#define GVK_IMPLEMENTATION
#include "gvk.h"
#include <SDL3/SDL.h>

int main() {
    gvk::init();

    AllocatedImage custom_texture = gvk::load_image("../custom.jpg").value();

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            ImGui_ImplSDL3_ProcessEvent(&e);
        }
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        gvk::draw();
    }

    gvk::quit();
    return 1;
}
