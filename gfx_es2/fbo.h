#pragma once

// Simple wrapper around FBO functionality.

struct FBO;

// Creates a simple FBO with a RGBA32 color buffer stored in a texture, and
// optionally an accompanying Z/stencil buffer.
// No mipmap support.
// num_color_textures must be 1 for now.
// you lose bound texture state.
FBO *fbo_create(int width, int height, int num_color_textures, bool z_stencil);

// These functions should be self explanatory.
void fbo_bind_as_render_target(FBO *fbo);
// color must be 0, for now.
void fbo_bind_color_as_texture(FBO *fbo, int color);
void fbo_bind_for_read(FBO *fbo);
void fbo_unbind();
void fbo_destroy(FBO *fbo);
