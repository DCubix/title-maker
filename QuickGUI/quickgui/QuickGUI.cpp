#include "QuickGUI.h"

#define NANOVG_GL3_IMPLEMENTATION
#include "../nanovg/nanovg_gl.h"

#include "Icons.h"

#include <algorithm>
#include <fstream>
#include <format>

#pragma region Color Stuff
static void rgbToHSV(float r, float g, float b, float& h, float& s, float& v) {
	float min, max, delta;

	min = std::min(std::min(r, g), b);
	max = std::max(std::max(r, g), b);

	v = max;
	delta = max - min;

	if (delta < 1e-5f) {
		s = 0.0f;
		h = 0.0f;
		return;
	}

	if (max > 0.0f) {
		s = (delta / max);
	}
	else {
		s = 0.0f;
		h = NAN;
		return;
	}

	if (r >= max) {
		h = (g - b) / delta;
	}
	else {
		if (g >= max) {
			h = 2.0f + (b - r) / delta;
		}
		else {
			h = 4.0f + (r - g) / delta;
		}
	}

	h *= 60.0;

	if (h < 0.0) {
		h += 360.0;
	}

	h /= 360.0f;
}

static void hsvToRGB(float h, float s, float v, float& r, float& g, float& b) {
	int i = int(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	switch (i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}
}
#pragma endregion

QuickGUI::~QuickGUI() {
	if (m_context) {
		nvgDeleteGL3(m_context);
		m_context = nullptr;
	}
}

void QuickGUI::beginFrame(uint32_t width, uint32_t height) {
	if (!m_state.mouseDown) m_state.hoveredWidget = InvalidWidget;

	m_state.screenWidth = width;
	m_state.screenHeight = height;

	nvgBeginFrame(context(), width, height, float(width) / float(height));
	nvgFontSize(context(), 13.0f);
	m_layoutStack.push_back(Rect(0, 0, width, height));

	nvgScissor(context(), 0, 0, width, height);
}

void QuickGUI::endFrame() {
	renderPopups();

	if (!m_state.mouseDown) m_state.activeWidget = InvalidWidget;
	m_state.mouseDelta.x = 0;
	m_state.mouseDelta.y = 0;
	nvgEndFrame(context());
	m_layoutStack.clear();
	m_state.keyDown = false;
	m_state.keyUp = false;
	m_state.mouseScroll = 0.0f;
}

NVGcontext* QuickGUI::context() {
	if (!m_context) {
		int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
#ifndef _NDEBUG
		flags |= NVG_DEBUG;
#endif
		m_context = nvgCreateGL3(flags);

		m_fontNormal = nvgCreateFont(m_context, "normal", "OpenSans-Regular.ttf");
		m_fontBold = nvgCreateFont(m_context, "bold", "OpenSans-Bold.ttf");
		m_fontItalic = nvgCreateFont(m_context, "italic", "OpenSans-Italic.ttf");
		m_fontIcons = nvgCreateFont(m_context, "icons", "entypo.ttf");
		nvgAddFallbackFontId(m_context, m_fontNormal, m_fontIcons);
		nvgAddFallbackFontId(m_context, m_fontBold, m_fontIcons);
		nvgAddFallbackFontId(m_context, m_fontItalic, m_fontIcons);
		nvgFontFace(m_context, "normal");

		std::ifstream fp("style.sty");
		std::string str = std::string(
			std::istreambuf_iterator<char>(fp),
			std::istreambuf_iterator<char>()
		);
		m_styleSheet.parse(str);
	}
	return m_context;
}

static WidgetID widgetID(const std::string& id) {
	return std::hash<std::string>()(id);
}

Widget QuickGUI::widget(const std::string& id, Rect bounds, bool checkBlocked) {
	const WidgetID wid = widgetID(id);

	auto pos = m_widgetState.find(wid);
	if (pos == m_widgetState.end()) {
		m_widgetState[wid] = Widget();
		pos = m_widgetState.find(wid);
	}

	Widget& ret = pos->second;
	ret.clicked = false;
	ret.keyPressed = false;
	ret.relativeDelta.x = 0;
	ret.relativeDelta.y = 0;
	ret.relativeMouse = {
		.x = std::clamp(m_state.mousePosition.x - bounds.x, 0.0f, bounds.width),
		.y = std::clamp(m_state.mousePosition.y - bounds.y, 0.0f, bounds.height),
	};
	ret.id = wid;
	ret.state = wid == m_state.focusedWidget ? WidgetState::Focused : WidgetState::Normal;
	ret.position = { bounds.x, bounds.y };

	bool blocked = (m_inputBlocked && checkBlocked);

	// check for panels
	bool insideParentPanel = true;
	if (!m_panelStack.empty()) {
		auto panel = m_panelStack.back();
		auto panelW = m_panels[panel.id];
		Rect parentRect = panel.bounds;

		// compensate scrolling
		parentRect.x += panelW.scroll.x;
		parentRect.y += panelW.scroll.y;
		insideParentPanel = parentRect.hasPoint(m_state.mousePosition);
	}

	if (bounds.hasPoint(m_state.mousePosition) && insideParentPanel && !blocked) {
		m_state.hoveredWidget = wid;
		ret.relativeDelta = m_state.mouseDelta;
		ret.state = WidgetState::Hovered;

		if (m_state.mouseDown && m_state.activeWidget == InvalidWidget) {
			m_state.focusedWidget = wid;
			m_state.activeWidget = wid;
			ret.state = WidgetState::Active;
		}
	}

	if (wid == m_state.focusedWidget && m_state.keyDown && !blocked) {
		ret.keyPressed = true;
	}

	if (isMouseDownWidget(wid) && !blocked) {
		ret.state = WidgetState::Active;
	}

	if (!m_state.mouseDown && isMouseDownWidget(wid) && !blocked) {
		m_state.lastClickedPosition.x = bounds.x;
		m_state.lastClickedPosition.y = bounds.y + bounds.height;
		ret.clicked = true;
	}

	//auto ctx = context();
	//nvgSave(ctx);
	//nvgBeginPath(ctx);
	//nvgRect(ctx, bounds.x, bounds.y, bounds.width, bounds.height);
	//if (wid == m_state.focusedWidget) {
	//	nvgStrokeColor(ctx, nvgRGB(0, 0, 255));
	//}
	//else {
	//	nvgStrokeColor(ctx, nvgRGB(255, 255, 0));
	//}
	//nvgStrokeWidth(ctx, 1.0f);
	//nvgStroke(ctx);
	//nvgRestore(ctx);

	return ret;
}

void QuickGUI::text(const std::string& text, Rect bounds, const std::string& style) {
	auto ctx = context();
	m_styleSheet.draw(style, ctx, bounds, text, 0);
}

bool QuickGUI::button(const std::string& id, const std::string& text, Rect bounds, size_t icon) {
	auto ctx = context();
	auto wd = widget(id, bounds);

	std::string style = "button";
	switch (wd.state) {
		case WidgetState::Normal:
		case WidgetState::Focused: style = "button"; break;
		case WidgetState::Hovered: style = "button_hover"; break;
		case WidgetState::Active: style = "button_active"; break;
		case WidgetState::Disabled: style = "button_disabled"; break;
	}
	
	m_styleSheet.draw(style, ctx, bounds, text, icon);

	return wd.clicked;
}

bool QuickGUI::iconButton(const std::string& id, size_t icon, Rect bounds) {
	auto ctx = context();
	auto wd = widget(id, bounds);

	std::string style = "icon_button";
	switch (wd.state) {
		case WidgetState::Normal:
		case WidgetState::Focused: style = "icon_button"; break;
		case WidgetState::Hovered: style = "icon_button_hover"; break;
		case WidgetState::Active: style = "icon_button_active"; break;
		case WidgetState::Disabled: style = "icon_button_disabled"; break;
	}

	auto element = m_styleSheet.draw(style, ctx, bounds, "", 0);
	Rect innerBounds = element.bounds;

	nvgSave(ctx);
	nvgTranslate(ctx, bounds.x, bounds.y);

	nvgTextAlign(ctx, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
	nvgFontSize(ctx, 23.0f);

	auto ico = ICON(icon);
	float b[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, ico.c_str(), nullptr, b);

	float iconWidth = b[2] - b[0];
	float iconHeight = b[3] - b[1];
	float iconOffX = (innerBounds.width / 2.0f - iconWidth / 2.0f);
	float iconOffY = (innerBounds.height / 2.0f - iconHeight / 2.0f);

	nvgText(
		ctx,
		innerBounds.x + iconOffX,
		innerBounds.y + iconOffY + 1.5f,
		ico.c_str(),
		nullptr
	);

	nvgRestore(ctx);

	return wd.clicked;
}

void QuickGUI::checkBox(const std::string& id, const std::string& text, Rect bounds, bool& checked) {
	const float boxSize = 22.0f;

	auto ctx = context();
	auto wd = widget(id, bounds);

	Rect tBounds = bounds;
	tBounds.x += (boxSize + 5.0f);
	tBounds.width -= (boxSize + 5.0f);
	m_styleSheet.draw("text_middle", ctx, tBounds, text, 0);

	Rect pBounds = bounds;
	pBounds.width = pBounds.height = boxSize;
	pBounds.y = bounds.y + (bounds.height / 2.0f - boxSize / 2.0f);
	m_styleSheet.draw("panel", ctx, pBounds, "", checked ? IC_CHECK : 0);

	if (wd.clicked) {
		checked = !checked;
	}
}

void QuickGUI::showPopup(const std::string& id) {
	m_openPopup = widgetID(id);
	m_inputBlocked = true;
}

bool QuickGUI::popup(const std::string& id, MenuItem* items, size_t numItems, size_t& selected) {
	auto wid = widgetID(id);
	auto&& pos = m_popups.find(wid);
	if (pos == m_popups.end()) {
		m_popups[wid] = Popup();
		pos = m_popups.find(wid);
	}

	pos->second.id = id;
	pos->second.items = std::vector<MenuItem>(items, items + numItems);

	if (m_openPopup != wid) return false;

	const size_t itemHeight = 24;
	const size_t paddingX = 10;
	const size_t paddingY = 6;

	auto ctx = context();
	auto position = m_state.lastClickedPosition;

	int width = 0;
	for (size_t i = 0; i < numItems; i++) {
		float bounds[4];
		nvgTextBounds(ctx, 0, 0, items[i].text.c_str(), nullptr, bounds);

		int textWidth = int(bounds[2] - bounds[0]);
		if (items[i].icon) {
			textWidth += iconSpaceWidth;
		}

		width = std::max(width, textWidth);
	}

	Rect bounds = Rect(
		position.x, position.y,
		width + paddingX * 2,
		itemHeight * numItems + paddingY * 2
	);

	if (m_state.mouseDown && !bounds.hasPoint(m_state.mousePosition)) {
		m_openPopup = InvalidWidget;
		m_inputBlocked = false;
		return false;
	}

	float y = bounds.y + paddingY;
	for (size_t i = 0; i < numItems; i++) {
		MenuItem item = items[i];

		Rect wdBounds = Rect(bounds.x, y, bounds.width, itemHeight);
		auto wd = widget(item.text + id, wdBounds, false);

		if (wd.clicked) {
			m_inputBlocked = false;
			m_openPopup = InvalidWidget;
			selected = i;
			return true;
		}

		y += itemHeight;
	}

	return false;
}

static void updateCursor(TextEdit& edit, Rect bounds) {
	if (edit.cursor <= 0) {
		return;
	}
	auto glyph = edit.glyphs[edit.cursor - 1];
	float glyphX = glyph.x;
	float glyphW = (glyph.maxx - glyph.minx) + 1.5f;
	
	float nextGlyphX = (glyphX + glyphW) - edit.viewOffset;
	if (nextGlyphX > bounds.width) {
		edit.viewOffset = (glyphX + glyphW) - bounds.width;
	}
	else if (nextGlyphX < 0.0f) {
		edit.viewOffset = (glyphX + glyphW) - bounds.width;
		edit.viewOffset = std::max(0.0f, edit.viewOffset);
	}
}

static void drawCursor(NVGcontext* ctx, TextEdit& edit, Rect bounds) {
	edit.blinkTime += 0.07f;
	if (edit.blinkTime >= 1.0f) {
		edit.blinkTime = 0.0f;
		edit.cursorShow = !edit.cursorShow;
	}

	if (edit.cursorShow) {
		float cursorX = edit.glyphs[edit.cursor].x;

		nvgSave(ctx);
		nvgTranslate(ctx, bounds.x, bounds.y);
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(ctx, cursorX - edit.viewOffset, bounds.height / 2, "|", nullptr);
		nvgRestore(ctx);
	}
}

static void lineEditor(
	const Widget& wd,
	Rect bounds,
	TextEdit& edit,
	std::string& text,
	QuickGUIState& guiState,
	float xOffset = 0.0f
) {
	if (wd.keyPressed) {
		if (edit.ctrl) {
			switch (guiState.key) {
				case Key::V: /* TODO: Clipboard Test */ break;
				default: break;
			}
			edit.ctrl = false;
			return;
		}

		if (::isprint(guiState.typedChar)) {
			text.insert(text.begin() + edit.cursor, char(guiState.typedChar));
			edit.cursor++;
			updateCursor(edit, bounds);
			edit.blinkTime = 0.0f;
			edit.cursorShow = true;
		}
		else {
			switch (guiState.key) {
				case Key::RIGHT:edit.cursor = std::min(int32_t(text.size()), edit.cursor + 1); break;
				case Key::LEFT: edit.cursor = edit.cursor > 0 ? edit.cursor - 1 : 0; break;
				case Key::BACKSPACE: {
					if (edit.cursor > 0) {
						edit.cursor = edit.cursor > 0 ? edit.cursor - 1 : 0;
						text.erase(text.begin() + edit.cursor);
					}
				} break;
				case Key::DELETE: {
					text.erase(text.begin() + edit.cursor);
					edit.cursor = std::min(int32_t(text.size()), edit.cursor);
				} break;
				case Key::HOME: edit.cursor = 0; break;
				case Key::END: edit.cursor = text.size(); break;
				case Key::LCONTROL:
				case Key::RCONTROL:
					edit.ctrl = true;
					break;
				default: break;
			}
			edit.blinkTime = 0.0f;
			edit.cursorShow = true;
			updateCursor(edit, bounds);
		}
	}
	else if (wd.clicked && guiState.mouseButton == QuickGUIState::Left) {
		edit.cursor = edit.glyphs.size() - 1;
		size_t i = 0;
		for (const auto& glyph : edit.glyphs) {
			float gw = glyph.maxx - glyph.minx;
			Rect grect = {
				(xOffset + ((glyph.x + 4) - gw / 2) - edit.viewOffset), 0.0f,
				gw, bounds.height
			};
			if (grect.hasPoint(wd.relativeMouse)) {
				edit.cursor = i;
				break;
			}
			i++;
		}
		edit.blinkTime = 0.0f;
		edit.cursorShow = true;
	}
}

static void drawEditText(NVGcontext* ctx, TextEdit& edit, const std::string& text, Rect bounds) {
	std::string textP = text + " ";
	edit.glyphs.resize(textP.size());

	nvgSave(ctx);
	nvgTranslate(ctx, bounds.x, bounds.y);

	nvgIntersectScissor(ctx, 0.0f, 0.0f, bounds.width, bounds.height);
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgTextGlyphPositions(ctx, 0.0f, 0.0f, textP.c_str(), nullptr, edit.glyphs.data(), edit.glyphs.size());

	nvgText(ctx, -edit.viewOffset, bounds.height / 2 + 1.5f, text.c_str(), nullptr);
	nvgRestore(ctx);
}

void QuickGUI::textEdit(const std::string& id, Rect bounds, std::string& text, const std::string& placeholder) {
	auto wd = widget(id, bounds);
	auto ctx = context();

	bool focused = m_state.focusedWidget == wd.id;

	std::string style = "textedit";
	if (focused) {
		style = "textedit_focus";
	}
	m_styleSheet.draw(style, ctx, bounds, "", 0);
	
	if (text.empty() && !focused) {
		m_styleSheet.draw("placeholder", ctx, bounds, placeholder, 0);
	}
	
	m_styleSheet.fontSetup(style, ctx, bounds);

	auto pos = m_textEdits.find(wd.id);
	if (pos == m_textEdits.end()) {
		m_textEdits[wd.id] = TextEdit();
		pos = m_textEdits.find(wd.id);
	}

	auto& edit = pos->second;

	std::string textP = text + " ";
	edit.glyphs.resize(textP.size());

	nvgSave(ctx);

	drawEditText(ctx, edit, text, bounds);
	
	if (focused) {
		drawCursor(ctx, edit, bounds);
		lineEditor(wd, bounds, edit, text, m_state);
	}

	nvgRestore(ctx);
}

static float snap(float v, float step) {
	float newStep = ::roundf(v / step);
	float newValue = newStep * step;
	return newValue;
}

static void updateValue(NumberEdit& numEdit, int dx, float& value, float step, float vmin, float vmax) {
	const float sensitivity = 0.2f;

	numEdit.actualValue += float(dx) * step * sensitivity;

	float newValue = snap(numEdit.actualValue, step);

	if (newValue != value) {
		value = std::clamp(newValue, vmin, vmax);
	}
}

void QuickGUI::number(
	const std::string& id,
	Rect bounds,
	float& value, float minValue, float maxValue, float step,
	const std::string& fmt,
	const std::string& label
)
{
	Rect decBounds = Rect(bounds.x, bounds.y, 16, bounds.height);
	Rect incBounds = Rect(bounds.x + bounds.width - 16, bounds.y, 16, bounds.height);
	Rect mainBounds = Rect(
		bounds.x + decBounds.width,
		bounds.y,
		bounds.width - (decBounds.width + incBounds.width),
		bounds.height
	);

	auto ctx = context();
	auto wid = widget(id, mainBounds);
	bool focused = m_state.focusedWidget == wid.id;

	auto pos = m_numberEdits.find(wid.id);
	if (pos == m_numberEdits.end()) {
		m_numberEdits[wid.id] = NumberEdit();
	}

	auto& numEdit = m_numberEdits[wid.id];

	std::string valueText = std::vformat(fmt, std::make_format_args(value));

	nvgSave(ctx);
	
	m_styleSheet.draw("panel", ctx, bounds, "", 0);

	auto el = m_styleSheet.fontSetup("textedit", ctx, mainBounds);

	float labelOffset = 0.0f;
	if (!numEdit.editingText) {
		nvgTranslate(ctx, mainBounds.x, mainBounds.y);

		auto allText = label + valueText;
		float txtBounds[4];
		nvgTextBounds(ctx, 0.0f, 0.0f, allText.c_str(), nullptr, txtBounds);
		labelOffset = mainBounds.width / 2 - (txtBounds[2] - txtBounds[0]) / 2;

		if (!label.empty()) {
			nvgSave(ctx);
			nvgGlobalAlpha(ctx, 0.5f);
			nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgApplyFillPaint(ctx, el.textPaint.value_or(Color{ 1.0f, 1.0f, 1.0f, 1.0f }));
			nvgText(ctx, labelOffset, mainBounds.height / 2 + 1.5f, label.c_str(), nullptr);
			
			float lblBounds[4];
			nvgTextBounds(ctx, 0.0f, 0.0f, label.c_str(), nullptr, lblBounds);

			labelOffset += (lblBounds[2] - lblBounds[0]) + 3.0f;

			nvgRestore(ctx);
		}

		nvgApplyFillPaint(ctx, el.textPaint.value_or(Color{ 1.0f, 1.0f, 1.0f, 1.0f }));
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(ctx, labelOffset, mainBounds.height / 2 + 1.5f, valueText.c_str(), nullptr);
	}
	else {
		drawEditText(ctx, numEdit.edit, numEdit.text, mainBounds);
		if (focused) {
			drawCursor(ctx, numEdit.edit, mainBounds);
		}
	}

	nvgRestore(ctx);

	if (iconButton(id + "_dec", IC_CHEVRON_LEFT, decBounds)) {
		value -= step;
		value = std::clamp(value, minValue, maxValue);
	}

	if (iconButton(id + "_inc", IC_CHEVRON_RIGHT, incBounds)) {
		value += step;
		value = std::clamp(value, minValue, maxValue);
	}

	bool clickedInside =
		mainBounds.hasPoint(m_state.mousePosition) &&
		m_state.mouseDown && focused;

	if (!numEdit.editingText) {
		if (clickedInside && m_state.mouseButton == QuickGUIState::Left && !numEdit.dragging) {
			numEdit.dragging = true;
			numEdit.prevPos = m_state.mousePosition;
			numEdit.actualValue = value;
		}
		else if (clickedInside && m_state.mouseButton == QuickGUIState::Right) {
			numEdit.editingText = true;
			numEdit.text = std::to_string(value);
		}
		else if (!m_state.mouseDown && numEdit.dragging) {
			numEdit.dragging = false;
		}

		if (numEdit.dragging) {
			auto delta = m_state.mousePosition - numEdit.prevPos;
			updateValue(numEdit, delta.x, value, step, minValue, maxValue);
			numEdit.prevPos = m_state.mousePosition;
		}
	}
	else {
		if (wid.keyPressed && (m_state.key == Key::ENTER || m_state.key == Key::RETURN)) {
			value = std::stof(numEdit.text);
			value = snap(value, step);
			value = std::clamp(value, minValue, maxValue);

			numEdit.editingText = false;
			numEdit.dragging = false;
		}
		else {
			lineEditor(wid, mainBounds, numEdit.edit, numEdit.text, m_state, labelOffset);
		}

		if (!mainBounds.hasPoint(m_state.mousePosition) && m_state.mouseDown) {
			value = std::stof(numEdit.text);
			value = snap(value, step);
			value = std::clamp(value, minValue, maxValue);

			numEdit.editingText = false;
			numEdit.dragging = false;
		}
	}
}

void QuickGUI::colorPicker(const std::string& id, Rect bounds, Color& color) {
	const float hueBarWidth = 18.0f;
	auto ctx = context();
	auto wd = widget(id, bounds);
	bool focused = m_state.focusedWidget == wd.id;

	auto pos = m_colorPickers.find(wd.id);
	if (pos == m_colorPickers.end()) {
		m_colorPickers[wd.id] = ColorPicker();
	}

	auto& cpicker = m_colorPickers[wd.id];
	rgbToHSV(color[0], color[1], color[2], cpicker.hsv[0], cpicker.hsv[1], cpicker.hsv[2]);

	nvgSave(ctx);

	m_styleSheet.draw("panel", ctx, bounds, "", 0);

	nvgTranslate(ctx, bounds.x, bounds.y);

	Rect b = bounds;
	b.expand(-5);

	float cx = b.width * 0.5f;
	float cy = b.height * 0.5f;
	float r1 = (b.width < b.height ? b.width : b.height) * 0.5f - 5.0f;
	float r0 = r1 - hueBarWidth;
	float aeps = 0.5f / r1;

#pragma region Rendering
	for (int i = 0; i < 6; i++) {
		float a0 = (float)i / 6.0f * NVG_PI * 2.0f - aeps;
		float a1 = (float)(i + 1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
		nvgBeginPath(ctx);
		nvgArc(ctx, cx, cy, r0, a0, a1, NVG_CW);
		nvgArc(ctx, cx, cy, r1, a1, a0, NVG_CCW);
		nvgClosePath(ctx);

		float ax = cx + ::cosf(a0) * (r0 + r1) * 0.5f;
		float ay = cy + ::sinf(a0) * (r0 + r1) * 0.5f;
		float bx = cx + ::cosf(a1) * (r0 + r1) * 0.5f;
		float by = cy + ::sinf(a1) * (r0 + r1) * 0.5f;

		NVGpaint paint = nvgLinearGradient(ctx, ax, ay, bx, by, nvgHSLA(a0 / (NVG_PI * 2), 1.0f, 0.55f, 255), nvgHSLA(a1 / (NVG_PI * 2), 1.0f, 0.55f, 255));
		nvgFillPaint(ctx, paint);
		nvgFill(ctx);
	}

	nvgBeginPath(ctx);
	nvgCircle(ctx, cx, cy, r0 - 0.5f);
	nvgCircle(ctx, cx, cy, r1 + 0.5f);
	nvgStrokeColor(ctx, nvgRGBA(0, 0, 0, 64));
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);

	// Selector
	nvgSave(ctx);
	nvgTranslate(ctx, cx, cy);
	nvgRotate(ctx, cpicker.hsv[0] * NVG_PI * 2);

	// Marker on
	nvgStrokeWidth(ctx, 2.0f);
	nvgBeginPath(ctx);
	nvgRect(ctx, r0 - 1, -3, r1 - r0 + 2, 6);
	nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 192));
	nvgStroke(ctx);

	NVGpaint paint = nvgBoxGradient(ctx, r0 - 3, -5, r1 - r0 + 6, 10, 2, 4, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(ctx);
	nvgRect(ctx, r0 - 2 - 10, -4 - 10, r1 - r0 + 4 + 20, 8 + 20);
	nvgRect(ctx, r0 - 2, -4, r1 - r0 + 4, 8);
	nvgPathWinding(ctx, NVG_HOLE);
	nvgFillPaint(ctx, paint);
	nvgFill(ctx);

	nvgRestore(ctx);

	nvgSave(ctx);
	nvgTranslate(ctx, cx, cy);
	// current color view
	nvgBeginPath(ctx);

	nvgMoveTo(ctx, -r1, r1);
	nvgLineTo(ctx, -r1 + 32.0f, r1);
	nvgLineTo(ctx, -r1, r1 - 32.0f);
	nvgClosePath(ctx);

	nvgFillColor(ctx, nvgRGBf(color[0], color[1], color[2]));
	nvgFill(ctx);

	nvgStrokeColor(ctx, nvgRGBf(1.0f, 1.0f, 1.0f));
	nvgStroke(ctx);

	// Center square
	float r = (r1 - 1) / 2.0f;

	nvgBeginPath(ctx);
	nvgRect(ctx, -r, -r, r * 2, r * 2);

	paint = nvgLinearGradient(ctx, -r, 0.0f, r, 0.0f, nvgRGBA(255, 255, 255, 255), nvgHSLA(cpicker.hsv[0], 1.0f, 0.5f, 255));
	nvgFillPaint(ctx, paint);
	nvgFill(ctx);

	paint = nvgLinearGradient(ctx, 0.0f, -r, 0.0f, r, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 255));
	nvgFillPaint(ctx, paint);
	nvgFill(ctx);

	nvgStrokeColor(ctx, nvgRGBA(0, 0, 0, 64));
	nvgStroke(ctx);
	//
	nvgRestore(ctx);

	nvgStrokeWidth(ctx, 2.0f);
	nvgBeginPath(ctx);
	nvgCircle(ctx, cpicker.selector.x, cpicker.selector.y, 4);
	nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 192));
	nvgStroke(ctx);

	Point center{ cx, cy };
	Point mouse = wd.relativeMouse;
	Point cm = (mouse - center);
	float dist = ::sqrtf(cm.dot(cm));

	Rect square{
		cx - r,
		cy - r,
		r * 2.0f,
		r * 2.0f
	};

	nvgRestore(ctx);
