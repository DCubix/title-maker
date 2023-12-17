#pragma once

#include <cstdint>
#include <array>
#include <variant>

#include "../nanovg/nanovg.h"

struct Point {
	float x{ 0 }, y{ 0 };

	Point operator +(Point o) {
		return Point{ x + o.x, y + o.y };
	}
	
	Point operator -(Point o) {
		return Point{ x - o.x, y - o.y };
	}

	Point operator *(float o) {
		return Point{ x * o, y * o };
	}

	Point operator *(Point o) {
		return Point{ x * o.x , y * o.y };
	}

	float dot(Point o) {
		return x * o.x + y * o.y;
	}

	Point rotated(float angle) {
		float c = ::cosf(angle), s = ::sinf(angle);
		return Point{
			.x = (x * c - y * s),
			.y = (x * s + y * c)
		};
	}

};

struct Rect {
	float x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };

	Rect() = default;
	Rect(float x, float y, float width, float height)
		: x(x), y(y), width(width), height(height) {}

	bool hasPoint(Point p);
	void expand(float amount);

	Rect operator -(Point o) {
		return Rect{ x - o.x, y - o.y, width, height };
	}

	Point location() const {
		return Point{ x, y };
	}

	Point size() const {
		return Point{ width, height };
	}

	static Rect fromMinMax(Point pmin, Point pmax);
};

struct Transform {
	float m[6];

	void toNanoVG(NVGcontext* ctx) {
		nvgTransform(ctx, m[0], m[1], m[2], m[3], m[4], m[5]);
	}

	static Transform fromNanoVG(NVGcontext* ctx) {
		Transform ret;
		nvgCurrentTransform(ctx, ret.m);
		return ret;
	}

	static Transform identity() {
		Transform ret;
		nvgTransformIdentity(ret.m);
		return ret;
	}

	static Transform rotation(float angle) {
		Transform ret;
		nvgTransformRotate(ret.m, angle);
		return ret;
	}

	static Transform scaling(float x, float y) {
		Transform ret;
		nvgTransformScale(ret.m, x, y);
		return ret;
	}

	static Transform translation(float x, float y) {
		Transform ret;
		nvgTransformTranslate(ret.m, x, y);
		return ret;
	}

	static Transform translation(Point p) {
		return Transform::translation(p.x, p.y);
	}

	Transform& inverted() {
		nvgTransformInverse(m, m);
		return *this;
	}

	Point operator *(Point p) {
		Point ret;
		nvgTransformPoint(&ret.x, &ret.y, m, p.x, p.y);
		return ret;
	}

	Transform operator *(Transform t) {
		Transform ret;
		::memcpy(ret.m, m, sizeof(float) * 6);
		nvgTransformPremultiply(ret.m, t.m);
		return ret;
	}
};

using Color = std::array<float, 4>;
using Paint = std::variant<NVGpaint, Color>;

void nvgApplyFillPaint(NVGcontext* ctx, Paint paint);
void nvgApplyStrokePaint(NVGcontext* ctx, Paint paint);

#define nvgColor(c) nvgRGBAf(c[0], c[1], c[2], c[3])
