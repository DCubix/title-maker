#include "Animation.h"

#include "Shape.h"
#include "../../QuickGUI/quickgui/Icons.h"

static MenuItem easingsMenu[] = {
	{ 0, "Linear", {} },
	{ 0, "In Cubic", {} },
	{ 0, "Out Cubic", {} },
	{ 0, "In/Out Cubic", {} },
	{ 0, "In Quad", {} },
	{ 0, "Out Quad", {} },
	{ 0, "In/Out Quad", {} },
	{ 0, "In Elastic", {} },
	{ 0, "Out Elastic", {} },
	{ 0, "Bounce", {} }
};

void Animation::onGUI(QuickGUI* gui, const std::string& baseID) {
	gui->number("ani_duration" + baseID, gui->layoutCutTop(24), durationSecs, 0.0f, 30.0f, 0.01f, "{:.2f}s", "Duration:");
	gui->layoutCutTop(5);

	gui->number("ani_delay" + baseID, gui->layoutCutTop(24), delaySecs, 0.0f, 30.0f, 0.01f, "{:.2f}s", "Delay:");
	gui->layoutCutTop(5);

	static size_t selectedEasing = 0;
	if (gui->button("ani_easing_sel" + baseID, easingsMenu[selectedEasing].text, gui->layoutCutTop(24), IC_LINE_CHART)) {
		gui->showPopup("ani_easing" + baseID);
	}

	if (gui->popup("ani_easing" + baseID, easingsMenu, 10, selectedEasing)) {
		Easing easingFuncs[] = {
			easings::Linear,
			easings::InCubic,
			easings::OutCubic,
			easings::InOutCubic,
			easings::InQuad,
			easings::OutQuad,
			easings::InOutQuad,
			easings::InElastic,
			easings::OutElastic,
			easings::OutBounce
		};
		easingFunction = easingFuncs[selectedEasing];
	}

	gui->layoutCutTop(5);
}

void Animation::play(Shape* target) {
	if (m_state == Delaying || m_state == Running) return;

	m_state = Delaying;
	m_target = target;
	onSetup();
}

void Animation::update(NVGcontext* ctx, float globalTime, bool forward) {
	switch (m_state) {
		case Delaying: {
			onRun(ctx, forward ? 0.0f : 1.0f);

			if (globalTime >= delaySecs) {
				m_state = Running;
			}
		} break;
		case Running: {
			float t = (globalTime - delaySecs) / durationSecs;
			float v = forward ? t : (1.0f - t);

			onRun(ctx, easingFunction ? easingFunction(v) : v);
			if (t >= 1.0f) {
				onFinish();
				m_state = Finished;
			}
		} break;
		default: break;
	}
}

void Animation::reset() {
	m_state = Idle;
}

void RevealAnimation::onSetup() {
	bounds = m_target->bounds;
}

void RevealAnimation::onRun(NVGcontext* ctx, float t) {
	Rect b;

	switch (direction) {
		case FromLeft:
			b.x = bounds.x;
			b.y = bounds.y;
			b.width = bounds.width * t;
			b.height = bounds.height;
			break;
		case FromRight:
			b.x = bounds.x + bounds.width * (1.0f - t);
			b.y = bounds.y;
			b.width = bounds.width * t;
			b.height = bounds.height;
			break;
		case FromTop:
			b.x = bounds.x;
			b.y = bounds.y;
			b.width = bounds.width;
			b.height = bounds.height * t;
			break;
		case FromBottom:
			b.x = bounds.x;
			b.y = bounds.y + bounds.height * (1.0f - t);
			b.width = bounds.width;
			b.height = bounds.height * t;
			break;
	}

	nvgIntersectScissor(
		ctx,
		b.x, b.y,
		b.width, b.height
	);
}

void RevealAnimation::onGUI(QuickGUI* gui, const std::string& baseID) {
	Animation::onGUI(gui, baseID);

	MenuItem directions[] = {
		{ IC_ARROW_RIGHT2, "From Left", {} },
		{ IC_ARROW_LEFT2, "From Right", {} },
		{ IC_ARROW_DOWN2, "From Top", {} },
		{ IC_ARROW_UP2, "From Bottom", {} }
	};

	static size_t selectedDir = 0;
	if (gui->button("ani_reveal_direction" + baseID, directions[selectedDir].text, gui->layoutCutTop(24), directions[selectedDir].icon)) {
		gui->showPopup("ani_reveal_direction_opts" + baseID);
	}

	if (gui->popup("ani_reveal_direction_opts" + baseID, directions, 4, selectedDir)) {
		direction = _Direction(selectedDir);
	}
}

void FadeAnimation::onRun(NVGcontext* ctx, float t) {
	nvgGlobalAlpha(ctx, t);
	if (zoom) {
		float v = 1.0f - t;
		Rect b = m_target->bounds;
		nvgTranslate(ctx, b.x, b.y);
		nvgTranslate(ctx, b.width / 2.0f, b.height / 2.0f);
		nvgScale(ctx, 1.0f + v * 0.25f, 1.0f + v * 0.25f);
		nvgTranslate(ctx, -b.width / 2.0f, -b.height / 2.0f);
		nvgTranslate(ctx, -b.x, -b.y);
	}
}

void FadeAnimation::onGUI(QuickGUI* gui, const std::string& baseID) {
	Animation::onGUI(gui, baseID);

	gui->checkBox("ani_fade_zoom" + baseID, "Zoom", gui->layoutCutTop(24), zoom);
}
