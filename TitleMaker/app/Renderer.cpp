#include "Renderer.h"

#include "../../QuickGUI/glad/glad.h"

void Renderer::setup(NVGcontext* ctx, int width, int height) {
	m_context = ctx;
	m_target = RenderTarget(width, height);
}

void Renderer::render(const ShapeList& shapes, float deltaTime) {
	m_target.bind();

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	glViewport(0, 0, m_target.width(), m_target.height());

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	nvgBeginFrame(m_context, m_target.width(), m_target.height(), float(m_target.width()) / float(m_target.height()));
	nvgSave(m_context);
	nvgTranslate(m_context, 0.0f, m_target.height());
	nvgScale(m_context, 1.0f, -1.0f);
	nvgScissor(m_context, 0, 0, m_target.width(), m_target.height());

	for (auto&& shape : shapes) {
		//shape->draw(m_context);
		nvgSave(m_context);
		shape->drawAnimated(m_context, deltaTime);
		nvgRestore(m_context);
	}

	nvgRestore(m_context);
	nvgEndFrame(m_context);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(vp[0], vp[1], vp[2], vp[3]);

	m_lastFrameData = m_target.readImage();
}
