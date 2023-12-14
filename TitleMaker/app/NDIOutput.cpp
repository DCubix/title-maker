#include "NDIOutput.h"

void NDIOutput::start(int width, int height) {
	if (!NDIlib_initialize()) {
		return;
	}

	NDIlib_send_create_t ndiCreateDesc = {};
	ndiCreateDesc.p_ndi_name = "Title Maker NDI Output";

	m_sender = NDIlib_send_create(&ndiCreateDesc);
	if (!m_sender) return;

	m_frameDesc.xres = width;
	m_frameDesc.yres = height;
	m_frameDesc.FourCC = NDIlib_FourCC_type_RGBA;
	m_frameDesc.line_stride_in_bytes = width * 4;
	
	m_frameBuffers[0].resize(width * height * 4);
	m_frameBuffers[1].resize(width * height * 4);

	m_isStarted = true;
	m_exitLoop = false;
	m_ndiThread = std::thread(&NDIOutput::mainLoop, this);
}

void NDIOutput::stop() {
	if (m_isStarted) {
		m_exitLoop = true;
		m_ndiThread.join();
	}
}

void NDIOutput::send(const std::vector<uint8_t>& data) {
	if (m_newFrameReady) return; // already waiting for a frame to be sent

	m_sendLock.lock();
	::memcpy(m_frameBuffers[m_availableFramebuffer].data(), data.data(), data.size());
	m_newFrameReady = true;
	m_sendLock.unlock();
}

void NDIOutput::mainLoop() {
	while (!m_exitLoop) {
		if (!NDIlib_send_get_no_connections(m_sender, 10000)) {
			continue;
		}

		if (m_newFrameReady) {
			m_availableFramebuffer = 1 - m_availableFramebuffer;
			m_newFrameReady = false;
		}
		m_frameDesc.p_data = m_frameBuffers[m_availableFramebuffer].data();

		NDIlib_send_send_video_v2(m_sender, &m_frameDesc);
	}
	m_isStarted = false;
	m_exitLoop = false;
	NDIlib_send_destroy(m_sender);
	NDIlib_destroy();
}