#pragma endregion

	bool clickedInside =
		bounds.hasPoint(m_state.mousePosition) &&
		m_state.mouseDown && focused;

	if (clickedInside && m_state.mouseButton == QuickGUIState::Left && cpicker.state == ColorPickerState::Idling) {
		if (square.hasPoint(mouse)) {
			cpicker.state = ColorPickerState::SatVal;
		}
		else if (dist >= r0 && dist <= r1) {
			cpicker.state = ColorPickerState::HueRing;
		}
	}

	switch (cpicker.state) {
		case ColorPickerState::SatVal: {
			cpicker.hsv[1] = std::clamp((mouse.x - square.x) / square.width, 0.0f, 1.0f);
			cpicker.hsv[2] = 1.0f - std::clamp((mouse.y - square.y) / square.height, 0.0f, 1.0f);
			hsvToRGB(cpicker.hsv[0], cpicker.hsv[1], cpicker.hsv[2], color[0], color[1], color[2]);
		} break;
		case ColorPickerState::HueRing: {
			const float twoPi = NVG_PI * 2.0f;
			float angle = ::atan2(cm.y, cm.x);
			if (angle < 0.0f) angle += twoPi;

			cpicker.hsv[0] = (angle / twoPi);

			hsvToRGB(cpicker.hsv[0], cpicker.hsv[1], cpicker.hsv[2], color[0], color[1], color[2]);
		} break;
		default: break;
	}

	cpicker.selector.x = square.x + cpicker.hsv[1] * square.width;
	cpicker.selector.y = square.y + (1.0f - cpicker.hsv[2]) * square.height;

	if (!m_state.mouseDown && focused) {
		cpicker.state = ColorPickerState::Idling;
	}
}

