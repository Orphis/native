#include "gl_state.h"

OpenGLState glstate;

void OpenGLState::Initialize() {
	if(initialized) return;

	blendEnabled = true;
	blendFuncS = blendFuncD = blendEquation = 0;
	setBlendEnabled(false);
	setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	setBlendEquation(GL_FUNC_ADD);

	cullFaceEnabled  = true;
	cullFaceMode = 0;
	setCullFaceEnabled(false);
	setCullFace(GL_FRONT);

	depthTestEnabled = true;
	setDepthTestEnabled(false);

	initialized = true;
}