# GVK renderer
## What is it?
GVK (short for gungu vulkan) is my new renderer using the Vulkan graphics api. I miss the times when I worked with pygame and love2d, where rendering an image was one simple line of code, and I intend to bring that experience to 3D with this project.

Everything is designed to be as easy as possible for the user, just 
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