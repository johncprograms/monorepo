# project overview
This is a bunch of experimental rendering code, most of it rewrites of older code from my days studying computer graphics at college.

# source overview
- [src/main_rt.cpp](src/main_rt.cpp) -- entry point and app-level code.
- [src/render.h](src/render.h) -- contains the texture sampling code, geometry transformation / projection pipeline code, rasterization code, and ray-tracing / path-tracing code. Some of it SIMD-ized, since these are all CPU implementations, and that's part of how you go fast on a CPU.
