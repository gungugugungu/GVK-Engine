# GVK renderer
## What is it?
GVK (short for gungu vulkan) is my new renderer using the Vulkan graphics api. I miss the times when I worked with pygame and love2d, where rendering an image was one simple line of code, and I intend to bring that experience to 3D with this project.

Everything is designed to be as easy as possible for the user, just 
```
gvk::draw_mesh(mesh, material, position, scale, rotation);
```
and the mesh is where you want it, how you want it.

But don't mistake it, this is not a game engine, this is just a renderer, something you can build an engine on. I made this tool

## Features:
- Full 3D rendering capabilites
- ImGui
- Automatic mipmaps
- MSAA
- 2D rendering
- Post-processing
- Frustum culling
- Flexible skybox system with equirectangular skybox loading

## To-do:
- Instanced rendering
- Material system
- Lighting with dynamic light count
- Physically-based rendering
- Shadowmapping

## External projects used:
- [Vulkan](https://vulkan.lunarg.com/)
- [SDL3](https://github.com/libsdl-org/SDL)
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [glm](https://github.com/g-truc/glm)
- [fmt](https://github.com/fmtlib/fmt)
- [ImGui](https://github.com/ocornut/imgui)
- [stb](https://github.com/nothings/stb)
- [tinygltf](https://github.com/syoyo/tinygltf)