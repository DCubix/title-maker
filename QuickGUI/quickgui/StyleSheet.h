#pragma once

#include <variant>
#include <unordered_map>
#include <string>
#include <array>
#include <functional>
#include <cstdint>
#include <vector>
#include <optional>

#include "../nanovg/nanovg.h"

#include "Internal.h"

constexpr float iconSpaceWidth = 22.0f;

enum class PropertyType {
	None = 0,
	Number,
	Color,
	String,
	Function,
	Enum
};

enum class ParamType {
	None = 0,
	Number,
	Color,
	Enum
};

struct Param {
	ParamType type{ ParamType::None };
	std::variant<nullptr_t, std::string, float, Color> value{ nullptr };
};
using ParamList = std::array<Param, 16>;

using StyleFunctionValue = std::variant<nullptr_t, float, Color, NVGpaint>;
using StyleFunctionCallback = std::function<StyleFunctionValue(NVGcontext*, const ParamList&, Rect)>;
struct StyleFunction {
	StyleFunctionCallback callback;
	ParamList params;
};

using PropertyValue = std::variant<nullptr_t, std::string, float, Color, StyleFunction>;
struct Property {
	PropertyType type{ PropertyType::None };
	PropertyValue value;
};

struct PropertyPack {
	std::array<Property, 4> values{};

	const Property& operator[](size_t index) const { return values[index]; }
	Property& operator[](size_t index) { return values[index]; }

	size_t count() const;
};

struct Element {
	std::optional<Paint> backgroundPaint;
	std::optional<Paint> textPaint;
	std::optional<Paint> borderPaint;

	float borderWidth{ 0.0f }, fontSize{14.0f};
	int alignmentX{ 0 }, alignmentY{ 0 };
	float borderRadius[4]{ 0.0f };

	float padding[4]{ 0.0f };

	Rect bounds;
};
using ElementSetter = std::function<void(NVGcontext*, Element&, const PropertyPack&)>;

using Style = std::unordered_map<std::string, PropertyPack>;

class StyleSheet {
public:
	void parse(const std::string& styleSheetData);

	Element getElement(
		const std::string& style,
		NVGcontext* ctx,
		Rect& bounds
	);

	Element draw(
		const std::string& style,
		NVGcontext* ctx,
		Rect bounds,
		const std::string& text,
		size_t icon
	);

	Element fontSetup(const std::string& style, NVGcontext* ctx, Rect& bounds);

	std::pair<float, float> calculateBounds(const std::string& style, NVGcontext* ctx, const std::string& text, size_t icon);
private:
	std::unordered_map<std::string, Style> m_styles;
};
