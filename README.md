# Evolve

Evolve is a learning project to study different aspects of game programming and related topics.

The code is mostly written in C++, but only very limited subset of C++ features are used to keep
codebase simple and clear. Strictly OSX related parts are written in Objective-C.

Possibly outdated list of implemented features includes:
1. Minimal MacOS platform layer:
   * Window creation
   * Fixed frame rate game loop
   * Hot reloading of frame rendering module (courtesy of handmadehero.org)

2. Zero dependency (bar a few standard C++ headers) 3d renderer:
   * Implemented as hot-reloadable dylib
   * Ad hoc TGA loader
   * Ad hoc OBJ loader
   * Rendering of loaded 3d model with diffuse texture, basic lighting, and smooth shading

# License

The software is provided under the ISC License (see LICENSE.txt).

You're free to fork the code, or use otherwise according to the license.
Major changes to the code though are not likely to be merged into this repository,
as the project is primarily used for learning purposes.
