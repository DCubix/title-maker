#pragma once

#include <cstdint>
#include <array>
#include <variant>

#include "../nanovg/nanovg.h"

struct Point {
	float x{ 0 }, y{ 0 };

	Point operator -(Point o) {
		return { x - o.x, y - o.y };
	}

	float dot(Point o) {
		return x * o.x + y * o.y;
	}
};
struct Rect {
	float x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };

	Rect() = default;
	Rect(float x, float y, float width, float height)
		: x(x), y(y), width(width), height(height) {}

	bool hasPoint(Point p);
	void expand(float amount);

	static Rect fromMinMax(Point pmin, Point pmax);
};

using Color = std::array<float, 4>;
using Paint = std::variant<NVGpaint, Color>;

void nvgApplyFillPaint(NVGcontext* ctx, Paint paint);
void nvgApplyStrokePaint(NVGcontext* ctx, Paint paint);

#define nvgColor(c) nvgRGBAf(c[0], c[1], c[2], c[3])
