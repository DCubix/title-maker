#include "StyleSheet.h"

#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <format>
#include <cassert>

#include "Icons.h"

static float hue2rgb(float p, float q, float t) {
	if (t < 0) t += 1;
	if (t > 1) t -= 1;
	if (t < 1. / 6) return p + (q - p) * 6 * t;
	if (t < 1. / 2) return q;
	if (t < 2. / 3) return p + (q - p) * (2. / 3 - t) * 6;
	return p;
}

static Color hsl2rgb(float h, float s, float l) {
	Color result;

	if (fabsf(s) <= 1e-5f) {
		result[0] = result[1] = result[2] = l; // achromatic
	}
	else {
		float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
		float p = 2.0f * l - q;
		result[0] = hue2rgb(p, q, h + 1.0f / 3.0f);
		result[1] = hue2rgb(p, q, h);
		result[2] = hue2rgb(p, q, h - 1.0f / 3.0f);
	}
	return result;
}

StyleFunctionValue style_Gradient(NVGcontext* ctx, const ParamList& params, Rect bounds) {
	float sx = std::get<float>(params[0].value) * bounds.width;
	float sy = std::get<float>(params[1].value) * bounds.height;
	float ex = std::get<float>(params[2].value) * bounds.width;
	float ey = std::get<float>(params[3].value) * bounds.height;
	Color colA = std::get<Color>(params[4].value);
	Color colB = std::get<Color>(params[5].value);
	return nvgLinearGradient(
		ctx,
		sx, sy, ex, ey,
		nvgRGBAf(colA[0], colA[1], colA[2], colA[3]),
		nvgRGBAf(colB[0], colB[1], colB[2], colB[3])
	);
}

static const std::unordered_map<std::string, StyleFunctionCallback> StyleFunctions = {
	{ "gradient", style_Gradient }
};
 
struct StringParser {
	std::vector<char> input;

	char next() {
		if (input.empty()) return 0;
		char c = input[0];
		input.erase(input.begin());
		return c;
	}

	char peek() {
		if (input.empty()) return 0;
		return input[0];
	}

	void skipSpaces() {
		while (peek() != 0 && isspace(peek())) next();
	}

	bool skipIfPresentAndSpaces(char c) {
		bool present = false;
		skipSpaces();
		if (peek() == c) {
			next();
			present = true;
		}
		else {
			throw std::runtime_error(std::format("expected a \"{}\".", c));
		}
		skipSpaces();
		return present;
	}

	std::string nextWhere(const std::function<bool(char)>& check) {
		std::string str = "";
		while (peek() != 0 && check(peek())) {
			str += next();
		}
		return str;
	}

	std::string nextIndentifier() {
		return nextWhere([](char c) { return isalnum(c) || c == '_' || c == '-'; });
	}

	std::string peekIndentifier() {
		std::string id = nextIndentifier();
		for (size_t i = id.size(); i-- > 0;) {
			input.insert(input.begin(), id[i]);
		}
		return id;
	}

	float nextNumber() {
		if (!isdigit(peek()) && peek() != '-') {
			throw std::runtime_error("expected a number string.");
		}

		std::string str = nextWhere([](char c) { return isdigit(c) || c == '.' || c == '-'; });
		return std::stof(str);
	}

	uint8_t nextHexByte() {
		if (!isxdigit(peek())) {
			throw std::runtime_error("expected a hexadecimal string.");
		}

		std::string str = "";
		str += next();
		str += next();
		return (uint8_t)std::stol(str, nullptr, 16);
	}