static void drawImage(
	NVGcontext* vg, int image, int iw, int ih,
	float alpha,
	float sx, float sy, float sw, float sh,
	float x, float y, float w, float h
)
{
	// Aspect ration of pixel in x an y dimensions. This allows us to scale
	// the sprite to fill the whole rectangle.
	float ax = w / sw;
	float ay = h / sh;

	NVGpaint img = nvgImagePattern(
		vg,
		x - sx * ax,
		y - sy * ay,
		(float)iw * ax,
		(float)ih * ay,
		0.0f, image, alpha
	);
	nvgBeginPath(vg);
	nvgRect(vg, x, y, w, h);
	nvgFillPaint(vg, img);
	nvgFill(vg);
}

Rect QuickGUI::image(uint32_t handle, Rect bounds, ImageFit fit) {
	auto ctx = context();

	auto pos = m_imageMap.find(handle);
	if (pos == m_imageMap.end()) {
		m_imageMap[handle] = nvglCreateImageFromHandleGL3(ctx, handle, 1920, 1080, NVG_IMAGE_NODELETE);
		pos = m_imageMap.find(handle);
	}

	m_styleSheet.draw("panel", ctx, bounds, "", 0);

	int iw, ih;
	nvgImageSize(ctx, pos->second, &iw, &ih);

	Rect innerBounds = bounds;
	innerBounds.expand(-6);

	float containerRatio = float(innerBounds.width) / innerBounds.height;
	float imageRatio = float(iw) / ih;

	Rect imageBounds = innerBounds;
	switch (fit) {
		case ImageFit::Contain: {
			if (imageRatio > containerRatio) {
				imageBounds.width = innerBounds.width;
				imageBounds.height = int32_t(innerBounds.width / imageRatio);
			}
			else {
				imageBounds.width = int32_t(innerBounds.height * imageRatio);
				imageBounds.height = innerBounds.height;
			}
		} break;
		case ImageFit::Cover: {
			if (imageRatio > containerRatio) {
				imageBounds.width = int32_t(innerBounds.height * imageRatio);
				imageBounds.height = innerBounds.height;
			}
			else {
				imageBounds.width = innerBounds.width;
				imageBounds.height = int32_t(innerBounds.width / imageRatio);
			}
		} break;
		default: break;
	}

	imageBounds.x = innerBounds.x + (innerBounds.width / 2 - imageBounds.width / 2);
	imageBounds.y = innerBounds.y + (innerBounds.height / 2 - imageBounds.height / 2);

	nvgSave(ctx);
	nvgIntersectScissor(ctx, innerBounds.x, innerBounds.y, innerBounds.width, innerBounds.height);
	drawImage(
		ctx,
		pos->second, iw, ih,
		1.0f, 0.0f, 0.0f, iw, ih,
		imageBounds.x, imageBounds.y, imageBounds.width, imageBounds.height
	);
	nvgRestore(ctx);

	return imageBounds; // returns the bounds of the image for using with widget (mouse events and whatnot)
}

