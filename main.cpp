//
// Created by gungu on 4/5/26.
//
#define GVK_IMPLEMENTATION
#include "gvk.h"
#include <SDL3/SDL.h>

/*
TODO FEATURES:
 - 8. lighting
    - gpu scene data uniform buffer
    - ambient light
    - directional light
    - point lights
    - tangent space normal mapping
    - tangent generation in gltf loading
    - dynamic light count
    - proper api for handling lights
 - 9. shadow mapping
    - depth-only pipeline
    - shadow map image
    - light space matrix
    - shadow pass
    - shadow sampling in the main fragment shader
    - shadow bias
    - PCF filtering
 - 10. instanced rendering
    - per-instance storage buffer
    - draw_mesh_instanced api
    - write the shader for it (gl_InstanceIndex)
    - move to indirect drawing so the instance count can eventually come from gpu-side data
*/

struct posdata {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

float frand()
{
    return (float)(rand()) / (float)(RAND_MAX);
}

int randomInt(int a, int b)
{
    if (a > b)
        return randomInt(b, a);
    if (a == b)
        return a;
    return a + (rand() % (b - a));
}

float frand(int a, int b)
{
    if (a > b)
        return frand(b, a);
    if (a == b)
        return a;

    return static_cast<float>(randomInt(a, b)) + frand();
}

void create_posdatas(vector<posdata>& john, int amount) {
    for (int i = 0; i<amount; i++) {
        john.push_back({.pos = {frand(-50.f, 50.f), frand(-50.f, 50.f), frand(-50.f, 50.f)}, .rot = {frand(-6.2832f, 6.2832f), frand(-6.2832f, 6.2832f), frand(-6.2832f, 6.2832f)}, .scale = {frand(1.0f, 2.f), frand(1.0f, 2.f), frand(1.0f, 2.f)}});
    }
}

int main() {
    gvk::init();
    srand (static_cast <unsigned> (time(0)));

    vector<shared_ptr<MeshAsset>> test_meshes = gvk::load_gltf_meshes("../test_monkey.glb").value();

    AllocatedImage monkey_texture = gvk::load_image("../custom.png").value();
    AllocatedImage water_normal = gvk::load_image("../water normal.jpg").value();

    gvk::Surface leclerc_surface;
    leclerc_surface.load_from_file("../custom.jpg");

    gvk::load_skybox("../textures/skyboxes/night.png");

    gvk::clear_color = {0.05f, 0.05f, 0.05f, 1.f};

    float yaw = 90.f;
    float pitch = 0.f;
    const float sensitivity = 0.1f;
    const float speed = 5.f;
    bool mouse_captured = false;
    bool rmb_down = false;

    Uint64 last_time = SDL_GetTicks();

    // instanced meshes demo
    vector<posdata> monkeyinstances;
    vector<posdata> teapotinstances;

    create_posdatas(monkeyinstances, 100);
    create_posdatas(teapotinstances, 100);

    Material monkeymat = gvk::create_material(monkey_texture, water_normal, gvk::_white_image, gvk::_black_image, gvk::_black_image, gvk::_black_image);
    Material teapotmat = gvk::create_material(gvk::_error_checkerboard_image, water_normal, gvk::_white_image, gvk::_black_image, gvk::_black_image, gvk::_black_image);

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
        if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_E]) gvk::camera.position += up * speed * dt;
        if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_Q]) gvk::camera.position -= up * speed * dt;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Tonemapping variables");

        if (ImGui::CollapsingHeader("COLOR GRADING")) {
            ImGui::Checkbox("Tonemapping", &gvk::main_post_processing_stack.tonemapping_enabled);
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
        }

        if (ImGui::CollapsingHeader("POST-PROCESSING EFFECTS")) {
            ImGui::Checkbox("Bloom", &gvk::main_post_processing_stack.bloom_enabled);
            ImGui::SliderFloat("Bloom intensity", &gvk::main_post_processing_stack.bloom_intensity, 0.f, 3.f);
            ImGui::SliderFloat("Bloom threshold", &gvk::main_post_processing_stack.bloom_filter_threshold, 0.5f, 1.5f);
            ImGui::SliderFloat("Bloom knee", &gvk::main_post_processing_stack.bloom_filter_knee, 0.f, 1.f);
            ImGui::SliderInt("Bloom blur passes", &gvk::main_post_processing_stack.bloom_blur_passes, 0, 20);
            ImGui::Checkbox("Vignette", &gvk::main_post_processing_stack.vignette_enabled);
            ImGui::SliderFloat("Vignette radius", &gvk::main_post_processing_stack.vignette_radius, 0.f, 1.f);
            ImGui::SliderFloat("Vignette strength", &gvk::main_post_processing_stack.vignette_strength, 0.f, 1.f);
        }

        ImGui::End();
        ImGui::Render();

        for (auto inst : monkeyinstances) {
            gvk::draw_mesh(test_meshes[0], monkeymat, inst.pos, inst.scale, glm::quat(inst.rot));
        }
        for (auto inst : teapotinstances) {
            gvk::draw_mesh(test_meshes[1], teapotmat, inst.pos, inst.scale, glm::quat(inst.rot));
        }

        gvk::draw();
    }

    vkDeviceWaitIdle(gvk::_vk_device);

    for (auto& mesh : test_meshes) {
        gvk::destroy_buffer(mesh->mesh_buffers.vertex_buffer);
        gvk::destroy_buffer(mesh->mesh_buffers.index_buffer);
    }
    gvk::destroy_image(monkey_texture);
    gvk::destroy_image(water_normal);

    gvk::quit();
    return 0;
}