	Color nextColor() {
		// parse #000000, rgb, rgba, hsv
		if (peek() == '#') {
			next();
			Color ret;
			ret[0] = float(nextHexByte()) / 255.0f;
			ret[1] = float(nextHexByte()) / 255.0f;
			ret[2] = float(nextHexByte()) / 255.0f;
			ret[3] = 1.0f;
			return ret;
		}
		else if (isalpha(peek())) {
			auto colFn = nextIndentifier();
			std::transform(colFn.begin(), colFn.end(), colFn.begin(), tolower);

			skipIfPresentAndSpaces('(');

			Color ret{};
			if (colFn == "rgb") {
				ret[0] = nextNumber() / 255.0f; skipIfPresentAndSpaces(',');
				ret[1] = nextNumber() / 255.0f; skipIfPresentAndSpaces(',');
				ret[2] = nextNumber() / 255.0f;
				ret[3] = 1.0f;
			}
			else if (colFn == "rgba") {
				ret[0] = nextNumber() / 255.0f; skipIfPresentAndSpaces(',');
				ret[1] = nextNumber() / 255.0f; skipIfPresentAndSpaces(',');
				ret[2] = nextNumber() / 255.0f; skipIfPresentAndSpaces(',');
				ret[3] = nextNumber();
			}
			else if (colFn == "hsl") {
				float h = nextNumber(); skipIfPresentAndSpaces(',');
				float s = nextNumber(); skipIfPresentAndSpaces(',');
				float l = nextNumber();
				ret = hsl2rgb(h, s, l);
				ret[3] = 1.0f;
			}
			else if (colFn == "hsla") {
				float h = nextNumber(); skipIfPresentAndSpaces(',');
				float s = nextNumber(); skipIfPresentAndSpaces(',');
				float l = nextNumber(); skipIfPresentAndSpaces(',');
				ret = hsl2rgb(h, s, l);
				ret[3] = nextNumber();
			}
			else {
				throw std::runtime_error("invalid color string.");
			}

			skipIfPresentAndSpaces(')');

			return ret;
		}
	}

	std::string nextString() {
		if (peek() != '\'' || peek() != '"') {
			throw std::runtime_error("expected a \" before string.");
		}
		char quote = next();

		std::string str = nextWhere([quote](char c) { return c != quote; });
		skipIfPresentAndSpaces(quote);

		return str;
	}

	Param nextParam() {
		Param ret{};
		ret.type = ParamType::None;
		ret.value = nullptr;

		if (isdigit(peek())) {
			ret.type = ParamType::Number;
			ret.value = nextNumber();
			return ret;
		}
		else if (peek() == '#') {
			ret.type = ParamType::Color;
			ret.value = nextColor();
			return ret;
		}
		else if (isalpha(peek())) {
			auto id = peekIndentifier();
			std::transform(id.begin(), id.end(), id.begin(), tolower);

			if (id == "rgb" || id == "rgba" || id == "hsl" || id == "hsla") {
				ret.type = ParamType::Color;
				ret.value = nextColor();
				return ret;
			}
			else {
				id = nextIndentifier();
				skipSpaces();

				ret.type = ParamType::Enum;
				ret.value = id;
				return ret;
			}
		}
		throw std::runtime_error("invalid param value.");
	}

	PropertyValue nextPropertyValue(PropertyType& type) {
		if (isdigit(peek())) {
			type = PropertyType::Number;
			return nextNumber();
		}
		else if (peek() == '#') {
			type = PropertyType::Color;
			return nextColor();
		}
		else if (isalpha(peek())) {
			auto id = peekIndentifier();
			std::transform(id.begin(), id.end(), id.begin(), tolower);

			if (id == "rgb" || id == "rgba" || id == "hsl" || id == "hsla") {
				type = PropertyType::Color;
				return nextColor();
			}
			else {
				id = nextIndentifier();
				skipSpaces();

				if (peek() == '(') {
					next();
					skipSpaces();

					type = PropertyType::Function;

					StyleFunction fun{};
					fun.callback = StyleFunctions.at(id);

					size_t i = 0;
					while (peek() != ')' && !input.empty()) {
						auto param = nextParam();
						skipSpaces();
						if (peek() != ')') skipIfPresentAndSpaces(',');
						fun.params[i++] = param;
					}
					skipIfPresentAndSpaces(')');

					return fun;
				}
				else {
					type = PropertyType::Enum;
					return id;
				}
			}
		}
		else if (peek() == '\'' || peek() == '"') {
			type = PropertyType::String;
			return nextString();
		}
		else {
			throw std::runtime_error("invalid property value.");
		}
	}

