#pragma once

#include "../glad/glad.h"

#include "../nanovg/nanovg.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <map>

#include "StyleSheet.h"

using WidgetID = std::size_t;
constexpr WidgetID InvalidWidget = WidgetID(0);

enum class WidgetState : uint8_t {
	Normal = 0,
	Hovered,
	Active,
	Focused,
	Disabled,
	Count
};

enum class Alignment : uint8_t {
	Left = 0,
	Center,
	Right
};

enum class Key : uint32_t {
	UNKNOWN = 0xFFFFFFFFU,
	A = 0,
	B, C, D, E, F, G, H,
	I, J, K, L, M, N, O, P, Q,
	R, S, T, U, V, W, X, Y, Z,
	NUM0, NUM1, NUM2, NUM3, NUM4,
	NUM5, NUM6, NUM7, NUM8, NUM9,
	ESC, LCONTROL, LSHIFT,
	LALT, LSYSTEM, RCONTROL, RSHIFT,
	RALT, RSYSTEM, MENU,
	LBRACKET, RBRACKET,
	SEMICOLON, COMMA, PERIOD,
	QUOTE, SLASH, BACKSLASH, TILDE,
	EQUAL, UNDERSCORE, SPACE, RETURN, BACKSPACE,
	TAB, PAGEUP, PAGEDOWN, END, HOME,
	INSERT, DELETE, ADD, SUBSTRACT, MULTIPLY,
	DIVIDE, LEFT, RIGHT, UP, DOWN,
	NUMPAD0, NUMPAD1, NUMPAD2, NUMPAD3,
	NUMPAD4, NUMPAD5, NUMPAD6, NUMPAD7,
	NUMPAD8, NUMPAD9, ENTER, DEL,
	CAPSLOCK, NUMLOCK, PRINTSCREEN, SCROLLLOCK,
	F1, F2, F3, F4, F5, F6, F7, F8,
	F9, F10, F11, F12, F13, F14, F15,
	PAUSE,
	PLUS, MINUS, ASTERISK,
	KEYCOUNT
};

enum class ImageFit {
	Stretch = 0,
	Contain,
	Cover
};

struct Widget {
	WidgetID id;
	Point relativeMouse, relativeDelta, position;
	bool clicked, keyPressed;
	WidgetState state;
};

struct QuickGUIState {
	int screenWidth, screenHeight;

	Point mousePosition{}, mouseDelta{}, lastClickedPosition{};
	float mouseScroll{ 0.0f };
	bool mouseDown{ false }, keyDown{ false }, keyUp{ false };
	enum {
		Left = 0,
		Middle,
		Right
	} mouseButton{ Left };

	Key key{ Key::UNKNOWN };
	int typedChar{ 0 };

	WidgetID
		activeWidget{ InvalidWidget },
		hoveredWidget{ InvalidWidget },
		focusedWidget{ InvalidWidget };
};

struct MenuItem {
	size_t icon;
	std::string text;
	std::vector<MenuItem> children;
};

struct TextEdit {
	std::vector<NVGglyphPosition> glyphs;

	int32_t cursor{ 0 };
	bool cursorShow{ false }, ctrl{ false };
	float blinkTime{ 0.0f };

	float viewOffset{ 0.0f };
};

struct NumberEdit {
	bool editingText{ false }, dragging{ false };
	TextEdit edit{};
	Point prevPos;
	std::string text;
	float actualValue{ 0.0f };
};

enum class ColorPickerState {
	Idling = 0,
	HueRing,
	SatVal
};

struct ColorPicker {
	ColorPickerState state{ ColorPickerState::Idling };
	Point selector;
	float hsv[3];
};

struct RadioButton {
	size_t icon{ 0 };
	std::string text;
};

struct PanelData {
	WidgetID id;
	Rect bounds;
};

struct Panel {
	Point scroll{ 0.0f, 0.0f };
	Point grabCoords{};
	bool dragging{ false };
	PanelData data;
};

struct Popup {
	std::string id;
	std::vector<MenuItem> items;
};

class QuickGUI {
public:
	~QuickGUI();

	virtual void startInput() {}
	virtual void endInput() {}

	virtual void processEvent(void* e) = 0;
	virtual Key translateKey(int keyCode) = 0;

	virtual void setMousePosition(Point pos) = 0;

	void beginFrame(uint32_t width, uint32_t height);
	void endFrame();

	NVGcontext* context();

	// layout stuff
	Rect layoutCutTop(int height);
	Rect layoutCutBottom(int height);
	Rect layoutCutLeft(int width);
	Rect layoutCutRight(int width);
	void layoutPushBounds(Rect bounds);
	Rect layoutPopBounds();
	Rect layoutPeek();

	std::vector<Rect> layoutSliceHorizontal(int height, int columns, int gap = 6);

	Widget widget(const std::string& id, Rect bounds, bool checkBlocked = true);

	void text(const std::string& text, Rect bounds, const std::string& style = "text");
	bool button(
		const std::string& id,
		const std::string& text,
		Rect bounds,
		size_t icon = 0
	);

	bool iconButton(
		const std::string& id,
		size_t icon,
		Rect bounds
	);

	void checkBox(const std::string& id, const std::string& text, Rect bounds, bool& checked);

	void showPopup(const std::string& id);
	bool popup(const std::string& id, MenuItem* items, size_t numItems, size_t& selected);

	void textEdit(
		const std::string& id,
		Rect bounds,
		std::string& text,
		const std::string& placeholder = ""
	);

	void number(
		const std::string& id,
		Rect bounds,
		float& value,
		float minValue = 0.0f,
		float maxValue = 1.0f,
		float step = 0.1f,
		const std::string& fmt = "{:.2f}",
		const std::string& label = ""
	);

	void colorPicker(
		const std::string& id,
		Rect bounds,
		Color& color
	);

	Rect image(
		uint32_t handle,
		Rect bounds,
		ImageFit fit = ImageFit::Stretch
	);

	bool radioSelector(const std::string& id, Rect bounds, RadioButton* buttons, size_t count, size_t& selected);

	void beginPanel(
		const std::string& id,
		Rect bounds
	);
	void endPanel();

	void tabs(const std::string& id, Rect bounds, MenuItem* items, size_t numItems, size_t& selected);

protected:
	void renderPopups();
	Widget& getWidget(const std::string& id);

	QuickGUIState m_state{};
	StyleSheet m_styleSheet;

	NVGcontext* m_context{ nullptr };

	int32_t m_fontNormal, m_fontItalic, m_fontBold, m_fontIcons;

	std::vector<Rect> m_layoutStack{};
	std::vector<PanelData> m_panelStack{};

	WidgetID m_openPopup{ InvalidWidget };

	std::map<WidgetID, TextEdit> m_textEdits;
	std::map<WidgetID, NumberEdit> m_numberEdits;
	std::map<WidgetID, ColorPicker> m_colorPickers;
	std::map<WidgetID, Panel> m_panels;
	std::map<WidgetID, Popup> m_popups;
	std::map<WidgetID, Widget> m_widgetState;
	std::map<GLuint, int> m_imageMap;

	bool m_inputBlocked{ false };

	void debugRect(Rect r);
	bool isMouseDownWidget(WidgetID wid) const { return m_state.hoveredWidget == wid && m_state.activeWidget == wid; }
};