static void drawRadioButton(
	NVGcontext* ctx,
	const std::string& text,
	float x, float y, float w, float h,
	float radiusLeft = 12.0f, float radiusRight = 12.0f,
	float bgBright = 0.0f,
	float borderAlpha = 0.0f)
{
	float bgAlpha = std::lerp(0.6f, 1.0f, bgBright);
	nvgBeginPath(ctx);
	nvgRoundedRectVarying(ctx, x, y, w, h, radiusLeft, radiusRight, radiusRight, radiusLeft);
	nvgFillColor(ctx, nvgRGBAf(bgBright, bgBright, bgBright, bgAlpha));
	if (borderAlpha >= 1e-5f) {
		nvgStrokeWidth(ctx, 1.0f);
		nvgStrokeColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, borderAlpha));
		nvgStroke(ctx);
	}
	nvgFill(ctx);

	nvgFillColor(ctx, nvgRGBf(1.0f - bgBright, 1.0f - bgBright, 1.0f - bgBright));
	nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFontSize(ctx, 14.0f);
	nvgTextBox(ctx, x, y + h / 2 + 1.5f, w, text.c_str(), nullptr);
}

bool QuickGUI::radioSelector(const std::string& id, Rect bounds, RadioButton* buttons, size_t count, size_t& selected) {
	auto ctx = context();
	auto root = widgetID(id);

	m_styleSheet.draw("panel", ctx, bounds, "", 0);

	auto el = m_styleSheet.fontSetup("button", ctx, bounds);

	auto buttonWidth = bounds.width / count;
	for (size_t i = 0; i < count; i++) {
		auto item = buttons[i];
		auto b = Rect(bounds.x + i * buttonWidth, bounds.y, buttonWidth, bounds.height);

		auto wd = widget(id + item.text, b);
		std::string style = "button_empty";
		switch (wd.state) {
			case WidgetState::Normal:
			case WidgetState::Focused: style = "button_empty"; break;
			case WidgetState::Hovered: style = "button_hover"; break;
			case WidgetState::Active: style = "button_active"; break;
			case WidgetState::Disabled: style = "button_empty"; break;
		}

		if (i == selected) {
			style = "button_hover";
		}

		if (!style.empty()) {
			m_styleSheet.draw(style, ctx, b, item.text, item.icon);
		}

		if (wd.clicked) {
			selected = i;
			return true;
		}
	}

	return false;
}

