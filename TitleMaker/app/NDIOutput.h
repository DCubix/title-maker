#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <Processing.NDI.Lib.h>
#include <thread>
#include <mutex>

class NDIOutput {
public:
	void start(int width, int height);
	void stop();
	void send(const std::vector<uint8_t>& data);
	bool started() const { return m_isStarted; }
private:
	std::atomic<bool> m_isStarted{ false };
	std::atomic<bool> m_exitLoop{ false };

	NDIlib_send_instance_t m_sender;
	NDIlib_video_frame_v2_t m_frameDesc{};

	std::vector<uint8_t> m_frameBuffers[2];
	size_t m_availableFramebuffer{ 0 };

	std::atomic<bool> m_newFrameReady{ false };

	void mainLoop();
	std::mutex m_sendLock;
	std::thread m_ndiThread;
};
