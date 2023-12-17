#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <memory>
#include <vector>

#include "../../QuickGUI/quickgui/QuickGUI.h"
#include "../../QuickGUI/quickgui/Icons.h"

#include "Renderer.h"
#include "Shape.h"
#include "Animation.h"
#include "NDIOutput.h"

enum class ManipulatorState {
	None = 0,
	SizingTL,
	SizingTR,
	SizingBL,
	SizingBR,
	SizingL,
	SizingR,
	SizingT,
	SizingB,
	Moving
};

struct Manipulator {
	ManipulatorState state{ ManipulatorState::None };
	Point prevPoint;
};

class QuickGUI_Impl : public QuickGUI {
public:
	void processEvent(void* e);
	Key translateKey(int keyCode);
	void setMousePosition(Point pos);

	bool viewport(
		const std::string& id, Rect bounds,
		int virtualWidth, int virtualHeight,
		const ShapeList& shapes,
		Shape** selected
	);

	void rectManipulator(
		Rect& bounds,
		float& rotation,
		Point mousePosition,
		Point mouseDelta,
		Rect viewportBounds,
		int virtualWidth, int virtualHeight
	);

	Point viewportToGui(Point p, int virtualWidth, int virtualHeight, std::optional<Rect> screen);
	Point guiToViewport(Point p, int virtualWidth, int virtualHeight, std::optional<Rect> screen);

	SDL_Window* window;

private:
	Point m_previousPosition{};
	Manipulator m_manipulator{};
};

class App {
public:
	int start(int argc, char** argv);

private:
	SDL_Window* m_window;
	SDL_GLContext m_context;

	std::unique_ptr<QuickGUI_Impl> m_gui;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<NDIOutput> m_ndiOutput;

	// App
	ShapeList m_shapes;
	Shape* m_selectedShape{ nullptr };
	//

	void drawMenu();
	void drawSideToolbar();
	void drawSideOptionsPanel();
	void drawBody();

	void drawMainView();

	void drawViewport();

	void drawOptionsPanel();
	void drawAnimationPanel();

	void mainLoop();
};