constexpr float scrollSize = 18.0f;
constexpr float contentPadding = 50.0f;

void QuickGUI::beginPanel(const std::string& id, Rect bounds) {
	auto ctx = context();
	auto root = widgetID(id);

	auto pos = m_panels.find(root);
	if (pos == m_panels.end()) {
		m_panels[root] = Panel();
	}

	auto& panel = m_panels[root];
	
	m_styleSheet.draw("panel_hollow", ctx, bounds, "", 0);
	auto el = m_styleSheet.getElement("panel_hollow", ctx, bounds);

	bounds.y -= panel.scroll.y;

	PanelData data{
		.id = root,
		.bounds = bounds,
	};
	m_panelStack.push_back(data);
	panel.data = data;

	bounds.width -= (scrollSize + 5.0f);

	layoutPushBounds(bounds);

	nvgSave(ctx);
	nvgIntersectScissor(ctx, bounds.x, bounds.y + panel.scroll.y, bounds.width, bounds.height);
}

void QuickGUI::endPanel() {
	auto ctx = context();

	PanelData data = m_panelStack.back();
	m_panelStack.pop_back();

	auto& panel = m_panels[data.id];
	Rect currentBounds = layoutPeek();

	float contentSize = (currentBounds.y - data.bounds.y) + contentPadding;
	float value = panel.scroll.y / contentSize;
	float pageScale = data.bounds.height / contentSize;

	Rect handleArea = Rect(
		data.bounds.x + data.bounds.width - scrollSize,
		data.bounds.y + panel.scroll.y,
		scrollSize, data.bounds.height
	);

	nvgResetScissor(ctx);
	m_styleSheet.draw("scroll_track", ctx, handleArea, "", 0);

	if (contentSize - contentPadding > 0.0f) {
		Rect handle;
		handle.x = handleArea.x;
		handle.y = handleArea.y + float(handleArea.height) * value;
		handle.width = handleArea.width;
		handle.height = (float(handleArea.height) * pageScale + 0.5f);

		auto wd = widget(std::to_string(panel.data.id) + "_thumb", handleArea);

		std::string style = "scroll_thumb";
		switch (wd.state) {
			case WidgetState::Normal:
			case WidgetState::Focused: style = "scroll_thumb"; break;
			case WidgetState::Hovered: style = "scroll_thumb_hover"; break;
			case WidgetState::Active: style = "scroll_thumb_active"; break;
			case WidgetState::Disabled: style = "scroll_thumb"; break;
		}
		m_styleSheet.draw(style, ctx, handle, "", 0);

		bool clickedInside =
			handleArea.hasPoint(m_state.mousePosition) &&
			m_state.mouseDown && m_state.activeWidget == wd.id;

		if (clickedInside && !panel.dragging) {
			panel.grabCoords = wd.relativeMouse;
			panel.dragging = true;
		}
		else if (!m_state.mouseDown && panel.dragging) {
			panel.dragging = false;
		}

		const float vmax = contentSize - data.bounds.height;

		if (vmax > 0.0f) {
			const float ratio = (contentSize / handleArea.height);
			if (panel.dragging) {
				panel.scroll.y = (wd.relativeMouse.y - panel.grabCoords.y + 1) * ratio;
			}
			panel.scroll.y = std::clamp(panel.scroll.y, 0.0f, vmax);
		}
	}

	nvgRestore(ctx);
	layoutPopBounds();
}

