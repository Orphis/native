#ifdef ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif

#include <cmath>
#include "base/logging.h"
#include "math/math_util.h"
#include "gfx_es2/draw_buffer.h"
#include "gfx_es2/glsl_program.h"
#include "gfx/texture_atlas.h"
#include "gfx/gl_debug_log.h"

enum {
  // Enough?
  MAX_VERTS = 5000,
};

DrawBuffer::DrawBuffer() : count_(0) {
  verts_ = new Vertex[MAX_VERTS];
  fontscalex = 1.0f;
  fontscaley = 1.0f;
}
DrawBuffer::~DrawBuffer() {
  delete [] verts_;
}

void DrawBuffer::Begin(DrawBufferMode dbmode) {
  count_ = 0;
  mode_ = dbmode;
}

void DrawBuffer::End() {
	// Currently does nothing, but call it!
}

void DrawBuffer::Flush(const GLSLProgram *program, bool set_blend_state) {
  if (count_ == 0)
    return;
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (set_blend_state) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
  glUniform1i(program->sampler0, 0);
  glEnableVertexAttribArray(program->a_position);
  glEnableVertexAttribArray(program->a_color);
  if (program->a_texcoord0 != -1)
    glEnableVertexAttribArray(program->a_texcoord0);
  GL_CHECK();
  glVertexAttribPointer(program->a_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &verts_[0].x);
  glVertexAttribPointer(program->a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &verts_[0].r);
  if (program->a_texcoord0 != -1)
    glVertexAttribPointer(program->a_texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &verts_[0].u);
  glDrawArrays(mode_ == DBMODE_LINES ? GL_LINES : GL_TRIANGLES, 0, count_);
  GL_CHECK();
  glDisableVertexAttribArray(program->a_position);
  glDisableVertexAttribArray(program->a_color);
  if (program->a_texcoord0 != -1)
    glDisableVertexAttribArray(program->a_texcoord0);
  GL_CHECK();

	count_ = 0;
}

void DrawBuffer::V(float x, float y, float z, uint32 color, float u, float v) {
	Vertex *vert = &verts_[count_++];
  vert->x = x;
  vert->y = y;
  vert->z = z;
	// todo: speedup rgba here
  vert->r = color & 0xFF;
  vert->g = (color >> 8) & 0xFF;
  vert->b = (color >> 16) & 0xFF;
  vert->a = (color >> 24) & 0xFF;
  vert->u = u;
  vert->v = v;
}

void DrawBuffer::Rect(float x, float y, float w, float h, uint32 color, int align) {
	DoAlign(align, &x, &y, w, h);
  RectVGradient(x, y, w, h, color, color);
}

void DrawBuffer::RectVGradient(float x, float y, float w, float h, uint32 colorTop, uint32 colorBottom) {
	V(x,     y,     0, colorTop,    0, 0);
	V(x + w, y,     0, colorTop,    1, 0);
	V(x + w, y + h, 0, colorBottom, 1, 1);
	V(x,     y,     0, colorTop,    0, 0);
	V(x + w, y + h, 0, colorBottom, 1, 1);
	V(x,     y + h, 0, colorBottom, 0, 1);
}

void DrawBuffer::MultiVGradient(float x, float y, float w, float h, GradientStop *stops, int numStops) {
	for (int i = 0; i < numStops - 1; i++) {
		float t0 = stops[i].t, t1 = stops[i+1].t;
		uint32_t c0 = stops[i].t, c1 = stops[i+1].t;
		RectVGradient(x, y + h * t0, w, h * (t1 - t0), c0, c1);
	}
}

void DrawBuffer::Rect(float x, float y, float w, float h,
                      float u, float v, float uw, float uh,
                      uint32 color) {
  V(x,     y,     0, color, u, v);
  V(x + w, y,     0, color, u + uw, v);
  V(x + w, y + h, 0, color, u + uw, v + uh);
  V(x,     y,     0, color, u, v);
  V(x + w, y + h, 0, color, u + uw, v + uh);
  V(x,     y + h, 0, color, u, v + uh);
}

