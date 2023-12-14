#pragma once

#include <optional>
#include <vector>
#include <memory>

#include "../../QuickGUI/quickgui/Internal.h"
#include "../../QuickGUI/quickgui/QuickGUI.h"
#include "../../QuickGUI/nanovg/nanovg.h"

#include "Animation.h"

enum class ShapeAnimation : size_t {
	Enter = 0,
	Exit,
	Count
};

class Shape {
public:
	virtual void draw(NVGcontext* ctx) = 0;
	virtual void gui(QuickGUI* gui) {}

	void drawAnimated(NVGcontext* ctx, float deltaTime);
	void triggerEnter();
	void triggerExit();

	Rect bounds{};

	std::unique_ptr<Animation> animations[size_t(ShapeAnimation::Count)];

private:
	enum _State {
		Idling = 0,
		Entering,
		Exiting
	};
	
	_State m_state{ Idling };
	_State m_nextState{ Idling };

	float m_globalTime{ 0.0f };

	void triggerAnim();
};
using ShapeList = std::vector<std::unique_ptr<Shape>>;

class ColoredShape : public Shape {
public:
	void gui(QuickGUI* gui);
	void applyFill(NVGcontext* ctx);

	enum FillMode {
		SolidColor = 0,
		Gradient
	} fillMode{ SolidColor };

	struct {
		Color color[2]{ Color{0.0f, 0.0f, 0.0f, 1.0f}, Color{1.0f, 1.0f, 1.0f, 1.0f} };
		Point stops[2];
	} background;

	float borderWidth{ 0.0f };
	Color borderColor;
};

class Rectangle : public ColoredShape {
public:
	void draw(NVGcontext* ctx);

	float borderRadius{ 0.0f };
};

class Ellipse : public ColoredShape {
public:
	void draw(NVGcontext* ctx);
};

class Text : public ColoredShape {
public:
	void draw(NVGcontext* ctx);
	void gui(QuickGUI* gui);

	float fontSize{ 30.0f };
	std::string text{ "Text" };
	// TODO: Font loading... yikes!
};