void QuickGUI::tabs(const std::string& id, Rect bounds, MenuItem* items, size_t numItems, size_t& selected) {
	const float buttonHorPad = 6.0f;
	const float buttonHeight = 24.0f;

	auto ctx = context();
	float x = bounds.x + buttonHorPad;
	float y = bounds.y;

	for (size_t i = 0; i < numItems; i++) {
		auto item = items[i];
		auto [tw, th] = m_styleSheet.calculateBounds("tab", ctx, item.text, item.icon);

		Rect wBounds = Rect(x, y, tw + buttonHorPad * 2, buttonHeight);

		if (x + wBounds.width + 1 >= bounds.width + bounds.x) {
			x = bounds.x + buttonHorPad;
			y += buttonHeight;

			wBounds.x = x;
			wBounds.y = y;
		}

		auto wd = widget(id + "#" + std::to_string(i), wBounds);

		std::string style = "tab";
		switch (wd.state) {
			case WidgetState::Normal:
			case WidgetState::Focused: style = "tab"; break;
			case WidgetState::Hovered: style = "tab_hover"; break;
			case WidgetState::Active: style = "tab_active"; break;
			case WidgetState::Disabled: style = "tab_disabled"; break;
		}

		if (i == selected) {
			style = "tab_hover";
		}

		m_styleSheet.draw(style, ctx, wBounds, item.text, item.icon);
		x += wBounds.width + 1;

		if (wd.clicked) {
			selected = i;
			break;
		}
	}

	m_styleSheet.draw("panel", ctx, Rect(bounds.x, y + buttonHeight, bounds.width, 2), "", 0);
}

