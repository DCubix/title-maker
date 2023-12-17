#include "App.h"

#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image_write.h"
#include <format>

constexpr float hitTestExpand = 4.0f;

static const int keyTranslationTable[] = {
	SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f,
	SDLK_g, SDLK_h, SDLK_i, SDLK_j, SDLK_k, SDLK_l,
	SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q, SDLK_r,
	SDLK_s, SDLK_t, SDLK_u, SDLK_v, SDLK_w, SDLK_x,
	SDLK_y, SDLK_z,
	SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
	SDLK_6, SDLK_7, SDLK_8, SDLK_9,
	SDLK_ESCAPE, SDLK_LCTRL, SDLK_LSHIFT, SDLK_LALT,
	SDLK_LGUI, SDLK_RCTRL, SDLK_RSHIFT, SDLK_RALT,
	SDLK_RGUI, SDLK_MENU, SDLK_LEFTBRACKET,
	SDLK_RIGHTBRACKET, SDLK_SEMICOLON, SDLK_COMMA,
	SDLK_PERIOD, SDLK_QUOTE, SDLK_SLASH, SDLK_BACKSLASH,
	SDLK_CARET, SDLK_EQUALS, SDLK_UNDERSCORE, SDLK_SPACE,
	SDLK_RETURN, SDLK_BACKSPACE, SDLK_TAB, SDLK_PAGEUP,
	SDLK_PAGEDOWN, SDLK_END, SDLK_HOME, SDLK_INSERT,
	SDLK_DELETE, SDLK_KP_PLUS, SDLK_KP_MINUS, SDLK_KP_MULTIPLY,
	SDLK_KP_DIVIDE, SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN,
	SDLK_KP_0, SDLK_KP_1, SDLK_KP_2, SDLK_KP_3, SDLK_KP_4,
	SDLK_KP_5, SDLK_KP_6, SDLK_KP_7, SDLK_KP_8, SDLK_KP_9,
	SDLK_KP_ENTER, SDLK_KP_CLEAR, SDLK_CAPSLOCK, SDLK_NUMLOCKCLEAR,
	SDLK_PRINTSCREEN, SDLK_SCROLLLOCK,
	SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7,
	SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14,
	SDLK_F15, SDLK_PAUSE, SDLK_PLUS, SDLK_MINUS, SDLK_ASTERISK
};

void QuickGUI_Impl::processEvent(void* e) {
	SDL_Event* ev = static_cast<SDL_Event*>(e);

	switch (ev->type) {
		case SDL_MOUSEBUTTONDOWN: {
			m_state.mouseDown = true;
			switch (ev->button.button) {
				case SDL_BUTTON_LEFT: m_state.mouseButton = QuickGUIState::Left; break;
				case SDL_BUTTON_MIDDLE: m_state.mouseButton = QuickGUIState::Middle; break;
				case SDL_BUTTON_RIGHT: m_state.mouseButton = QuickGUIState::Right; break;
			}
		} break;
		case SDL_MOUSEBUTTONUP: m_state.mouseDown = false; break;
		case SDL_MOUSEMOTION: {
			m_state.mousePosition.x = ev->motion.x;
			m_state.mousePosition.y = ev->motion.y;
			m_state.mouseDelta.x = m_previousPosition.x - ev->motion.x;
			m_state.mouseDelta.y = m_previousPosition.y - ev->motion.y;
			m_previousPosition.x = ev->motion.x;
			m_previousPosition.y = ev->motion.y;
		} break;
		case SDL_TEXTINPUT:
		case SDL_KEYDOWN: {
			m_state.keyDown = true;
			m_state.key = translateKey(ev->key.keysym.sym);
			m_state.typedChar = ev->text.text[0];
		} break;
		case SDL_KEYUP: {
			m_state.keyUp = true;
			m_state.key = translateKey(ev->key.keysym.sym);
		} break;
		case SDL_MOUSEWHEEL: {
			m_state.mouseScroll = float(ev->wheel.y);
		} break;
	}
}

Key QuickGUI_Impl::translateKey(int keyCode) {
	auto key = std::find(std::begin(keyTranslationTable), std::end(keyTranslationTable), keyCode);
	if (key == std::end(keyTranslationTable)) {
		SDL_Log("Unknown key code: %d (%s)", keyCode, SDL_GetKeyName(keyCode));
		return Key::UNKNOWN;
	}

	int index = key - std::begin(keyTranslationTable);
	return Key(index);
}

