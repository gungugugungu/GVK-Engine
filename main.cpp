//
// Created by gungu on 4/5/26.
//
#define GVK_IMPLEMENTATION
#include "gvk.h"
#include <SDL3/SDL.h>

int main() {
    gvk::init();

    vector<shared_ptr<MeshAsset>> test_meshes = gvk::load_gltf_meshes("../test_monkey.glb").value();
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
        //ImGui::ShowDemoWindow();
        ImGui::Render();

        gvk::draw_mesh(test_meshes[2], custom_texture, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, glm::quat(glm::vec3(0.f, 0.f, 0.f)));

        gvk::draw();
    }

    gvk::quit();
    return 1;
}
