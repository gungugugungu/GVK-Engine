//
// Created by gungu on 4/5/26.
//
#define GVK_IMPLEMENTATION
#include "gvk.h"
#include <SDL3/SDL.h>

/*
TODO FEATURES:
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

    vector<shared_ptr<gvk::MeshAsset>> test_meshes = gvk::load_gltf_meshes("../test_monkey.glb").value();

    // skinned mesh
    gvk::SkinnedGLTFData _gordon_freeman_returns = gvk::load_gltf_meshes_skinned("../dancing.glb").value();
    gvk::SkinnedMeshAsset* gordon_freeman = _gordon_freeman_returns.meshes[0].get();
    gordon_freeman->skin = &_gordon_freeman_returns.skins[0];

    gvk::SkinnedInstance instance;
    instance.asset = gordon_freeman;
    if (!_gordon_freeman_returns.animations.empty()) {
        instance.clip = &_gordon_freeman_returns.animations[0];
    } else {
        instance.clip = nullptr;
    }
    instance.current_time = 0.f;
    instance.joint_matrices.resize(_gordon_freeman_returns.skins[0].joint_count);

    gvk::AllocatedImage monkey_texture = gvk::load_image("../custom.png").value();
    gvk::AllocatedImage water_normal = gvk::load_image("../water normal.jpg").value();

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

    gvk::Material monkeymat = gvk::create_material(monkey_texture, water_normal, gvk::_white_image, gvk::_black_image, gvk::_black_image, gvk::_black_image);
    gvk::Material teapotmat = gvk::create_material(gvk::_error_checkerboard_image, water_normal, gvk::_white_image, gvk::_black_image, gvk::_black_image, gvk::_black_image);

    gvk::GLTFReturns scene1 = gvk::load_gltf_scene("../scene.glb").value();
    for (auto pl : scene1.point_lights) {
        gvk::point_lights.push_back(pl);
    }
    for (auto sl : scene1.spot_lights) {
        gvk::spot_lights.push_back(sl);
    }
    if (&scene1.dir_light != nullptr) {
        gvk::directional_light = scene1.dir_light;
    }

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

        update_animation(instance, dt);

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

        if (ImGui::CollapsingHeader("LIGHTS")) {
            ImGui::BeginChild("LIGHTSIES");
            if (ImGui::CollapsingHeader("AMBIENT LIGHT")) {
                float colorthingy[3] = {gvk::ambient_light.color.x, gvk::ambient_light.color.y, gvk::ambient_light.color.z};
                ImGui::ColorPicker3("COLOR", colorthingy);
                gvk::ambient_light.color.x = colorthingy[0];
                gvk::ambient_light.color.y = colorthingy[1];
                gvk::ambient_light.color.z = colorthingy[2];
                ImGui::SliderFloat("INTENSITY", &gvk::ambient_light.intensity, 0.f, 20.f);
            }
            if (ImGui::CollapsingHeader("DIRECTIONAL LIGHT")) {
                float directionthingy[3] = {gvk::directional_light.direction.x, gvk::directional_light.direction.y, gvk::directional_light.direction.z};
                ImGui::SliderFloat3("DIRECTION", directionthingy, -1.f, 1.f);
                gvk::directional_light.direction.x = directionthingy[0];
                gvk::directional_light.direction.y = directionthingy[1];
                gvk::directional_light.direction.z = directionthingy[2];
                float colorthingy[3] = {gvk::directional_light.color.x, gvk::directional_light.color.y, gvk::directional_light.color.z};
                ImGui::ColorPicker3("COLOR", colorthingy);
                gvk::directional_light.color.x = colorthingy[0];
                gvk::directional_light.color.y = colorthingy[1];
                gvk::directional_light.color.z = colorthingy[2];
                ImGui::SliderFloat("INTENSITY", &gvk::directional_light.intensity, 0.f, 20.f);
            }
            if (ImGui::CollapsingHeader("POINT LIGHTS"))
            {
                static int selected_point = -1;

                ImGui::BeginChild("POINT LIGHTS LIST", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 220), true);
                {
                    for (int i = 0; i < (int)gvk::point_lights.size(); ++i)
                    {
                        char label[64];
                        snprintf(label, sizeof(label), "POINT LIGHT %d", i);

                        if (ImGui::Selectable(label, selected_point == i))
                            selected_point = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("POINT LIGHT EDITOR", ImVec2(0, 220), true);
                {
                    if (selected_point >= 0 && selected_point < (int)gvk::point_lights.size())
                    {
                        auto& light = gvk::point_lights[selected_point];

                        float pos[3] = { light.position.x, light.position.y, light.position.z };
                        if (ImGui::DragFloat3("POSITION", pos, 0.1f))
                        {
                            light.position = { pos[0], pos[1], pos[2] };
                        }

                        float col[3] = { light.color.x, light.color.y, light.color.z };
                        if (ImGui::ColorEdit3("COLOR", col))
                        {
                            light.color = { col[0], col[1], col[2] };
                        }

                        ImGui::DragFloat("RANGE", &light.range, 0.1f, 0.1f, 1000.f);
                        ImGui::DragFloat("INTENSITY", &light.intensity, 0.05f, 0.0f, 100.f);

                        if (ImGui::Button("DELETE"))
                        {
                            gvk::point_lights.erase(gvk::point_lights.begin() + selected_point);
                            selected_point = -1;
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No point light selected");
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("ADD POINT LIGHT"))
                {
                    gvk::PointLight l;
                    gvk::point_lights.push_back(l);
                    selected_point = (int)gvk::point_lights.size() - 1;
                }
            }

            if (ImGui::CollapsingHeader("SPOT LIGHTS"))
            {
                static int selected_spot = -1;

                ImGui::BeginChild("SPOT LIGHT LIST", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 220), true);
                {
                    for (int i = 0; i < (int)gvk::spot_lights.size(); ++i)
                    {
                        char label[64];
                        snprintf(label, sizeof(label), "SPOT LIGHT %d", i);

                        if (ImGui::Selectable(label, selected_spot == i))
                            selected_spot = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("SPOT LIGHT EDITOR", ImVec2(0, 220), true);
                {
                    if (selected_spot >= 0 && selected_spot < (int)gvk::spot_lights.size())
                    {
                        auto& light = gvk::spot_lights[selected_spot];

                        float pos[3] = { light.position.x, light.position.y, light.position.z };
                        if (ImGui::DragFloat3("POSITION", pos, 0.1f))
                        {
                            light.position = { pos[0], pos[1], pos[2] };
                        }

                        float dir[3] = { light.direction.x, light.direction.y, light.direction.z };
                        if (ImGui::DragFloat3("DIRECTION", dir, 0.01f, -1.f, 1.f))
                        {
                            light.direction = glm::normalize(glm::vec3{ dir[0], dir[1], dir[2] });
                        }

                        float col[3] = { light.color.x, light.color.y, light.color.z };
                        if (ImGui::ColorEdit3("COLOR", col))
                        {
                            light.color = { col[0], col[1], col[2] };
                        }

                        ImGui::DragFloat("RANGE", &light.range, 0.1f, 0.1f, 1000.f);
                        ImGui::DragFloat("INTENSITY", &light.intensity, 0.05f, 0.0f, 100.f);

                        if (ImGui::Button("DELETE"))
                        {
                            gvk::spot_lights.erase(gvk::spot_lights.begin() + selected_spot);
                            selected_spot = -1;
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No spot light selected");
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("ADD SPOTLIGHT"))
                {
                    gvk::SpotLight l;
                    gvk::spot_lights.push_back(l);
                    selected_spot = (int)gvk::spot_lights.size() - 1;
                }
            }
            ImGui::EndChild();
        }

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
        // RENDER
        ImGui::Render();

        /*
        for (auto inst : monkeyinstances) {
            gvk::draw_mesh(test_meshes[0], monkeymat, inst.pos, inst.scale, glm::quat(inst.rot));
        }
        for (auto inst : teapotinstances) {
            gvk::draw_mesh(test_meshes[1], teapotmat, inst.pos, inst.scale, glm::quat(inst.rot));
        }
        */

        for (auto& mesh : scene1.meshes) {
            gvk::draw_mesh(&mesh.mesh, mesh.material, mesh.position, mesh.scale, mesh.rot);
        }

        gvk::draw_skinned_mesh(instance, _gordon_freeman_returns.materials[0], {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {1.f, 0.f, 0.f, 0.f});

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