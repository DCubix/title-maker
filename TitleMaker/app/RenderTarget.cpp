#include "RenderTarget.h"

RenderTarget::RenderTarget(int width, int height) {
	glGenTextures(1, &m_textureId);
	glBindTexture(GL_TEXTURE_2D, m_textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenBuffers(1, &m_pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, nullptr, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	m_width = width;
	m_height = height;
}

void RenderTarget::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void RenderTarget::dispose() {
	glDeleteTextures(1, &m_textureId);
	glDeleteFramebuffers(1, &m_fbo);
	glDeleteBuffers(1, &m_pbo);
}

const std::vector<uint8_t>& RenderTarget::readImage() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	const size_t dataSize = m_width * m_height * 4;
	if (m_pixels.size() < dataSize) {
		m_pixels.resize(dataSize);
	}

	auto ret = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (!ret) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	::memcpy(m_pixels.data(), ret, dataSize);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return m_pixels;
}
