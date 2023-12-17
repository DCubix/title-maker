#include "Internal.h"

#include <algorithm>

bool Rect::hasPoint(Point p) {
	return p.x >= x &&
		p.x <= x + width &&
		p.y >= y &&
		p.y <= y + height;
}

void Rect::expand(float amount) {
	x -= amount;
	y -= amount;
	width += amount * 2;
	height += amount * 2;
}

Rect Rect::fromMinMax(Point pmin, Point pmax) {
	if (pmin.x > pmax.x) std::swap(pmin.x, pmax.x);
	if (pmin.y > pmax.y) std::swap(pmin.y, pmax.y);
	return Rect(pmin.x, pmin.y, pmax.x - pmin.x, pmax.y - pmin.y);
}

void nvgApplyFillPaint(NVGcontext* ctx, Paint paint) {
	switch (paint.index()) {
		case 0: nvgFillPaint(ctx, std::get<NVGpaint>(paint)); break;
		default: auto col = std::get<Color>(paint); nvgFillColor(ctx, nvgColor(col)); break;
	}
}

void nvgApplyStrokePaint(NVGcontext* ctx, Paint paint) {
	switch (paint.index()) {
		case 0: nvgStrokePaint(ctx, std::get<NVGpaint>(paint)); break;
		default: auto col = std::get<Color>(paint); nvgStrokeColor(ctx, nvgColor(col)); break;
	}
}


