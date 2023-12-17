#include "Shape.h"

#include "../../QuickGUI/quickgui/Icons.h"
#include "portable-file-dialogs.h"
#include <filesystem>

void Rectangle::draw(NVGcontext* ctx) {
	Shape::draw(ctx);

	Rect b = rectSpaceBounds();
	nvgBeginPath(ctx);
	if (borderRadius > 0.0f) nvgRoundedRect(ctx, b.x, b.y, b.width, b.height, borderRadius);
	else nvgRect(ctx, b.x, b.y, b.width, b.height);
	applyFill(ctx);
	nvgFill(ctx);

	if (borderWidth > 0.0f) {
		nvgStrokeWidth(ctx, borderWidth);
		nvgStrokeColor(ctx, nvgColor(borderColor));
		nvgStroke(ctx);
	}
}

void Ellipse::draw(NVGcontext* ctx) {
	Shape::draw(ctx);

	Rect b = rectSpaceBounds();
	nvgBeginPath(ctx);
	nvgEllipse(ctx,
		b.x + b.width / 2.0f, b.y + b.height / 2.0f,
		b.width / 2.0f, b.height / 2.0f
	);
	applyFill(ctx);
	nvgFill(ctx);

	if (borderWidth > 0.0f) {
		nvgStrokeWidth(ctx, borderWidth);
		nvgStrokeColor(ctx, nvgColor(borderColor));
		nvgStroke(ctx);
	}
}

static RadioButton fillStyleButtons[] = {
	{ IC_WATERDROP, "Solid Color" },
	{ IC_WATERDROPS, "Gradient" }
};

void ColoredShape::gui(QuickGUI* gui) {
	static size_t selected = size_t(fillMode);
	gui->text("Background Mode", gui->layoutCutTop(19));
	if (gui->radioSelector("fill_mode", gui->layoutCutTop(26), fillStyleButtons, 2, selected)) {
		fillMode = ColoredShape::FillMode(selected);
	}
	gui->layoutCutTop(8);
	
	if (fillMode == ColoredShape::SolidColor) {
		auto& col = background.color[0];
		gui->text("Background Color", gui->layoutCutTop(19));
		gui->colorPicker("bg0_color", gui->layoutCutTop(160), col);
		gui->layoutCutTop(2);
		gui->number("bg0_color_a", gui->layoutCutTop(26), col[3], 0.0f, 1.0f, 0.01f, "{:.2f}", "A");
		gui->layoutCutTop(8);
	}
	else {
		auto& col0 = background.color[0];
		auto& col1 = background.color[1];
		gui->text("Background", gui->layoutCutTop(19));
		gui->colorPicker("bg0_color", gui->layoutCutTop(140), col0);
		gui->layoutCutTop(2);
		gui->number("bg0_color_a", gui->layoutCutTop(26), col0[3], 0.0f, 1.0f, 0.01f, "{:.2f}", "A");
		gui->colorPicker("bg1_color", gui->layoutCutTop(140), col1);
		gui->layoutCutTop(2);
		gui->number("bg1_color_a", gui->layoutCutTop(26), col1[3], 0.0f, 1.0f, 0.01f, "{:.2f}", "A");
		gui->layoutCutTop(8);

		gui->text("Extents", gui->layoutCutTop(19));
		{
			auto cols = gui->layoutSliceHorizontal(26, 2);
			gui->number("bg_ex1_x", cols[0], background.stops[0].x, -1.0f, 1.0f, 0.01f, "{:.2f}", "X1");
			gui->number("bg_ex1_y", cols[1], background.stops[0].y, -1.0f, 1.0f, 0.01f, "{:.2f}", "Y1");
		}
		{
			auto cols = gui->layoutSliceHorizontal(26, 2);
			gui->number("bg_ex2_x", cols[0], background.stops[1].x, -1.0f, 1.0f, 0.01f, "{:.2f}", "X2");
			gui->number("bg_ex2_y", cols[1], background.stops[1].y, -1.0f, 1.0f, 0.01f, "{:.2f}", "Y2");
		}
		gui->layoutCutTop(8);
	}

	gui->text("Border Color", gui->layoutCutTop(19));
	gui->colorPicker("bd_color", gui->layoutCutTop(160), borderColor);
	gui->layoutCutTop(2);
	gui->number("bd_color_a", gui->layoutCutTop(26), borderColor[3], 0.0f, 1.0f, 0.01f, "{:.2f}", "A");
	gui->layoutCutTop(8);

	gui->text("Border Width", gui->layoutCutTop(19));
	gui->number("bd_width", gui->layoutCutTop(26), borderWidth, 0.0f, 99.0f, 0.1f, "{:.1f} px");
	gui->layoutCutTop(8);
}

