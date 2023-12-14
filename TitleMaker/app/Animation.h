#pragma once

#include "../../QuickGUI/quickgui/Internal.h"
#include "../../QuickGUI/quickgui/QuickGUI.h"

#include <algorithm>

using Easing = std::function<float(float)>;

class Shape;
class Animation {
public:
	virtual void onSetup() {}
	virtual void onRun(NVGcontext* ctx, float t) = 0;
	virtual void onFinish() {}

	virtual void onGUI(QuickGUI* gui, const std::string& baseID);

	void play(Shape* target);
	void update(NVGcontext* ctx, float globalTime, bool forward = true);

	bool finished() const { return m_state == Finished; }
	void reset();

	float delaySecs{ 0.0f };
	float durationSecs{ 1.5f };
	Easing easingFunction{ nullptr };
private:
	enum _State {
		Idle = 0,
		Delaying,
		Running,
		Finished
	} m_state{ Idle };
protected:
	Shape* m_target;
};

class RevealAnimation : public Animation {
public:
	void onSetup();
	void onRun(NVGcontext* ctx, float t);
	void onGUI(QuickGUI* gui, const std::string& baseID);

	enum _Direction {
		FromLeft = 0,
		FromRight,
		FromTop,
		FromBottom
	} direction{ FromLeft };

	Rect bounds;
};

class FadeAnimation : public Animation {
public:
	void onRun(NVGcontext* ctx, float t);
	void onGUI(QuickGUI* gui, const std::string& baseID);

	bool zoom{ false };
};

#define PENNER 1.70158f
#define PI 3.141592654f
#define EPSILON 1e-5f

