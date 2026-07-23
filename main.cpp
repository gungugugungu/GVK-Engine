//
// Created by gungu on 4/5/26.
//
#define GVK_IMPLEMENTATION
#include "gvk.h"
#include <SDL3/SDL.h>

/*
TODO FEATURES:
 - 9. lighting
    - gpu scene data uniform buffer
    - ambient light
    - directional light
    - point lights
    - tangent space normal mapping
    - tangent generation in gltf loading
    - dynamic light count
    - proper api for handling lights
 - 10. shadow mapping
    - depth-only pipeline
    - shadow map image
    - light space matrix
    - shadow pass
    - shadow sampling in the main fragment shader
    - shadow bias
    - PCF filtering
 - 8. material system
    - material struct (albedo map, normal map, roughness map, emissive map, scalar tint and roughness and metallic factors)
    - material descriptor layout
    - default fallback textures
    - gltf material loading
    - draw_mesh api update
    - emissive support in the shader
    - creating and destroying materials for user-made materials
 - 7. instanced rendering
    - per-instance storage buffer
    - draw_mesh_instanced api
    - write the shader for it (gl_InstanceIndex)
    - move to indirect drawing so the instance count can eventually come from gpu-side data
*/

int main() {
    gvk::init();

    vector<shared_ptr<MeshAsset>> test_meshes = gvk::load_gltf_meshes("../test_monkey.glb").value();
    AllocatedImage custom_texture = gvk::load_image("../custom.png").value();
    gvk::Surface leclerc_surface;
    leclerc_surface.load_from_file("../custom.jpg");
    leclerc_surface.refresh();

    gvk::display.clear(1920, 1080, {1, 1, 1, 0});
    gvk::display.draw(leclerc_surface, {16, 16});
    gvk::display.refresh();

    gvk::load_skybox("../textures/skyboxes/night.png");

    gvk::clear_color = {0.05f, 0.05f, 0.05f, 1.f};

    float yaw = 90.f;
    float pitch = 0.f;
    const float sensitivity = 0.1f;
    const float speed = 5.f;
    bool mouse_captured = false;
    bool rmb_down = false;

    Uint64 last_time = SDL_GetTicks();

    bool running = true;
    while (running) {
        Uint64 now = SDL_GetTicks();
        float dt = (float)(now - last_time) / 1000.f;
        last_time = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == SDL_BUTTON_LEFT && !mouse_captured) {
                    mouse_captured = true;
                    SDL_SetWindowRelativeMouseMode(gvk::window, true);
                }
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    rmb_down = true;
                    SDL_SetWindowRelativeMouseMode(gvk::window, true);
                    SDL_HideCursor();
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    rmb_down = false;
                    SDL_SetWindowRelativeMouseMode(gvk::window, false);
                    SDL_ShowCursor();
                }
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                mouse_captured = false;
                SDL_SetWindowRelativeMouseMode(gvk::window, false);
            }

            if (e.type == SDL_EVENT_MOUSE_MOTION && mouse_captured) {
                yaw += e.motion.xrel * sensitivity;
                pitch -= e.motion.yrel * sensitivity;
                pitch = glm::clamp(pitch, -89.f, 89.f);
            }
        }

        if (rmb_down) {
            glm::vec3 dir;
            dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            dir.y = sin(glm::radians(pitch));
            dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            gvk::camera.direction = glm::normalize(dir);
        }

        glm::vec3 forward = gvk::camera.direction;
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3{0.f, 1.f, 0.f}));
        glm::vec3 up = glm::vec3{0.f, 1.f, 0.f};

        auto* keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_W]) gvk::camera.position += forward * speed * dt;
        if (keys[SDL_SCANCODE_S]) gvk::camera.position -= forward * speed * dt;
        if (keys[SDL_SCANCODE_A]) gvk::camera.position -= right * speed * dt;
        if (keys[SDL_SCANCODE_D]) gvk::camera.position += right * speed * dt;
        if (keys[SDL_SCANCODE_SPACE]) gvk::camera.position += up * speed * dt;
        if (keys[SDL_SCANCODE_LCTRL]) gvk::camera.position -= up * speed * dt;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Tonemapping variables");

        ImGui::SliderFloat("Exposure", &gvk::main_post_processing_stack.tonemap_values.exposure, 0.5f, 2.f);
        ImGui::SliderFloat("Temp", &gvk::main_post_processing_stack.tonemap_values.temp, -1.f, 1.f);
        ImGui::SliderFloat("Tint", &gvk::main_post_processing_stack.tonemap_values.tint, -1.f, 1.f);
        ImGui::SliderFloat("Contrast", &gvk::main_post_processing_stack.tonemap_values.contrast, 0.5f, 1.5f);
        ImGui::SliderFloat("Saturation", &gvk::main_post_processing_stack.tonemap_values.saturation, 0.5f, 1.5f);
        ImGui::SliderFloat("Highlights", &gvk::main_post_processing_stack.tonemap_values.highlights, -0.5f, 0.5f);
        ImGui::SliderFloat("Midtones", &gvk::main_post_processing_stack.tonemap_values.midtones, -0.5f, 0.5f);
        ImGui::SliderFloat("Shadows", &gvk::main_post_processing_stack.tonemap_values.shadows, -0.5f, 0.5f);
        ImGui::SliderFloat("Whites", &gvk::main_post_processing_stack.tonemap_values.whites, -1.0f, 1.0f);
        ImGui::SliderFloat("Blacks", &gvk::main_post_processing_stack.tonemap_values.blacks, -0.1f, 0.1f);
        ImGui::SliderFloat("Vibrance", &gvk::main_post_processing_stack.tonemap_values.vibrance, 0.5f, 1.5f);
        if (ImGui::Button("Reinhard")) gvk::main_post_processing_stack.tonemap_values.op = 0;
        ImGui::SameLine();
        if (ImGui::Button("Uncharted 2")) gvk::main_post_processing_stack.tonemap_values.op = 1;
        ImGui::SameLine();
        if (ImGui::Button("ACES")) gvk::main_post_processing_stack.tonemap_values.op = 2;

        ImGui::End();
        ImGui::Render();

        gvk::draw_mesh(test_meshes[0], custom_texture, {-2.5f, 0.f, 0.f}, {1, 1, 1}, glm::quat(glm::vec3(0, 3.1416, 0)));
        gvk::draw_mesh(test_meshes[1], gvk::_error_checkerboard_image, {2.5f, -1.f, 0.f}, {1, 1, 1}, glm::quat(glm::vec3(1.5708, 0, 0)));

        gvk::draw();
    }

    vkDeviceWaitIdle(gvk::_vk_device);

    for (auto& mesh : test_meshes) {
        gvk::destroy_buffer(mesh->mesh_buffers.vertex_buffer);
        gvk::destroy_buffer(mesh->mesh_buffers.index_buffer);
    }
    gvk::destroy_image(custom_texture);

    gvk::quit();
    return 0;
}