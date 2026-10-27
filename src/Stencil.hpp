#pragma once
// betobullet panel language ("stencil", docs/art/source/brand_v1/bb_stencil__v1.png): black and
// white only, chamfered frames with thick strokes, solid label plates, hazard stripes, rows of
// cartridges. Shared by every betobullet plugin; each copy is identical.
#include <rack.hpp>
#include "Controls.hpp"

namespace bb {
using namespace rack;

/** The two inks of a theme: 0 = black panel (main), 1 = white panel (inverse). */
struct Ink {
	NVGcolor bg, fg;
};

inline Ink ink(int theme) {
	Ink k;
	if (theme == 1) {
		k.bg = nvgRGB(0xF1, 0xF1, 0xEE);
		k.fg = nvgRGB(0x0B, 0x0B, 0x0B);
	}
	else {
		k.bg = nvgRGB(0x0B, 0x0B, 0x0B);
		k.fg = nvgRGB(0xF1, 0xF1, 0xEE);
	}
	return k;
}

static const char* const THEME_NAMES[2] = {"Black", "White"};

inline float px(float mm) {
	return mm2px(mm);
}

/** Path of a rectangle with its four corners cut at 45 degrees. */
inline void chamferPath(NVGcontext* vg, Rect mm, float cut) {
	float x0 = px(mm.pos.x), y0 = px(mm.pos.y), x1 = px(mm.pos.x + mm.size.x), y1 = px(mm.pos.y + mm.size.y), c = px(cut);
	nvgBeginPath(vg);
	nvgMoveTo(vg, x0 + c, y0);
	nvgLineTo(vg, x1 - c, y0);
	nvgLineTo(vg, x1, y0 + c);
	nvgLineTo(vg, x1, y1 - c);
	nvgLineTo(vg, x1 - c, y1);
	nvgLineTo(vg, x0 + c, y1);
	nvgLineTo(vg, x0, y1 - c);
	nvgLineTo(vg, x0, y0 + c);
	nvgClosePath(vg);
}

/** Outlined chamfered frame. */
inline void frame(NVGcontext* vg, Rect mm, const Ink& k, float cut = 2.0f, float stroke = 0.6f) {
	chamferPath(vg, mm, cut);
	nvgStrokeColor(vg, k.fg);
	nvgStrokeWidth(vg, px(stroke));
	nvgLineJoin(vg, NVG_MITER);
	nvgStroke(vg);
}

/** Frame with heavy corner brackets on the outside (the screen and the name plate). */
inline void bracketFrame(NVGcontext* vg, Rect mm, const Ink& k, float arm = 3.0f) {
	frame(vg, mm, k, 1.2f, 0.35f);
	float x0 = mm.pos.x - 0.9f, y0 = mm.pos.y - 0.9f, x1 = mm.pos.x + mm.size.x + 0.9f, y1 = mm.pos.y + mm.size.y + 0.9f;
	nvgStrokeColor(vg, k.fg);
	nvgStrokeWidth(vg, px(0.9f));
	nvgLineCap(vg, NVG_BUTT);
	const float pts[4][4] = {{x0, y0, 1, 1}, {x1, y0, -1, 1}, {x0, y1, 1, -1}, {x1, y1, -1, -1}};
	for (const auto& p : pts) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, px(p[0] + p[2] * arm), px(p[1]));
		nvgLineTo(vg, px(p[0]), px(p[1]));
		nvgLineTo(vg, px(p[0]), px(p[1] + p[3] * arm));
		nvgStroke(vg);
	}
}

/** Chamfered frame with a filled triangle tucked into each corner (the concept's notches). */
inline void notchedFrame(NVGcontext* vg, Rect mm, const Ink& k, float cut = 2.6f, float notch = 1.8f) {
	frame(vg, mm, k, cut, 0.6f);
	float x0 = mm.pos.x + 0.9f, y0 = mm.pos.y + 0.9f, x1 = mm.pos.x + mm.size.x - 0.9f, y1 = mm.pos.y + mm.size.y - 0.9f;
	const float c[4][4] = {{x0, y0, 1, 1}, {x1, y0, -1, 1}, {x0, y1, 1, -1}, {x1, y1, -1, -1}};
	nvgFillColor(vg, k.fg);
	for (const auto& p : c) {
		float d = cut - 0.3f;
		nvgBeginPath(vg);
		nvgMoveTo(vg, px(p[0] + p[2] * d), px(p[1]));
		nvgLineTo(vg, px(p[0] + p[2] * (d + notch)), px(p[1]));
		nvgLineTo(vg, px(p[0]), px(p[1] + p[3] * (d + notch)));
		nvgLineTo(vg, px(p[0]), px(p[1] + p[3] * d));
		nvgClosePath(vg);
		nvgFill(vg);
	}
}