void QuickGUI_Impl::setMousePosition(Point pos) {
	SDL_WarpMouseInWindow(window, pos.x, pos.y);
}

static void drawCross(NVGcontext* ctx, Point p, Color col) {
	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, p.x - 10, p.y);
	nvgLineTo(ctx, p.x + 10, p.y);
	nvgMoveTo(ctx, p.x, p.y - 10);
	nvgLineTo(ctx, p.x, p.y + 10);
	nvgStrokeColor(ctx, nvgColor(col));
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);
	nvgRestore(ctx);
}

static void drawVec(NVGcontext* ctx, float x, float y, float vx, float vy, Color col) {
	float len = ::sqrtf(vx * vx + vy * vy);
	vx /= len;
	vy /= len;

	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgCircle(ctx, x, y, 2.2f);
	nvgMoveTo(ctx, x, y);
	nvgLineTo(ctx, x + vx * 16.0f, y + vy * 16.0f);
	nvgStrokeColor(ctx, nvgColor(col));
	nvgStrokeWidth(ctx, 1.2f);
	nvgStroke(ctx);
	nvgRestore(ctx);
}

static void drawRect(NVGcontext* ctx, Rect b, Color col) {
	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgRect(ctx, b.x, b.y, b.width, b.height);
	nvgStrokeColor(ctx, nvgColor(col));
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);
	nvgRestore(ctx);
}

static Point transformMouse(Point mousePosition, Rect bounds, float rotation) {
	Point boundsOrigin = bounds.location();
	Point localMouse = mousePosition - bounds.location() - bounds.size() * 0.5f;
	return localMouse.rotated(-rotation) + bounds.size() * 0.5f;
}

bool QuickGUI_Impl::viewport(
	const std::string& id, Rect bounds, int virtualWidth, int virtualHeight,
	const ShapeList& shapes,
	Shape** selected
) {
	auto ctx = context();
	auto wid = widget(id, bounds);
	auto p = wid.relativeMouse;

	nvgSave(ctx);
	nvgTranslate(ctx, bounds.x, bounds.y);

	if (*selected) {
		rectManipulator(
			(*selected)->bounds,
			(*selected)->rotation,
			p, wid.relativeDelta,
			bounds,
			virtualWidth, virtualHeight
		);
	}

	drawCross(ctx, p, Color{ 0.0f, 1.0f, 1.0f, 1.0f });

	nvgRestore(ctx);

	if (wid.clicked && m_state.mouseButton == QuickGUIState::Left) {
		bool clickedShape = false;
		for (auto&& shape : shapes) {
			Rect b = shape->bounds;
			
			float rotation = nvgDegToRad(shape->rotation);

			Point minGui = viewportToGui({ b.x, b.y }, virtualWidth, virtualHeight, bounds);
			Point maxGui = viewportToGui({ b.x + b.width, b.y + b.height }, virtualWidth, virtualHeight, bounds);
			Rect vpBounds = Rect::fromMinMax(minGui, maxGui);
			Point vpBoundsOrigin = vpBounds.size() * 0.5f;
			Point vpBoundsPos = vpBounds.location() + vpBoundsOrigin;
			Rect vpBoundsLocal = vpBounds - vpBoundsPos;

			Transform xform = (Transform::rotation(rotation) * Transform::translation(vpBoundsPos)).inverted();

			// mouse position
			Point mousePos = xform * p;

			if (vpBoundsLocal.hasPoint(mousePos)) {
				*selected = shape.get();
				clickedShape = true;
				break;
			}
		}
		if (!clickedShape) {
			*selected = nullptr;
			m_manipulator.state = ManipulatorState::None;
		}
		return clickedShape;
	}

	return false;
}

static void drawPointManipulator(NVGcontext* ctx, Point position, float size, Color color) {
	nvgBeginPath(ctx);
	nvgCircle(ctx, position.x, position.y, size);

	nvgFillColor(ctx, nvgColor(color));
	nvgFill(ctx);

	nvgStrokeWidth(ctx, 1.2f);
	nvgStrokeColor(ctx, nvgRGB(0, 0, 0));
	nvgStroke(ctx);
}

