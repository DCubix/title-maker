#pragma once

#include "../../QuickGUI/glad/glad.h"

#include <vector>
#include <cstdint>

class RenderTarget {
public:
	RenderTarget() = default;
	RenderTarget(int width, int height);

	void bind();
	void dispose();

	const std::vector<uint8_t>& readImage();

	GLuint textureId() const { return m_textureId; }
	int width() const { return m_width; }
	int height() const { return m_height; }

private:
	std::vector<uint8_t> m_pixels;
	GLuint m_fbo{ 0 }, m_pbo{ 0 }, m_textureId{0};
	int m_width, m_height;
};