/** Solid label plate centred at mm, the text knocked out in the panel colour. */
inline void plate(NVGcontext* vg, Vec mm, const char* s, const Ink& k, float minWidth = 7.0f, float h = 3.0f) {
	float w = std::max(px(minWidth), nvgTextBounds(vg, 0, 0, s, NULL, NULL) + px(1.8f));
	nvgBeginPath(vg);
	nvgRect(vg, px(mm.x) - w / 2, px(mm.y) - px(h) / 2, w, px(h));
	nvgFillColor(vg, k.fg);
	nvgFill(vg);
	nvgFillColor(vg, k.bg);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgText(vg, px(mm.x), px(mm.y) + px(0.15f), s, NULL);
}

/** Diagonal hazard stripes filling mm. */
inline void hazard(NVGcontext* vg, Rect mm, const Ink& k, float pitch = 2.4f) {
	nvgSave(vg);
	nvgScissor(vg, px(mm.pos.x), px(mm.pos.y), px(mm.size.x), px(mm.size.y));
	float h = mm.size.y;
	for (float x = mm.pos.x - h; x < mm.pos.x + mm.size.x; x += pitch) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, px(x), px(mm.pos.y + h));
		nvgLineTo(vg, px(x + pitch * 0.5f), px(mm.pos.y + h));
		nvgLineTo(vg, px(x + pitch * 0.5f + h), px(mm.pos.y));
		nvgLineTo(vg, px(x + h), px(mm.pos.y));
		nvgClosePath(vg);
		nvgFillColor(vg, k.fg);
		nvgFill(vg);
	}
	nvgRestore(vg);
}

/** One cartridge standing on its base, height h mm, centred on x. */
inline void bullet(NVGcontext* vg, float xMm, float baseMm, float h, const Ink& k) {
	float w = h * 0.26f, x = px(xMm - w / 2), y = px(baseMm), W = px(w), H = px(h);
	nvgBeginPath(vg);
	nvgMoveTo(vg, x, y);
	nvgLineTo(vg, x, y - H * 0.55f);
	nvgBezierTo(vg, x, y - H * 0.85f, x + W * 0.5f, y - H, x + W * 0.5f, y - H);
	nvgBezierTo(vg, x + W * 0.5f, y - H, x + W, y - H * 0.85f, x + W, y - H * 0.55f);
	nvgLineTo(vg, x + W, y);
	nvgClosePath(vg);
	nvgFillColor(vg, k.fg);
	nvgFill(vg);
	// the case/projectile seam, knocked out
	nvgBeginPath(vg);
	nvgRect(vg, x, y - H * 0.56f, W, std::max(1.f, H * 0.05f));
	nvgFillColor(vg, k.bg);
	nvgFill(vg);
}

inline void bullets(NVGcontext* vg, float x0, float x1, float baseMm, float h, int n, const Ink& k) {
	for (int i = 0; i < n; i++)
		bullet(vg, n == 1 ? (x0 + x1) / 2 : x0 + (x1 - x0) * i / (n - 1), baseMm, h, k);
}

/** fms::Colors for a theme: black knobs, nickel jacks, controls outlined in the theme ink. */
inline fms::Colors controlColors(int theme) {
	Ink k = ink(theme);
	fms::Colors c = {
		nvgRGB(0x14, 0x14, 0x14), nvgRGB(0x30, 0x30, 0x30), nvgRGB(0xF4, 0xF4, 0xF2), k.fg,
		nvgRGB(0xB8, 0xB8, 0xB6), nvgRGB(0xE4, 0xE4, 0xE2), nvgRGB(0x50, 0x50, 0x50), nvgRGB(0x1C, 0x1C, 0x1C), nvgRGB(0x04, 0x04, 0x04),
		nvgRGB(0x14, 0x14, 0x14), nvgRGB(0x34, 0x34, 0x34), nvgRGB(0xF4, 0xF4, 0xF2), nvgRGB(0xFF, 0xFF, 0xFF), k.fg,
		nvgRGB(0xF4, 0xF4, 0xF2)};
	return c;
}

/** Short connecting rule between two jacks on a row. */
inline void link(NVGcontext* vg, float xa, float xb, float yMm, const Ink& k, float jackR = 3.9f) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, px(xa + jackR + 0.8f), px(yMm));
	nvgLineTo(vg, px(xb - jackR - 0.8f), px(yMm));
	nvgStrokeColor(vg, k.fg);
	nvgStrokeWidth(vg, px(0.45f));
	nvgStroke(vg);
}

/** The module name in the header plate: stencil type, tracked out. */
inline void title(NVGcontext* vg, Rect mm, const char* name, int fontHandle, const Ink& k, float sizeMm) {
	bracketFrame(vg, mm, k, 2.6f);
	nvgFontFaceId(vg, fontHandle);
	nvgFontSize(vg, px(sizeMm));
	nvgTextLetterSpacing(vg, px(sizeMm * 0.06f));
	nvgFillColor(vg, k.fg);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgText(vg, px(mm.pos.x + mm.size.x / 2), px(mm.pos.y + mm.size.y / 2) + px(sizeMm * 0.04f), name, NULL);
	nvgTextLetterSpacing(vg, 0.f);
}

}  // namespace bb
