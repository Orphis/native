#pragma once

#if defined(ANDROID) || defined(BLACKBERRY)
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


class OpenGLState
{
private:
	bool initialized;

	bool blendEnabled;
	GLenum blendFuncS, blendFuncD, blendEquation;

	bool   cullFaceEnabled;
	GLenum cullFaceMode;

	bool   depthTestEnabled;
	GLenum depthTestFunc;

	inline void setState(bool &oldValue, bool newValue, GLenum state) {
		if(newValue && !oldValue) {
			glEnable(state);
			oldValue = newValue;
		} else if(!newValue && oldValue) {
			glDisable(state);
			oldValue = newValue;
		}
	}

public:
	OpenGLState() : initialized(false) {}
	void Initialize();

	inline void setBlendEnabled(bool newState) {
		setState(blendEnabled, newState, GL_BLEND);
	}
	inline void setBlendFunc(GLenum sfactor, GLenum dfactor) {
		if(sfactor != blendFuncS || dfactor != blendFuncD) {
			glBlendFunc(sfactor, dfactor);
			blendFuncS = sfactor;
			blendFuncD = dfactor;
		}
	}
	inline void setBlendEquation(GLenum eq) {
		if(eq != blendEquation) {
			glBlendEquation(eq);
			blendEquation = eq;
		}
	}
	inline void setCullFaceEnabled(bool newState) {
		setState(cullFaceEnabled, newState, GL_CULL_FACE);
	}
	inline void setCullFace(GLenum mode) {
		if(mode != cullFaceMode) {
			glCullFace(mode);
			cullFaceMode = mode;
		}
	}
	inline void setDepthTestEnabled(bool newState) {
		setState(depthTestEnabled, newState, GL_DEPTH_TEST);
	}
	inline void setDepthFunc(GLenum func) {
		if(func != depthTestFunc) {
			glDepthFunc(func);
			depthTestFunc = func;
		}
	}
};
	
extern OpenGLState glstate;