void ColoredShape::applyFill(NVGcontext* ctx) {
	if (fillMode == ColoredShape::SolidColor) {
		nvgFillColor(ctx, nvgColor(background.color[0]));
	}
	else {
		float sx = background.stops[0].x * bounds.width;
		float sy = background.stops[0].y * bounds.height;
		float ex = background.stops[1].x * bounds.width;
		float ey = background.stops[1].y * bounds.height;
		auto [ colA, colB ] = background.color;
		auto paint = nvgLinearGradient(
			ctx,
			sx, sy, ex, ey,
			nvgRGBAf(colA[0], colA[1], colA[2], colA[3]),
			nvgRGBAf(colB[0], colB[1], colB[2], colB[3])
		);
		nvgFillPaint(ctx, paint);
	}
}

void Shape::draw(NVGcontext* ctx) {
	Point position = bounds.location();
	Transform xformDraw = Transform::translation(position) * Transform::rotation(nvgDegToRad(rotation));
	xformDraw.toNanoVG(ctx);
}

void Shape::drawAnimated(NVGcontext* ctx, float deltaTime) {
	Animation* anim = nullptr;
	switch (m_state) {
		case Entering: anim = animations[size_t(ShapeAnimation::Enter)].get(); break;
		case Exiting: anim = animations[size_t(ShapeAnimation::Exit)].get(); break;
		default: break;
	}

	if (anim) {
		nvgSave(ctx);

		anim->update(ctx, m_globalTime, m_state == Entering);
		m_globalTime += deltaTime;

		if (anim->finished()) {
			anim->reset();
			m_state = Idling;
			m_nextState = Idling;
			triggerAnim();
		}
	}
	else {
		m_state = m_nextState;
		triggerAnim();
	}

	draw(ctx);

	if (anim) {
		nvgRestore(ctx);
	}
}

void Shape::triggerEnter() {
	if (m_nextState != Entering)
		m_nextState = Entering;
}

void Shape::triggerExit() {
	if (m_nextState != Exiting)
		m_nextState = Exiting;
}

Rect Shape::rectSpaceBounds() const {
	Point hsize = bounds.size() * 0.5f;
	return Rect(
		-hsize.x,
		-hsize.y,
		bounds.width,
		bounds.height
	);
}

void Shape::triggerAnim() {
	m_globalTime = 0.0f;

	Animation* anim = nullptr;
	switch (m_state) {
		case Entering: anim = animations[size_t(ShapeAnimation::Enter)].get(); break;
		case Exiting: anim = animations[size_t(ShapeAnimation::Exit)].get(); break;
		default: break;
	}
	
	if (!anim) return;

	anim->play(this);
}

void Text::draw(NVGcontext* ctx) {
	Shape::draw(ctx);

	if (!m_fontFileName.empty() || nvgFindFont(ctx, font.c_str()) == -1) {
		m_fontHandle = nvgCreateFont(ctx, font.c_str(), m_fontFileName.c_str());
		m_fontFileName = "";
	}

	if (m_fontHandle > 0) nvgFontFaceId(ctx, m_fontHandle);
	nvgFillColor(ctx, nvgColor(background.color[0]));
	nvgFontSize(ctx, fontSize);
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgTextBox(ctx, bounds.x, bounds.y, bounds.width, text.c_str(), nullptr);
}

void Text::gui(QuickGUI* gui) {
	auto& col = background.color[0];
	gui->text("Color", gui->layoutCutTop(19));
	gui->colorPicker("bg0_color", gui->layoutCutTop(160), col);
	gui->layoutCutTop(2);
	gui->number("bg0_color_a", gui->layoutCutTop(26), col[3], 0.0f, 1.0f, 0.01f, "{:.2f}", "A");
	gui->layoutCutTop(8);
	
	gui->text("Text", gui->layoutCutTop(19));
	gui->textEdit("txt_text", gui->layoutCutTop(26), text, "Say something...");
	gui->layoutCutTop(5);

	gui->number("txt_font_size", gui->layoutCutTop(26), fontSize, 8.0f, 256.0f, 1.0f, "{:.0f}px", "Font Size:");

	gui->layoutCutTop(5);

	std::string btnFontText = m_fontHandle <= 0 ? "Select Font" : font;
	if (gui->button("btn_font_file", btnFontText, gui->layoutCutTop(26), IC_DOCUMENT_TEXT)) {
		auto fp = pfd::open_file(
			"Select Font", pfd::path::home(),
			{ "Fonts (.ttf .otf)", "*.ttf *.otf" },
			pfd::opt::none
		);

		if (!fp.result().empty()) {
			m_fontFileName = fp.result()[0];
			font = std::filesystem::path(m_fontFileName).stem().generic_string();
		}
	}
}
