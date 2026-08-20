# GVK renderer
## What is it?
GVK (short for gungu vulkan) is my new renderer using the Vulkan graphics api. I miss the times when I worked with pygame and love2d, where rendering an image was one simple line of code, and I intend to bring that experience to 3D with this project.

Everything is designed to be as easy as possible for the user, just write 
```
gvk::draw_mesh(mesh, material, position, scale, rotation);
```
and the mesh is where you want it, how you want it.

But don't mistake it, this is not a game engine, this is just a renderer, something you can build an engine on. I made this tool so my later projects have a graphical backbone to build on, but every game needs a different engine.
## Features:
- Full 3D rendering capabilites
- ImGui
- Automatic mipmaps
- MSAA
- 2D rendering
- Post-processing
- Frustum culling
- Flexible skybox system with equirectangular skybox loading
- Material system
- Lighting with dynamic light count
- Physically-based rendering
- Animated and rigged meshes (mesh skinning)
- A cool demo
## How to try the demo
Go to the releases section on Github and download either windows.demo.zip or linux.demo.zip *(depending on your os)* under the DEMO release. Once downloaded, just extract the zip file, go in the bin folder and open GVK executable

**Make sure:**
- Your GPU supports Vulkan
- You have downloaded the latest graphics drivers for your GPU
- You have downloaded the latest [visual studio redistributables](https://aka.ms/vc14/vc_redist.x86.exe)

**Without these redistributables the project *can not* run at all**
## How to use
I mentioned how I want this project to be simple, and it hopefully is. 

Firstly, add GVK-Engine as a git submodule to your project, and add these lines to your CMakeLists.txt:
```
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/GVK-Engine) # add it as a subdirectory
add_executable([project] main.cpp)
target_link_libraries([project] PRIVATE gvk::gvk) # link the library
```
and you're done. Here's a simple main.cpp you can build on:
```
#define GVK_IMPLEMENTATION
#include "include/GVK-Engine/gvk.h"

int main() {
    relative_gvk_path = "../include/GVK-Engine"; // this is the hard coded value in the renderer, if your path is different, edit this value
    gvk::init();

    gvk::clear_color = {0.05f, 0.05f, 0.05f, 1.f};

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        // imgui goes here
        ImGui::Render();

        gvk::draw();
    }

    vkDeviceWaitIdle(gvk::_vk_device);

    gvk::quit();
    return 0;
}
```
## AI disclosure
This project was created so I could learn Vulkan, and if I copied everything from AI that would have been to effective. My goal was to use as little AI as I could, while still leaving out the headaches of development. Here's what I used AI for:
1. Many of the shaders were written by AI, but that's because I haven't learned any of the maths I need to know to write any of these shader. When a shader wasn't too math heavy I wrote it myself, but there weren't too many of those.
2. The GLTF loading isn't hard, it's just a headache to work with it. GLTF is a horrible format for developers, but it's the best I knew, so I rather had AI write the GLTF loading functions. I'm still glad I did that, I would have learned nothing from writing those.
## External projects used:
- [Vulkan](https://vulkan.lunarg.com/)
- [SDL3](https://github.com/libsdl-org/SDL)
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [glm](https://github.com/g-truc/glm)
- [fmt](https://github.com/fmtlib/fmt)
- [ImGui](https://github.com/ocornut/imgui)
- [stb](https://github.com/nothings/stb)
- [tinygltf](https://github.com/syoyo/tinygltf)