void QuickGUI_Impl::rectManipulator(
	Rect& bounds,
	float& rotDeg,
	Point mousePosition,
	Point mouseDelta,
	Rect viewportBounds,
	int virtualWidth, int virtualHeight
) {
	const float size = 10.0f;
	const float hsize = size / 2.0f;

	struct {
		Point loc;
		ManipulatorState state;
	} points[8] = {
		{ { -1, -1 }, ManipulatorState::SizingTL },
		{ {  0, -1 }, ManipulatorState::SizingT  },
		{ {  1, -1 }, ManipulatorState::SizingTR },
		{ { -1,  1 }, ManipulatorState::SizingBL },
		{ {  0,  1 }, ManipulatorState::SizingB  },
		{ {  1,  1 }, ManipulatorState::SizingBR },
		{ { -1,  0 }, ManipulatorState::SizingL  },
		{ {  1,  0 }, ManipulatorState::SizingR  }
	};

	auto ctx = context();
	float rotation = nvgDegToRad(rotDeg);

	Point minGui = viewportToGui({ bounds.x, bounds.y }, virtualWidth, virtualHeight, viewportBounds);
	Point maxGui = viewportToGui({ bounds.x + bounds.width, bounds.y + bounds.height }, virtualWidth, virtualHeight, viewportBounds);
	
	Rect vpBounds = Rect::fromMinMax(minGui, maxGui);
	Point vpBoundsOrigin = vpBounds.size() * 0.5f;
	Point vpBoundsPos = vpBounds.location();

	Rect vpBoundsDraw = Rect(
		-vpBoundsOrigin.x,
		-vpBoundsOrigin.y,
		vpBounds.width,
		vpBounds.height
	);

	// rectangle space
	Transform xformDraw = Transform::translation(vpBoundsPos) * Transform::rotation(rotation);
	Transform xform = (Transform::translation(vpBoundsPos) * Transform::rotation(rotation)).inverted();
	
	auto matText = std::format(
		"Offset: {:.2f},{:.2f} | X: {:.2f},{:.2f} | Y: {:.2f},{:.2f}",
		xform.m[4], xform.m[5],
		xform.m[0], xform.m[1],
		xform.m[2], xform.m[3]
	);
	nvgText(ctx, 20.0f, 60.0f, matText.c_str(), nullptr);

	drawVec(ctx, 20.0f, 100.0f, xform.m[0], xform.m[1], Color{ 0.0f, 1.0f, 1.0f, 1.0f });
	drawVec(ctx, 50.0f, 100.0f, xform.m[2], xform.m[3], Color{ 0.0f, 1.0f, 1.0f, 1.0f });

	// mouse position
	Point mousePos = xform * mousePosition;
	drawCross(ctx, mousePos, Color{ 1.0f, 1.0f, 1.0f, 1.0f });

	drawCross(ctx, xform * vpBoundsPos, Color{ 1.0f, 0.0f, 1.0f, 1.0f });

	drawRect(ctx, vpBoundsDraw, vpBoundsDraw.hasPoint(mousePos) ? Color{ 0.0f, 1.0f, 0.0f, 1.0f } : Color{ 1.0f, 1.0f, 0.0f, 1.0f });
	//Rect vpBoundsLocalHitTest = vpBoundsLocal;
	//vpBoundsLocalHitTest.expand(hitTestExpand);

	//Point localRotMouse = transformMouse(mousePosition, vpBounds, rotation);

	//drawCross(ctx, localRotMouse, Color{ 1.0f, 1.0f, 0.0f, 1.0f });
	//drawRect(ctx, vpBoundsLocalHitTest, Color{1.0f, 0.0f, 0.0f, 1.0f});

	//if (vpBoundsLocalHitTest.hasPoint(localRotMouse)) {
	//	drawRect(ctx, vpBoundsLocalHitTest, Color{ 0.0f, 1.0f, 0.0f, 1.0f });
	//}

	//Point mouseGui = guiToViewport(localRotMouse, virtualWidth, virtualHeight, viewportBounds);

	//Rect selBounds = vpBounds;
	//selBounds.expand(hitTestExpand);

	nvgSave(ctx);

	//nvgIntersectScissor(ctx, 0, 0, viewportBounds.width, viewportBounds.height);

	xformDraw.toNanoVG(ctx);

	nvgBeginPath(ctx);
	nvgRect(ctx, vpBoundsDraw.x, vpBoundsDraw.y, vpBoundsDraw.width, vpBoundsDraw.height);
	
	nvgSave(ctx);
	nvgStrokeWidth(ctx, 2.0f);

	nvgStrokeColor(ctx, nvgRGB(0, 0, 0));
	nvgLineStyle(ctx, NVG_LINE_SOLID);
	nvgStroke(ctx);

	nvgStrokeColor(ctx, nvgRGB(255, 255, 255));
	nvgLineStyle(ctx, NVG_LINE_DASHED);
	nvgStroke(ctx);
	
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, vpBoundsDraw.x + vpBoundsDraw.width / 2.0f, vpBoundsDraw.y);
	nvgLineTo(ctx, vpBoundsDraw.x + vpBoundsDraw.width / 2.0f, vpBoundsDraw.y - 32.0f);

	nvgStrokeColor(ctx, nvgRGB(0, 0, 0));
	nvgLineStyle(ctx, NVG_LINE_SOLID);
	nvgStroke(ctx);

	nvgStrokeColor(ctx, nvgRGB(255, 255, 255));
	nvgLineStyle(ctx, NVG_LINE_DASHED);
	nvgStroke(ctx);

	nvgRestore(ctx);

	Point halfSize = (vpBoundsDraw.size() * 0.5f);

	drawPointManipulator(ctx, Point{ 0.0f, 0.0f }, size / 3.0f, Color{ 0.2f, 0.6f, 1.0f, 1.0f });
	drawPointManipulator(ctx, Point{ 0.0f, -(halfSize.y + 32.0f) }, size / 2.0f, Color{ 0.2f, 1.0f, 0.3f, 1.0f });

	for (size_t i = 0; i < 8; i++) {
		auto p = points[i];
		Point pos = p.loc * halfSize;

		drawPointManipulator(ctx, pos, size / 2.0f, Color{ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	nvgRestore(ctx);

	//Point mouseDeltaVp = mouseGui - m_manipulator.prevPoint;
	//if (m_state.mouseDown) {
	//	switch (m_manipulator.state) {
	//		case ManipulatorState::None: {
	//			for (size_t i = 0; i < 8; i++) {
	//				auto p = points[i];
	
	//				Point loc = p.loc * halfSize + halfSize;
	//				Rect b = Rect(loc.x - hsize, loc.y - hsize, size, size);
	//				b.expand(6);

	//				if (b.hasPoint(localRotMouse)) {
	//					m_manipulator.state = p.state;
	//					m_manipulator.prevPoint = mouseGui;
	//					break;
	//				}
	//			}

	//			Rect wholeBounds = vpBounds;
	//			wholeBounds.expand(hitTestExpand * 2.0f);
	//			if (wholeBounds.hasPoint(localRotMouse)) {
	//				m_manipulator.state = ManipulatorState::Moving;
	//				m_manipulator.prevPoint = mouseGui;
	//			}
	//		} break;
	//		case ManipulatorState::SizingTL:
	//			bounds.x += mouseDeltaVp.x;
	//			bounds.y += mouseDeltaVp.y;
	//			bounds.width -= mouseDeltaVp.x;
	//			bounds.height -= mouseDeltaVp.y;
	//			if (bounds.width < 0) {
	//				bounds.width = 1;
	//			}
	//			break;
	//		case ManipulatorState::SizingTR:
	//			bounds.y += mouseDeltaVp.y;
	//			bounds.width += mouseDeltaVp.x;
	//			bounds.height -= mouseDeltaVp.y;
	//			if (bounds.width < 0) {
	//				bounds.width = 1;
	//			}
	//			break;
	//		case ManipulatorState::SizingT:
	//			bounds.y += mouseDeltaVp.y;
	//			bounds.height -= mouseDeltaVp.y;
	//			break;
	//		case ManipulatorState::SizingBL:
	//			bounds.x += mouseDeltaVp.x;
	//			bounds.width -= mouseDeltaVp.x;
	//			bounds.height += mouseDeltaVp.y;
	//			if (bounds.width < 0) {
	//				bounds.width = 1;
	//			}
	//			break;
	//		case ManipulatorState::SizingBR:
	//			bounds.width += mouseDeltaVp.x;
	//			bounds.height += mouseDeltaVp.y;
	//			if (bounds.width < 0) {
	//				bounds.width = 1;
	//			}
	//			break;
	//		case ManipulatorState::SizingB:
	//			bounds.height += mouseDeltaVp.y;
	//			break;
	//		case ManipulatorState::SizingL:
	//			vpBounds.x += mouseDeltaVp.x;
	//			vpBounds.width -= mouseDeltaVp.x;

	//			break;
	//		case ManipulatorState::SizingR:
	//			bounds.width += mouseDeltaVp.x;
	//			break;
	//		case ManipulatorState::Moving:
	//			bounds.x += mouseDeltaVp.x;
	//			bounds.y += mouseDeltaVp.y;
	//			break;
	//		default: break;
	//	}
	//}
	//else {
	//	m_manipulator.state = ManipulatorState::None;
	//}
	//m_manipulator.prevPoint = mouseGui;

	if (m_state.keyDown && m_state.key == Key::A) {
		rotDeg += 1.0f;
	}
}

Point QuickGUI_Impl::viewportToGui(Point p, int virtualWidth, int virtualHeight, std::optional<Rect> screen) {
	Rect sBounds = screen.value_or(Rect(0, 0, m_state.screenWidth, m_state.screenHeight));
	float fractX = float(p.x) / virtualWidth;
	float fractY = float(p.y) / virtualHeight;
	return { (fractX * sBounds.width), (fractY * sBounds.height) };
}

Point QuickGUI_Impl::guiToViewport(Point p, int virtualWidth, int virtualHeight, std::optional<Rect> screen) {
	Rect sBounds = screen.value_or(Rect(0, 0, m_state.screenWidth, m_state.screenHeight));
	float fractX = float(p.x) / sBounds.width;
	float fractY = float(p.y) / sBounds.height;
	return { (fractX * virtualWidth), (fractY * virtualHeight) };
}

int App::start(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

	m_window = SDL_CreateWindow(
		"Window",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1280, 720,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
	);
	m_context = SDL_GL_CreateContext(m_window);
	gladLoadGL();

	m_gui = std::make_unique<QuickGUI_Impl>();
	m_renderer = std::make_unique<Renderer>();
	m_renderer->setup(m_gui->context(), 1920, 1080);

	m_gui->window = m_window;

	m_ndiOutput = std::make_unique<NDIOutput>();

	mainLoop();

	m_ndiOutput->stop();

	SDL_GL_DeleteContext(m_context);
	SDL_DestroyWindow(m_window);
	SDL_Quit();

	return 0;
}

void App::drawMenu() {
	auto bounds = m_gui->layoutCutTop(40);
	bounds.expand(-4);
	m_gui->layoutPushBounds(bounds);

	if (m_gui->button("snap", "Snapshot", m_gui->layoutCutLeft(120), IC_CAMERA)) {
		auto data = m_renderer->target().readImage();
		stbi_write_png(
			"snapshot.png",
			m_renderer->target().width(),
			m_renderer->target().height(), 4,
			data.data(), m_renderer->target().width() * 4
		);
	}
	m_gui->layoutCutLeft(5);

	if (!m_ndiOutput->started()) {
		if (m_gui->button("ndi_start", "Start NDI", m_gui->layoutCutLeft(120), IC_WIFI)) {
			m_ndiOutput->start(m_renderer->target().width(), m_renderer->target().height());
		}
	}
	else {
		if (m_gui->button("ndi_stop", "Stop NDI", m_gui->layoutCutLeft(120), IC_WIFI)) {
			m_ndiOutput->stop();
		}
	}
	m_gui->layoutCutLeft(5);

	m_gui->layoutPopBounds();
}

void App::drawSideToolbar() {
	auto bounds = m_gui->layoutCutLeft(47);
	bounds.expand(-5);
	m_gui->layoutPushBounds(bounds);

	if (m_gui->iconButton("rect", IC_RECTANGLE, m_gui->layoutCutTop(32))) {
		auto rect = std::make_unique<Rectangle>();
		rect->bounds = Rect(100, 100, 200, 200);
		rect->background.color[0] = Color{ 1.0f, 0.0f, 0.0f, 1.0f };
		m_shapes.push_back(std::move(rect));
	}
	m_gui->layoutCutTop(5);

	if (m_gui->iconButton("ellipse", IC_RECORD, m_gui->layoutCutTop(32))) {
		auto ellip = std::make_unique<Ellipse>();
		ellip->bounds = Rect(0, 0, 200, 200);
		ellip->background.color[0] = Color{1.0f, 0.0f, 0.0f, 1.0f};
		m_shapes.push_back(std::move(ellip));
	}
	m_gui->layoutCutTop(5);

	if (m_gui->iconButton("text", IC_LIST, m_gui->layoutCutTop(32))) {
		auto txt = std::make_unique<Text>();
		txt->bounds = Rect(0, 0, 200, 70);
		txt->background.color[0] = Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		m_shapes.push_back(std::move(txt));
	}
	m_gui->layoutCutTop(5);

	m_gui->layoutPopBounds();
}

void App::drawSideOptionsPanel() {
	auto bounds = m_gui->layoutCutRight(300);
	bounds.expand(-3);

	m_gui->layoutPushBounds(bounds);

	MenuItem tabs[] = {
		{ IC_GEAR, "Options", {} },
		{ IC_VIDEO, "Animation", {} }
	};

	static size_t sidePanelSel = 0;

	m_gui->tabs("side_tabs", m_gui->layoutCutTop(26), tabs, 2, sidePanelSel);

	if (sidePanelSel == 0) drawOptionsPanel();
	else if (sidePanelSel == 1) drawAnimationPanel();
	
	m_gui->layoutPopBounds();
}

void App::drawBody() {
	drawSideToolbar();
	drawSideOptionsPanel();
	drawMainView();
}

void App::drawMainView() {
	MenuItem tabs[] = {
		{ IC_PENCIL, "Editor", {} },
		{ IC_MONITOR, "Presentation", {} }
	};

	static size_t mainPanelSel = 0;

	m_gui->tabs("main_tabs", m_gui->layoutCutTop(26), tabs, 2, mainPanelSel);

	if (mainPanelSel == 0) drawViewport();
	else if (mainPanelSel == 1) {}
}

void App::drawViewport() {
	auto bounds = m_gui->layoutPeek();
	Rect imgBounds = m_gui->image(m_renderer->target().textureId(), bounds, ImageFit::Contain);

	m_gui->viewport(
		"vp",
		imgBounds,
		m_renderer->target().width(), m_renderer->target().height(),
		m_shapes, &m_selectedShape
	);
}

void App::drawOptionsPanel() {
	m_gui->beginPanel("options_panel", m_gui->layoutPeek());
	if (m_selectedShape) {
		m_gui->text("Position", m_gui->layoutCutTop(19));
		{
			auto cols = m_gui->layoutSliceHorizontal(26, 2);
			m_gui->number("pos_x", cols[0], m_selectedShape->bounds.x, -9999.0f, 9999.0f, 1.0f, "{:.1f}", "X");
			m_gui->number("pos_y", cols[1], m_selectedShape->bounds.y, -9999.0f, 9999.0f, 1.0f, "{:.1f}", "Y");
		}
		m_gui->layoutCutTop(8);

		{
			m_gui->number("rot", m_gui->layoutCutTop(26), m_selectedShape->rotation, 0.0f, 359.9f, 1.0f, "{:.1f}deg", "Rotation");
		}
		m_gui->layoutCutTop(8);

		m_gui->text("Size", m_gui->layoutCutTop(19));
		{
			auto cols = m_gui->layoutSliceHorizontal(26, 2);
			m_gui->number("siz_x", cols[0], m_selectedShape->bounds.width, 0.0f, 9999.0f, 1.0f, "{:.1f}", "W");
			m_gui->number("siz_y", cols[1], m_selectedShape->bounds.height, 0.0f, 9999.0f, 1.0f, "{:.1f}", "H");
		}
		m_gui->layoutCutTop(8);

		m_selectedShape->gui(m_gui.get());
		m_gui->layoutCutTop(8);
	}
	else {
		m_gui->text("No selected element.", m_gui->layoutPeek());
	}
	m_gui->endPanel();
}

static MenuItem animationTypes[] = {
	{ 0, "None", {} },
	{ 0, "Fade", {} },
	{ 0, "Reveal", {} }
};

void App::drawAnimationPanel() {
	m_gui->beginPanel("animation_panel", m_gui->layoutPeek());

	if (m_selectedShape) {
		static size_t selectedAnimTypeEnter = 0;
		static size_t selectedAnimTypeExit = 0;

		m_gui->text("On Enter Animation", m_gui->layoutCutTop(19));
		if (m_gui->button("ani_enter_set", animationTypes[selectedAnimTypeEnter].text, m_gui->layoutCutTop(24), IC_LIST)) {
			m_gui->showPopup("ani_type_enter");
		}
		m_gui->layoutCutTop(5);

		auto&& enter = m_selectedShape->animations[size_t(ShapeAnimation::Enter)];
		if (enter) {
			enter->onGUI(m_gui.get(), "_onenter");

			m_gui->layoutCutTop(5);

			if (m_gui->button("play_enter", "Play Enter", m_gui->layoutCutTop(24), IC_PLAY)) {
				m_selectedShape->triggerEnter();
			}
		}

		m_gui->layoutCutTop(5);

		m_gui->text("On Exit Animation", m_gui->layoutCutTop(19));
		if (m_gui->button("ani_exit_set", animationTypes[selectedAnimTypeExit].text, m_gui->layoutCutTop(24), IC_LIST)) {
			m_gui->showPopup("ani_type_exit");
		}
		m_gui->layoutCutTop(5);

		auto&& exit = m_selectedShape->animations[size_t(ShapeAnimation::Exit)];
		if (exit) {
			exit->onGUI(m_gui.get(), "_onexit");

			m_gui->layoutCutTop(5);

			if (m_gui->button("play_exit", "Play Exit", m_gui->layoutCutTop(24), IC_PLAY)) {
				m_selectedShape->triggerExit();
			}
		}

		m_gui->layoutCutTop(5);
		
		if (m_gui->popup("ani_type_enter", animationTypes, 3, selectedAnimTypeEnter)) {
			switch (selectedAnimTypeEnter) {
				default: m_selectedShape->animations[size_t(ShapeAnimation::Enter)].reset(); break;
				case 1: m_selectedShape->animations[size_t(ShapeAnimation::Enter)].reset(new FadeAnimation()); break;
				case 2: m_selectedShape->animations[size_t(ShapeAnimation::Enter)].reset(new RevealAnimation()); break;
			}
		}
		
		if (m_gui->popup("ani_type_exit", animationTypes, 3, selectedAnimTypeExit)) {
			switch (selectedAnimTypeExit) {
				default: m_selectedShape->animations[size_t(ShapeAnimation::Exit)].reset(); break;
				case 1: m_selectedShape->animations[size_t(ShapeAnimation::Exit)].reset(new FadeAnimation()); break;
				case 2: m_selectedShape->animations[size_t(ShapeAnimation::Exit)].reset(new RevealAnimation()); break;
			}
		}
	}
	else {
		m_gui->text("No selected element.", m_gui->layoutPeek());
	}
	m_gui->endPanel();
}

void App::mainLoop() {
	const double timeStep = 1.0 / 30;
	double startTime = double(SDL_GetTicks()) / 1000.0;
	double accum = 0.0;

	bool running = true;

	while (running) {
		bool canRender = false;
		double currentTime = double(SDL_GetTicks()) / 1000.0;
		double delta = currentTime - startTime;
		startTime = currentTime;

		accum += delta;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (
				e.type == SDL_WINDOWEVENT &&
				e.window.event == SDL_WINDOWEVENT_CLOSE &&
				(SDL_GetWindowFromID(e.window.windowID) == m_window)
			) {
				running = false;
			}
			m_gui->processEvent(&e);
		}

		while (accum >= timeStep) {
			accum -= timeStep;

			canRender = true;
		}

		if (canRender) {
			m_renderer->render(m_shapes, timeStep);
			if (m_ndiOutput->started()) {
				m_ndiOutput->send(m_renderer->lastFrameData());
			}

			glClearColor(0.14f, 0.14f, 0.14f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			int w, h;
			SDL_GetWindowSize(m_window, &w, &h);
			m_gui->beginFrame(w, h);

			drawMenu();
			drawBody();

			m_gui->endFrame();
			SDL_GL_SwapWindow(m_window);
		}
	}
}