namespace easings {
	inline float Step(float t) { return t >= 0.5f ? 1.0f : 0.0f; }
	inline float SmoothStep(float t, float e0 = 0.0f, float e1 = 1.0f) {
		t = std::clamp((t - e0) / (e1 - e0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
	inline float Linear(float t) { return t; }
	inline float InQuad(float t) { return t * t; }
	inline float OutQuad(float t) { return t * (2.0f - t); }
	inline float InOutQuad(float t) {
		return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
	}
	inline float InCubic(float t) { return t * t * t; }
	inline float OutCubic(float t) { return (--t) * t * t + 1.0f; }
	inline float InOutCubic(float t) {
		float k = 2.0f * t - 2.0f;
		return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * k * k + 1.0f;
	}
	inline float InQuart(float t) { return t * t * t * t; }
	inline float OutQuart(float t) { return 1.0f - (--t) * t * t * t; }
	inline float InOutQuart(float t) {
		return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (--t) * t * t * t;
	}
	inline float InQuint(float t) { return t * t * t * t * t; }
	inline float OutQuint(float t) { return 1.0f + (--t) * t * t * t * t; }
	inline float InOutQuint(float t) {
		return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f + 16.0f * (--t) * t * t * t * t;
	}
	inline float InSine(float t) { return -1.0f * ::cosf(t / 1.0f * (PI * 0.5f)) + 1.0f; }
	inline float OutSine(float t) { return ::sinf(t / 1.0f * (PI * 0.5f)); }
	inline float InOutSine(float t) { return -1.0f / 2.0f * (::cosf(PI * t) - 1.0f); }
	inline float InExpo(float t) {
		return (t <= EPSILON) ? 0.0f : ::powf(2.0f, 10.0f * (t - 1.0f));
	}
	inline float OutExpo(float t) {
		return (t >= 1.0f - EPSILON) ? 1.0f : (-::powf(2.0f, -10.0f * t) + 1.0f);
	}
	inline float InOutExpo(float t) {
		if (t <= EPSILON) return 0.0f;
		if (t >= 1.0f - EPSILON) return 1.0f;
		if ((t /= 1.0f / 2.0f) < 1.0f) return 1.0f / 2.0f * ::powf(2.0f, 10.0f * (t - 1.0f));
		return 1.0f / 2.0f * (-::powf(2.0f, -10.0f * --t) + 2.0f);
	}
	inline float InCirc(float t) {
		return -1.0f * (::sqrtf(1.0f - t * t) - 1.0f);
	}
	inline float OutCirc(float t) {
		return ::sqrtf(1.0f - (t -= 1.0f) * t);
	}
	inline float InOutCirc(float t) {
		if ((t /= 0.5f) < 1.0f) return -1.0f / 2.0f * (::sqrtf(1.0f - t * t) - 1.0f);
		return 1.0f / 2.0f * (::sqrtf(1.0f - (t -= 2.0f) * t) + 1.0f);
	}
	inline float InElastic(float t) {
		float s = PENNER;
		float p = 0.0f;
		float a = 1.0f;
		if (t <= EPSILON) return 0.0f;
		if (t >= 1.0f - EPSILON) return 1.0f;
		if (!p) p = 0.3f;
		if (a < 1.0f) {
			a = 1.0f;
			s = p / 4.0f;
		}
		else s = p / (2.0f * PI) * ::asinf(1.0f / a);
		return -(a * ::powf(2.0f, 10.0f * (t -= 1.0f)) * ::sinf((t - s) * (2.0f * PI) / p));
	}
	inline float OutElastic(float t) {
		float s = PENNER;
		float p = 0.0f, a = 1.0f;
		if (t <= EPSILON) return 0.0f;
		if (t >= 1.0f - EPSILON) return 1.0f;
		if (!p) p = 0.3f;
		if (a < 1.0f) {
			a = 1.0f;
			s = p / 4.0f;
		}
		else s = p / (2.0f * PI) * ::asinf(1.0f / a);
		return a * ::powf(2.0f, -10.0f * t) * ::sinf((t - s) * (2.0f * PI) / p) + 1.0f;
	}
	inline float InOutElastic(float t) {
		float s = PENNER;
		float p = 0;
		float a = 1;
		if (t <= EPSILON) return 0.0f;
		if ((t /= 0.5f) >= 2.0f - EPSILON) return 1.0f;
		if (!p) p = (0.3f * 1.5f);
		if (a < 1.0f) {
			a = 1.0f;
			s = p / 4.0f;
		}
		else s = p / (2.0f * PI) * ::asinf(1.0f / a);
		if (t < 1.0f)
			return -0.5f * (a * ::powf(2.0f, 10.0f * (t -= 1.0f)) * ::sinf((t - s) * (2.0f * PI) / p));
		return a * ::powf(2.0f, -10.0f * (t -= 1.0f)) * ::sinf((t - s) * (2.0f * PI) / p) * 0.5f + 1.0f;
	}
	inline float InBack(float t) {
		return 1.0f * t * t * ((PENNER + 1.0f) * t - PENNER);
	}
	inline float OutBack(float t) {
		return 1.0f * ((t = t / 1.0f - 1.0f) * t * ((PENNER + 1.0f) * t + PENNER) + 1.0f);
	}
	inline float InOutBack(float t) {
		float s = PENNER;
		if ((t /= 0.5f) < 1.0f) return 1.0f / 2.0f * (t * t * (((s *= (1.525f)) + 1.0f) * t - s));
		return 1.0f / 2.0f * ((t -= 2.0f) * t * (((s *= (1.525f)) + 1.0f) * t + s) + 2.0f);
	}
	inline float OutBounce(float t) {
		if ((t /= 1.0f) < (1.0f / 2.75f)) {
			return (7.5625f * t * t);
		}
		else if (t < (2.0f / 2.75f)) {
			return (7.5625f * (t -= (1.5f / 2.75f)) * t + 0.75f);
		}
		else if (t < (2.5f / 2.75f)) {
			return (7.5625f * (t -= (2.25f / 2.75f)) * t + 0.9375f);
		}
		else {
			return (7.5625f * (t -= (2.625f / 2.75f)) * t + 0.984375f);
		}
	}
	inline float InBounce(float t) {
		return 1.0f - OutBounce(1.0f - t);
	}
	inline float InOutBounce(float t) {
		if (t < 0.5f) return InBounce(t * 2.0f) * 0.5f;
		return OutBounce(t * 2.0f - 1.0f) * 0.5f + 0.5f;
	}
}
