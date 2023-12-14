#pragma once

#include "../../QuickGUI/glad/glad.h"
#include "../../QuickGUI/nanovg/nanovg.h"

#include "RenderTarget.h"
#include "Shape.h"

class Renderer {
public:
	void setup(NVGcontext* ctx, int width, int height);
	void render(const ShapeList& shapes, float deltaTime);

	RenderTarget& target() { return m_target; }
	const std::vector<uint8_t>& lastFrameData() const { return m_lastFrameData; }

private:
	NVGcontext* m_context;
	RenderTarget m_target;

	std::vector<uint8_t> m_lastFrameData;
};