void QuickGUI::renderPopups() {
	for (auto&& [wid, popup] : m_popups) {
		if (m_openPopup != wid) continue;

		const size_t itemHeight = 24;
		const size_t paddingX = 10;
		const size_t paddingY = 6;

		auto ctx = context();
		auto position = m_state.lastClickedPosition;

		Rect tmpBounds = Rect(0, 0, 1, 1);
		m_styleSheet.fontSetup("menu_item", ctx, tmpBounds);

		int width = 0;
		for (size_t i = 0; i < popup.items.size(); i++) {
			float bounds[4];
			nvgTextBounds(ctx, 0, 0, popup.items[i].text.c_str(), nullptr, bounds);

			int textWidth = int(bounds[2] - bounds[0]);
			if (popup.items[i].icon) {
				textWidth += iconSpaceWidth;
			}

			width = std::max(width, textWidth);
		}

		Rect bounds = Rect(
			position.x, position.y,
			width + paddingX * 2,
			itemHeight * popup.items.size() + paddingY * 2
		);

		m_styleSheet.draw("panel", ctx, bounds, "", 0);

		float y = bounds.y + paddingY;
		for (size_t i = 0; i < popup.items.size(); i++) {
			MenuItem item = popup.items[i];

			auto wd = getWidget(item.text + popup.id);

			std::string style = "menu_item";
			switch (wd.state) {
				case WidgetState::Normal:
				case WidgetState::Focused: style = "menu_item"; break;
				case WidgetState::Hovered: style = "menu_item_hover"; break;
				case WidgetState::Active: style = "menu_item_active"; break;
				case WidgetState::Disabled: style = "menu_item_disabled"; break;
			}

			Rect wdBounds = Rect(bounds.x, y, bounds.width, itemHeight);
			m_styleSheet.draw(style, ctx, wdBounds, item.text, item.icon);

			y += itemHeight;
		}
	}
}