	PropertyPack nextPropertyPack() {
		PropertyPack ret{};

		size_t i = 0;
		while (peek() != ';' && i < 4) {
			Property prop;
			prop.value = nextPropertyValue(prop.type);
			ret[i++] = prop;

			if (peek() == ' ') {
				skipSpaces();
			}
			else if (peek() == ';') {
				break;
			}
		}

		return ret;
	}
};

void StyleSheet::parse(const std::string& styleSheetData) {
	StringParser sp{};
	sp.input = std::vector<char>(styleSheetData.begin(), styleSheetData.end());

	while (!sp.input.empty()) {
		sp.skipSpaces();

		// Read style name
		if (isalpha(sp.peek())) {
			std::string styleName = sp.nextIndentifier();
			sp.skipIfPresentAndSpaces('{');

			Style style{};

			// parse properties
			while (sp.peek() != '}') {
				if (sp.peek() == '@') {
					// inherit from a style
					sp.next();
					auto inherited = sp.nextIndentifier();
					sp.skipIfPresentAndSpaces(';');

					auto pos = m_styles.find(inherited);
					if (pos != m_styles.end()) {
						for (auto [k, v] : pos->second) {
							style[k] = v;
						}
					}
				}
				else {
					std::string propName = sp.nextIndentifier();
					sp.skipIfPresentAndSpaces(':');

					auto value = sp.nextPropertyPack();
					sp.skipIfPresentAndSpaces(';');

					style[propName] = value;
				}
			}
			sp.skipIfPresentAndSpaces('}');

			m_styles[styleName] = style;
		}
	}
}

#define elementSetter(name) void element_set##name(NVGcontext* ctx, Element& el, const PropertyPack& props)

elementSetter(Color) {
	assert(props[0].type == PropertyType::Color);

	auto color = std::get<Color>(props[0].value);
	el.textPaint = color;
}

elementSetter(Background) {
	assert(props[0].type == PropertyType::Color || props[0].type == PropertyType::Function);

	if (props[0].type == PropertyType::Color) {
		auto col = std::get<Color>(props[0].value);
		el.backgroundPaint = col;
	}
	else {
		auto fn = std::get<StyleFunction>(props[0].value);
		el.backgroundPaint = std::get<NVGpaint>(fn.callback(ctx, fn.params, el.bounds));
	}
}

elementSetter(BorderRadius) {
	assert(props[0].type == PropertyType::Number);

	switch (props.count()) {
		// 1 value = all corners
		case 1:
			std::fill(std::begin(el.borderRadius), std::end(el.borderRadius), std::get<float>(props[0].value));
			break;
		// 4 value = tl  tr  br  bl
		case 4:
			el.borderRadius[0] = std::get<float>(props[0].value);
			el.borderRadius[1] = std::get<float>(props[1].value);
			el.borderRadius[2] = std::get<float>(props[2].value);
			el.borderRadius[3] = std::get<float>(props[3].value);
			break;
		default: break;
	}
}

elementSetter(BorderWidth) {
	assert(props[0].type == PropertyType::Number);

	el.borderWidth = std::get<float>(props[0].value);
}

elementSetter(BorderColor) {
	assert(props[0].type == PropertyType::Color);

	auto color = std::get<Color>(props[0].value);
	el.borderPaint = color;
}

elementSetter(FontSize) {
	assert(props[0].type == PropertyType::Number);

	el.fontSize = std::get<float>(props[0].value);
}

