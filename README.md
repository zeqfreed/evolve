# Evolve

Evolve is a learning project to study different aspects of game programming and related topics.

The code is mostly written in C++, but only very limited subset of C++ features are used to keep
codebase simple and clear. Strictly OSX related parts are written in Objective-C.

Possibly outdated list of implemented features includes:

1. Minimal MacOS platform layer:
   * Window creation
   * Fixed frame rate game loop
   * Hot reloading of frame rendering module (courtesy of handmadehero.org)

2. Renderer hot reloadable module
   * Ad hoc loader of TGA textures and OBJ models

# Renderer

Features

   * "Programmable pipeline" with vertex and fragment shaders
   * Half-space triangle rasterization
   * Diffuse texturing
   * Normal mapping
   * Basic lighting
   * Shadow mapping

Example output

![Screenshot](screenshot.png?raw=true)

# License

The software is provided under the ISC License (see LICENSE.txt).

You're free to fork the code, or use otherwise according to the license.
Major changes to the code though are not likely to be merged into this repository,
as the project is primarily used for learning purposes.