Widget& QuickGUI::getWidget(const std::string& id) {
	const WidgetID wid = widgetID(id);

	auto&& pos = m_widgetState.find(wid);
	if (pos == m_widgetState.end()) {
		m_widgetState[wid] = Widget();
		pos = m_widgetState.find(wid);
	}

	return pos->second;
}

void QuickGUI::debugRect(Rect r) {
	auto ctx = context();
	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgRect(ctx, r.x, r.y, r.width, r.height);
	nvgStrokeColor(ctx, nvgRGB(0, 255, 0));
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);
	nvgRestore(ctx);
}

void QuickGUI::layoutPushBounds(Rect bounds) {
	m_layoutStack.push_back(bounds);
}

Rect QuickGUI::layoutPopBounds() {
	Rect rect = m_layoutStack.back();
	if (m_layoutStack.size() <= 1) return rect;
	m_layoutStack.pop_back();
	return rect;
}

Rect QuickGUI::layoutPeek() {
	return m_layoutStack.back();
}

std::vector<Rect> QuickGUI::layoutSliceHorizontal(
	int height,
	int columns,
	int gap
) {
	std::vector<Rect> ret;

	auto bounds = layoutCutTop(height);
	layoutPushBounds(bounds);

	int columnWidth = (bounds.width / columns);

	while (layoutPeek().width > 0) {
		Rect b = layoutCutLeft(columnWidth - gap);
		layoutCutLeft(gap);
		ret.push_back(b);
	}

	layoutPopBounds();

	return ret;
}

Rect QuickGUI::layoutCutTop(int height) {
	if (m_layoutStack.empty()) {
		throw std::runtime_error("layout stack is empty");
	}

	Rect parent = m_layoutStack.back(); m_layoutStack.pop_back();
	Rect newRect = Rect(parent.x, parent.y, parent.width, height);
	parent.y += height;
	parent.height -= height;

	m_layoutStack.push_back(parent);

	return newRect;
}

Rect QuickGUI::layoutCutBottom(int height) {
	if (m_layoutStack.empty()) {
		throw std::runtime_error("layout stack is empty");
	}

	Rect parent = m_layoutStack.back(); m_layoutStack.pop_back();
	parent.height -= height;

	Rect newRect = Rect(parent.x, parent.y + parent.height, parent.width, height);

	m_layoutStack.push_back(parent);

	return newRect;
}

Rect QuickGUI::layoutCutLeft(int width) {
	if (m_layoutStack.empty()) {
		throw std::runtime_error("layout stack is empty");
	}

	Rect parent = m_layoutStack.back(); m_layoutStack.pop_back();
	Rect newRect = Rect(parent.x, parent.y, width, parent.height);
	parent.x += width;
	parent.width -= width;

	m_layoutStack.push_back(parent);

	return newRect;
}

Rect QuickGUI::layoutCutRight(int width) {
	if (m_layoutStack.empty()) {
		throw std::runtime_error("layout stack is empty");
	}

	Rect parent = m_layoutStack.back(); m_layoutStack.pop_back();
	parent.width -= width;

	Rect newRect = Rect(parent.x + parent.width, parent.y, width, parent.height);

	m_layoutStack.push_back(parent);

	return newRect;
}
