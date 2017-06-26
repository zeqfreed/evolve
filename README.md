# Evolve

Evolve is a learning project to study different aspects of game programming and related topics.

The code is mostly written in C++, but only very limited subset of C++ features are used to keep
codebase simple and clear. Strictly OSX related parts are written in Objective-C.

Possibly outdated list of implemented features includes:

1. Minimal MacOS platform layer
   * Window creation
   * Game loop
   * Hot reloading of shared executable modules

2. 3D rendering facilities
   * "Programmable pipeline" with vertex and fragment functions
   * Half-space triangle rasterization

2. Cubes module
   * Renders a 10x10x10 grid of textured cubes to assess renderer performance
   * Lazy and poor font rendering demonstration

3. Viewer module
   * Loading of TGA textures and OBJ models
   * Loading and animation of M2 models (Vanilla WoW)
   * Perspective-correct attribute interpolation, normal & shadow mapping, basic lighting in shader functions

# Example output

![Screenshot](screenshot.png?raw=true)

# License

The software is provided under the ISC License (see LICENSE.txt).

You're free to fork the code, or use otherwise according to the license.
Major changes to the code though are not likely to be merged into this repository,
as the project is primarily used for learning purposes.