elementSetter(TextAlign) {
	assert(props[0].type == PropertyType::Enum);

	auto align = std::get<std::string>(props[0].value);
	if (align == "left") el.alignmentX = NVG_ALIGN_LEFT;
	else if (align == "center") el.alignmentX = NVG_ALIGN_CENTER;
	else if (align == "right") el.alignmentX = NVG_ALIGN_RIGHT;
}

elementSetter(VerticalAlign) {
	assert(props[0].type == PropertyType::Enum);

	auto align = std::get<std::string>(props[0].value);
	if (align == "top") el.alignmentY = NVG_ALIGN_TOP;
	else if (align == "middle") el.alignmentY = NVG_ALIGN_MIDDLE;
	else if (align == "bottom") el.alignmentY = NVG_ALIGN_BOTTOM;
}

elementSetter(Padding) {
	assert(props[0].type == PropertyType::Number);
	assert(props[1].type == PropertyType::Number);
	assert(props[2].type == PropertyType::Number);
	assert(props[3].type == PropertyType::Number);

	el.padding[0] = std::get<float>(props[0].value);
	el.padding[1] = std::get<float>(props[1].value);
	el.padding[2] = std::get<float>(props[2].value);
	el.padding[3] = std::get<float>(props[3].value);
}

static const std::unordered_map<std::string, ElementSetter> ElementSetters = {
	{ "color", element_setColor },
	{ "background", element_setBackground },
	{ "border-radius", element_setBorderRadius },
	{ "border-width", element_setBorderWidth },
	{ "border-color", element_setBorderColor },
	{ "font-size", element_setFontSize },
	{ "text-align", element_setTextAlign },
	{ "vertical-align", element_setVerticalAlign },
	{ "padding", element_setPadding }
};

Element StyleSheet::draw(const std::string& style, NVGcontext* ctx, Rect bounds, const std::string& text, size_t icon) {
	Rect innerBounds = Rect(0, 0, bounds.width, bounds.height);
	Element element = getElement(style, ctx, innerBounds);

	nvgSave(ctx);
	nvgTranslate(ctx, bounds.x, bounds.y);

	nvgFontSize(ctx, element.fontSize);

	if (element.backgroundPaint.has_value() || element.borderPaint.has_value()) {
		nvgBeginPath(ctx);

		auto br = element.borderRadius;
		if (br[0] > 0.0f || br[1] > 0.0f || br[2] > 0.0f || br[3] > 0.0f) {
			nvgRoundedRectVarying(
				ctx, 0.0f, 0.0f,
				bounds.width, bounds.height,
				br[0], br[1], br[2], br[3]
			);
		}
		else {
			nvgRect(ctx, 0.0f, 0.0f, bounds.width, bounds.height);
		}

		if (element.backgroundPaint.has_value()) {
			nvgApplyFillPaint(ctx, element.backgroundPaint.value());
			nvgFill(ctx);
		}

		if (element.borderPaint.has_value()) {
			nvgApplyStrokePaint(ctx, element.borderPaint.value());
			nvgStroke(ctx);
		}
	}

	float offX = 0.0f, offY = 0.0f, iconOffX = 0.0f;

	float tbounds[4];
	nvgTextBoxBounds(ctx, 0.0f, 0.0f, innerBounds.width, text.c_str(), nullptr, tbounds);

	float textWidth = tbounds[2] - tbounds[0];
	auto [totalWidth, height] = calculateBounds(style, ctx, text, icon);

	if (icon) {
		nvgSave(ctx);
		nvgFontSize(ctx, 18.0f);

		auto ico = ICON(icon);
		float icBounds[4];
		nvgTextBounds(ctx, 0.0f, 0.0f, ico.c_str(), nullptr, icBounds);

		float iconWidth = icBounds[2] - icBounds[0];
		float iconHeight = icBounds[3] - icBounds[1];

		if (element.alignmentX == NVG_ALIGN_LEFT) {
			offX = iconWidth;
		}
		else if (element.alignmentX == NVG_ALIGN_CENTER) {
			offX += iconWidth / 4.0f;
			iconOffX = innerBounds.width / 2 - totalWidth / 2.0f;
		}
		else if (element.alignmentX == NVG_ALIGN_RIGHT) {
			iconOffX = (innerBounds.width - totalWidth) - element.padding[1];
		}
		offX += 4.0f;

		if (element.textPaint.has_value()) {
			nvgApplyFillPaint(ctx, element.textPaint.value());
		}

		nvgTextAlign(ctx, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);

		float iconOffY = innerBounds.height / 2.0f;

		nvgText(
			ctx,
			innerBounds.x + iconOffX,
			iconOffY + 1.5f,
			ico.c_str(),
			nullptr
		);

		nvgRestore(ctx);
	}

	if (!text.empty()) {
		nvgFontSize(ctx, element.fontSize);

		if (element.textPaint.has_value()) {
			nvgApplyFillPaint(ctx, element.textPaint.value());
		}

		int alignmentFlags = element.alignmentX | element.alignmentY;
		if (alignmentFlags) nvgTextAlign(ctx, alignmentFlags);

		if (element.alignmentY == NVG_ALIGN_MIDDLE) {
			offY = innerBounds.height / 2.0f + 1.5f;
		}
		else if (element.alignmentY == NVG_ALIGN_BOTTOM) {
			offY = innerBounds.height;
		}

		nvgTextBox(ctx, innerBounds.x + offX, innerBounds.y + offY, innerBounds.width, text.c_str(), nullptr);
	}

	nvgRestore(ctx);

	return element;
}