void DrawBuffer::MeasureImage(int atlas_image, float *w, float *h) {
  const AtlasImage &image = atlas->images[atlas_image];
  *w = (float)image.w;
  *h = (float)image.h;
}

void DrawBuffer::DrawImage(int atlas_image, float x, float y, float scale, Color color, int align) {
  const AtlasImage &image = atlas->images[atlas_image];
  float w = (float)image.w * scale;
  float h = (float)image.h * scale;
	if (align & ALIGN_HCENTER) x -= w / 2;
	if (align & ALIGN_RIGHT) x -= w;
	if (align & ALIGN_VCENTER) y -= h / 2;
	if (align & ALIGN_BOTTOM) y -= h;
  DrawImageStretch(atlas_image, x, y, x + w, y + h, color);
}

void DrawBuffer::DrawImageStretch(int atlas_image, float x1, float y1, float x2, float y2, Color color) {
  const AtlasImage &image = atlas->images[atlas_image];
  V(x1,  y1, color, image.u1, image.v1);
  V(x2,  y1, color, image.u2, image.v1);
  V(x2,  y2, color, image.u2, image.v2);
  V(x1,  y1, color, image.u1, image.v1);
  V(x2,  y2, color, image.u2, image.v2);
  V(x1,  y2, color, image.u1, image.v2);
}

// TODO: add arc support
void DrawBuffer::Circle(float xc, float yc, float radius, float thickness, int segments, float startAngle, uint32 color, float u_mul) {
	float angleDelta = PI * 2 / segments;
	float uDelta = 1.0f / segments;
	float t2 = thickness / 2.0f;
	float r1 = radius + t2;
	float r2 = radius - t2;
	for (int i = 0; i < segments + 1; i++) {
		float angle1 = i * angleDelta;
		float angle2 = (i + 1) * angleDelta;
		float u1 = u_mul * i * uDelta;
		float u2 = u_mul * (i + 1) * uDelta;
		// TODO: get rid of one pair of cos/sin per loop, can reuse from last iteration
		float c1 = cosf(angle1), s1 = sinf(angle1), c2 = cosf(angle2), s2 = sinf(angle2);
		const float x[4] = {c1 * r1 + xc, c2 * r1 + xc, c1 * r2 + xc, c2 * r2 + xc};
		const float y[4] = {s1 * r1 + yc, s2 * r1 + yc, s1 * r2 + yc, s2 * r2 + yc};
		V(x[0],  y[0], color, u1, 0);
		V(x[1],  y[1], color, u2, 0);
		V(x[2],  y[2], color, u1, 1);
		V(x[1],  y[1], color, u2, 0);
		V(x[3],  y[3], color, u2, 1);
		V(x[2],  y[2], color, u1, 1);
	}
}

void DrawBuffer::DrawTexRect(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, Color color) {
  V(x1,  y1, color, u1, v1);
  V(x2,  y1, color, u2, v1);
  V(x2,  y2, color, u2, v2);
  V(x1,  y1, color, u1, v1);
  V(x2,  y2, color, u2, v2);
  V(x1,  y2, color, u1, v2);
}

void DrawBuffer::DrawImage4Grid(int atlas_image, float x1, float y1, float x2, float y2, Color color, float corner_scale) {
  const AtlasImage &image = atlas->images[atlas_image];

  float um = (image.u2 - image.u1) * 0.5f;
  float vm = (image.v2 - image.v1) * 0.5f;
  float iw2 = (image.w * 0.5f) * corner_scale;
  float ih2 = (image.h * 0.5f) * corner_scale;
  float xa = x1 + iw2;
  float xb = x2 - iw2;
  float ya = y1 + ih2;
  float yb = y2 - ih2;
  float u1 = image.u1, v1 = image.v1, u2 = image.u2, v2 = image.v2;
  // Top row
  DrawTexRect(x1, y1, xa, ya, u1, v1, um, vm, color);
  DrawTexRect(xa, y1, xb, ya, um, v1, um, vm, color);
  DrawTexRect(xb, y1, x2, ya, um, v1, u2, vm, color);
  // Middle row
  DrawTexRect(x1, ya, xa, yb, u1, vm, um, vm, color);
  DrawTexRect(xa, ya, xb, yb, um, vm, um, vm, color);
  DrawTexRect(xb, ya, x2, yb, um, vm, u2, vm, color);
  // Bottom row
  DrawTexRect(x1, yb, xa, y2, u1, vm, um, v2, color);
  DrawTexRect(xa, yb, xb, y2, um, vm, um, v2, color);
  DrawTexRect(xb, yb, x2, y2, um, vm, u2, v2, color);
}

void DrawBuffer::DrawImage2GridH(int atlas_image, float x1, float y1, float x2, Color color, float corner_scale) {
  const AtlasImage &image = atlas->images[atlas_image];
  float um = (image.u1 + image.u2) * 0.5f;
  float iw2 = (image.w * 0.5f) * corner_scale;
  float xa = x1 + iw2;
  float xb = x2 - iw2;
  float u1 = image.u1, v1 = image.v1, u2 = image.u2, v2 = image.v2;
  float y2 = y1 + image.h;
  DrawTexRect(x1, y1, xa, y2, u1, v1, um, v2, color);
  DrawTexRect(xa, y1, xb, y2, um, v1, um, v2, color);
  DrawTexRect(xb, y1, x2, y2, um, v1, u2, v2, color);
}

void DrawBuffer::MeasureText(int font, const char *text, float *w, float *h) {
  const AtlasFont &atlasfont = *atlas->fonts[font];
  unsigned char cval;
  float wacc = 0, maxh = 0;
	int lines = 1;
  while ((cval = *text++) != '\0') {
    if (cval < 32) continue;
    if (cval > 127) continue;
		if (cval == '\n') {
			wacc = 0;
			lines++;
		}
    AtlasChar c = atlasfont.chars[cval - 32];
    wacc += c.wx * fontscalex;
  }
  *w = wacc;
  *h = atlasfont.height * fontscaley * lines;
}

void DrawBuffer::DrawTextShadow(int font, const char *text, float x, float y, Color color, int flags) {
	uint32_t alpha = (color >> 1) & 0xFF000000;
  DrawText(font, text, x + 2, y + 2, alpha, flags);
  DrawText(font, text, x, y, color, flags);
}

void DrawBuffer::DoAlign(int align, float *x, float *y, float w, float h) {
	if (align & ALIGN_HCENTER) *x -= w / 2;
	if (align & ALIGN_RIGHT) *x -= w;
	if (align & ALIGN_VCENTER) *y -= h / 2;
	if (align & ALIGN_BOTTOM) *y -= h;
}

void DrawBuffer::DrawText(int font, const char *text, float x, float y, Color color, int flags) {
  const AtlasFont &atlasfont = *atlas->fonts[font];
  unsigned char cval;
  float w, h;
  MeasureText(font, text, &w, &h);
  if (flags) {
		DoAlign(flags, &x, &y, w, h);
  }
	y+=atlasfont.ascend*fontscaley;
  float sx = x;
  while ((cval = *text++) != '\0') {
    if (cval == '\n') {
      y += atlasfont.height * fontscaley;
      x = sx;
      continue;
    }
    if (cval < 32) continue;
    if (cval > 127) continue;
    AtlasChar c = atlasfont.chars[cval - 32];
    float cx1 = x + c.ox * fontscalex;
    float cy1 = y + c.oy * fontscaley;
    float cx2 = x + (c.ox + c.pw) * fontscalex;
    float cy2 = y + (c.oy + c.ph) * fontscaley;
    V(cx1,  cy1, color, c.sx, c.sy);
    V(cx2,  cy1, color, c.ex, c.sy);
    V(cx2,  cy2, color, c.ex, c.ey);
    V(cx1,  cy1, color, c.sx, c.sy);
    V(cx2,  cy2, color, c.ex, c.ey);
    V(cx1,  cy2, color, c.sx, c.ey);
    x += c.wx * fontscalex;
  }
}

void DrawBuffer::EnableBlend(bool enable) {
	if (enable)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
}