Element StyleSheet::fontSetup(const std::string& style, NVGcontext* ctx, Rect& bounds) {
	auto element = getElement(style, ctx, bounds);

	int alignmentFlags = element.alignmentX | element.alignmentY;
	if (alignmentFlags) nvgTextAlign(ctx, alignmentFlags);
	nvgFontSize(ctx, element.fontSize);

	if (element.textPaint.has_value()) {
		nvgApplyFillPaint(ctx, element.textPaint.value());
	}

	return element;
}

std::pair<float, float> StyleSheet::calculateBounds(const std::string& style, NVGcontext* ctx, const std::string& text, size_t icon) {
	Rect bounds = Rect(0, 0, 1000, 1000);
	fontSetup(style, ctx, bounds);

	float tbounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, text.c_str(), nullptr, tbounds);
	float textWidth = tbounds[2] - tbounds[0];
	float totalWidth = textWidth;
	float height = (tbounds[3] - tbounds[1]);

	if (icon) {
		nvgTextAlign(ctx, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
		nvgFontSize(ctx, 18.0f);

		auto ico = ICON(icon);
		float bounds[4];
		nvgTextBounds(ctx, 0.0f, 0.0f, ico.c_str(), nullptr, bounds);

		float iconWidth = bounds[2] - bounds[0];
		float iconHeight = bounds[3] - bounds[1];

		totalWidth += iconWidth + 5.0f; // icon | text spacing
		height = std::max(height, iconHeight);
	}

	return std::make_pair(totalWidth, height);
}

Element StyleSheet::getElement(const std::string& style, NVGcontext* ctx, Rect& bounds) {
	Rect innerBounds = Rect(0, 0, bounds.width, bounds.height);
	Element element{ .bounds = innerBounds };

	auto stylePos = m_styles.find(style);
	if (stylePos == m_styles.end()) return element;

	for (auto [k, v] : stylePos->second) {
		ElementSetters.at(k)(ctx, element, v);
	}

	bounds.x += element.padding[0];
	bounds.width -= element.padding[0] + element.padding[1];
	bounds.y += element.padding[2];
	bounds.height -= element.padding[2] + element.padding[3];

	return element;
}

size_t PropertyPack::count() const {
	size_t c = 0;
	for (auto&& v : values) {
		if (v.type != PropertyType::None) c++;
	}
	return c;
}
