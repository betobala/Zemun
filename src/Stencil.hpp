#pragma once
// betobullet panel language. The "clean" set (label, rule, outRing, nameplate) is the current look:
// black panel, white text, thin rules, red only for outputs and lit things. The stencil helpers
// (frames, plates, hazard, bullets) are kept for reference. Shared by every betobullet plugin.
#include <rack.hpp>
#include "Controls.hpp"

namespace bb {
using namespace rack;

/** The two inks of a theme: 0 = black panel (main), 1 = white panel (inverse). */
struct Ink {
	NVGcolor bg, fg;
	NVGcolor red, dim;  // the accent; secondary text and rules
};

inline Ink ink(int theme) {
	Ink k;
	if (theme == 1) {
		k.bg = nvgRGB(0xF1, 0xF1, 0xEE);
		k.fg = nvgRGB(0x0B, 0x0B, 0x0B);
		k.red = nvgRGB(0xD0, 0x1E, 0x25);
		k.dim = nvgRGB(0x8C, 0x8C, 0x8A);
	}
	else {
		k.bg = nvgRGB(0x0E, 0x0E, 0x0E);
		k.fg = nvgRGB(0xF1, 0xF1, 0xEE);
		k.red = nvgRGB(0xE8, 0x2A, 0x2F);
		k.dim = nvgRGB(0x5E, 0x5E, 0x5C);
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
		nvgRGB(0x14, 0x14, 0x14), nvgRGB(0x30, 0x30, 0x30), nvgRGB(0xF4, 0xF4, 0xF2), k.dim,
		nvgRGB(0xB8, 0xB8, 0xB6), nvgRGB(0xE4, 0xE4, 0xE2), nvgRGB(0x50, 0x50, 0x50), nvgRGB(0x1C, 0x1C, 0x1C), nvgRGB(0x04, 0x04, 0x04),
		nvgRGB(0x14, 0x14, 0x14), nvgRGB(0x34, 0x34, 0x34), k.red, nvgRGB(0xFF, 0x6A, 0x6E), k.dim,
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

// ---------------------------------------------------------------- clean set
/** Plain label: centred text in the given colour (current font and size). */
inline void label(NVGcontext* vg, Vec mm, const char* s, NVGcolor c) {
	nvgFillColor(vg, c);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgText(vg, px(mm.x), px(mm.y), s, NULL);
}

/** Thin horizontal rule between sections. */
inline void rule(NVGcontext* vg, float x0, float x1, float yMm, const Ink& k) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, px(x0), px(yMm));
	nvgLineTo(vg, px(x1), px(yMm));
	nvgStrokeColor(vg, k.dim);
	nvgStrokeWidth(vg, px(0.25f));
	nvgStroke(vg);
}

/** Red ring around an output jack, so outputs read at a glance. */
inline void outRing(NVGcontext* vg, Vec mm, const Ink& k, float r = 4.3f) {
	nvgBeginPath(vg);
	nvgCircle(vg, px(mm.x), px(mm.y), px(r));
	nvgStrokeColor(vg, k.red);
	nvgStrokeWidth(vg, px(0.55f));
	nvgStroke(vg);
}

/** The graphic mark of each module's logo, drawn under (or beside) its name. */
enum Mark { MARK_NONE, MARK_BAR, MARK_PEAKS, MARK_BRAID, MARK_PULSES, MARK_ENVELOPE, MARK_SAWS, MARK_DOTS, MARK_OCTOPUS, MARK_DELTA };

inline void strokePath(NVGcontext* vg, NVGcolor c, float wMm) {
	nvgStrokeColor(vg, c);
	nvgStrokeWidth(vg, px(wMm));
	nvgLineCap(vg, NVG_ROUND);
	nvgLineJoin(vg, NVG_ROUND);
	nvgStroke(vg);
}

/** Draws mark m in the band x0..x1 (px), top y0, height h (px). */
inline void drawMark(NVGcontext* vg, Mark m, float x0, float x1, float y0, float h, const Ink& k) {
	float w = x1 - x0;
	switch (m) {
		case MARK_PEAKS: {  // the two-peak response of the filter
			nvgBeginPath(vg);
			for (int i = 0; i <= 80; i++) {
				float u = i / 80.f;
				float v = 0.95f * std::exp(-std::pow((u - 0.3f) / 0.075f, 2.f)) + 0.7f * std::exp(-std::pow((u - 0.72f) / 0.075f, 2.f));
				float x = x0 + w * u, y = y0 + h * (0.8f - 0.75f * v);
				if (i == 0)
					nvgMoveTo(vg, x, y);
				else
					nvgLineTo(vg, x, y);
			}
			strokePath(vg, k.red, 0.7f);
			nvgBeginPath(vg);
			nvgMoveTo(vg, x0, y0 + h);
			nvgLineTo(vg, x1, y0 + h);
			strokePath(vg, k.dim, 0.3f);
			break;
		}
		case MARK_BRAID: {  // four LFO phases braided, one in red
			for (int p = 3; p >= 0; p--) {
				nvgBeginPath(vg);
				for (int i = 0; i <= 80; i++) {
					float u = i / 80.f;
					float y = y0 + h * (0.5f - 0.45f * std::sin(2.f * M_PI * (2.f * u) + p * M_PI / 2.f));
					if (i == 0)
						nvgMoveTo(vg, x0 + w * u, y);
					else
						nvgLineTo(vg, x0 + w * u, y);
				}
				strokePath(vg, p == 0 ? k.red : (p == 2 ? k.fg : k.dim), 0.55f);
			}
			break;
		}
		case MARK_PULSES: {  // a gate train: the pulses that pass in red, the skipped ones short and grey
			static const int pass[7] = {1, 0, 1, 1, 0, 0, 1};
			for (int i = 0; i < 7; i++) {
				float x = x0 + w * (i + 0.5f) / 7.f;
				nvgBeginPath(vg);
				nvgMoveTo(vg, x, y0 + h);
				nvgLineTo(vg, x, pass[i] ? y0 : y0 + h * 0.7f);
				strokePath(vg, pass[i] ? k.red : k.dim, 0.75f);
			}
			nvgBeginPath(vg);
			nvgMoveTo(vg, x0, y0 + h);
			nvgLineTo(vg, x1, y0 + h);
			strokePath(vg, k.dim, 0.3f);
			break;
		}
		case MARK_ENVELOPE: {  // an attack/decay envelope, as on the Piqued screen
			nvgBeginPath(vg);
			nvgMoveTo(vg, x0, y0 + h);
			nvgLineTo(vg, x0 + w * 0.22f, y0 + h);
			nvgLineTo(vg, x0 + w * 0.36f, y0);
			for (int i = 1; i <= 40; i++) {
				float u = i / 40.f;
				nvgLineTo(vg, x0 + w * (0.36f + 0.5f * u), y0 + h * (1.f - std::exp(-4.5f * u)));
			}
			nvgLineTo(vg, x1, y0 + h);
			strokePath(vg, k.red, 0.65f);
			break;
		}
		case MARK_SAWS: {  // three detuned saws, one in red
			for (int s = 2; s >= 0; s--) {
				float period = w / (3.f + 0.35f * s), off = s * w * 0.06f;
				nvgBeginPath(vg);
				nvgMoveTo(vg, x0, y0 + h);
				for (float x = x0 - off; x < x1; x += period) {
					float xa = std::max(x0, x), xb = std::min(x1, x + period);
					float ya = y0 + h * (x >= x0 ? 1.f : 1.f - (x0 - x) / period);
					nvgLineTo(vg, xa, ya);
					nvgLineTo(vg, xb, y0 + h * (1.f - (xb - x) / period));
				}
				strokePath(vg, s == 0 ? k.red : (s == 1 ? k.fg : k.dim), 0.5f);
			}
			break;
		}
		case MARK_DOTS: {  // a line with notes on it, the middle one red
			nvgBeginPath(vg);
			nvgMoveTo(vg, x0, y0 + h * 0.5f);
			nvgLineTo(vg, x1, y0 + h * 0.5f);
			strokePath(vg, k.dim, 0.3f);
			for (int i = 0; i < 5; i++) {
				nvgBeginPath(vg);
				nvgCircle(vg, x0 + w * (0.1f + 0.2f * i), y0 + h * 0.5f, h * 0.32f);
				nvgFillColor(vg, i == 2 ? k.red : k.fg);
				nvgFill(vg);
			}
			break;
		}
		case MARK_OCTOPUS: {  // a head and eight arms, one in red
			float cx = (x0 + x1) / 2, r = h * 0.42f;
			nvgBeginPath(vg);
			nvgArc(vg, cx, y0 + r, r, M_PI, 0.f, NVG_CW);
			nvgClosePath(vg);
			nvgFillColor(vg, k.fg);
			nvgFill(vg);
			for (int i = 0; i < 8; i++) {
				float x = cx - r + (2.f * r) * (i + 0.5f) / 8.f;
				nvgBeginPath(vg);
				nvgMoveTo(vg, x, y0 + r + h * 0.08f);
				nvgLineTo(vg, x, y0 + h * (i % 2 ? 0.95f : 0.82f));
				strokePath(vg, i == 5 ? k.red : k.fg, 0.5f);
			}
			break;
		}
		case MARK_DELTA:
		case MARK_BAR:
		default: {
			nvgBeginPath(vg);
			nvgRect(vg, x0, y0, w, px(0.7f));
			nvgFillColor(vg, k.red);
			nvgFill(vg);
			break;
		}
	}
}

/** Module logo at the top: the name in stencil type with the module's mark under it (or a red delta
beside it). MARK_BAR is the plain short red bar. */
inline void nameplate(NVGcontext* vg, float cxMm, float yMm, const char* name, int fontHandle, const Ink& k, float sizeMm,
                      Mark mark = MARK_BAR, float markWMm = 0.f, float markHMm = 0.f) {
	nvgFontFaceId(vg, fontHandle);
	nvgFontSize(vg, px(sizeMm));
	nvgTextLetterSpacing(vg, px(sizeMm * 0.08f));
	float tw = nvgTextBounds(vg, 0, 0, name, NULL, NULL);
	float shift = mark == MARK_DELTA ? px(sizeMm * 0.45f) : 0.f;  // make room for the delta
	label(vg, Vec(cxMm - shift / mm2px(1.f), yMm), name, k.fg);
	nvgTextLetterSpacing(vg, 0.f);
	if (mark == MARK_DELTA) {
		float x = px(cxMm) - shift + tw / 2 + px(sizeMm * 0.2f), s = px(sizeMm * 0.62f), yb = px(yMm + sizeMm * 0.34f);
		nvgBeginPath(vg);
		nvgMoveTo(vg, x, yb);
		nvgLineTo(vg, x + s * 0.6f, yb - s);
		nvgLineTo(vg, x + s * 1.2f, yb);
		nvgClosePath(vg);
		nvgFillColor(vg, k.red);
		nvgFill(vg);
		return;
	}
	if (mark == MARK_NONE)
		return;
	float w = mark == MARK_BAR ? std::min(px(sizeMm * 2.4f), tw) : tw;
	if (mark == MARK_OCTOPUS)
		w = px(sizeMm * 1.6f);
	float h = px(sizeMm * (mark == MARK_BAR ? 0.2f : (mark == MARK_OCTOPUS ? 1.0f : 0.55f)));
	if (markWMm > 0.f)
		w = px(markWMm);
	if (markHMm > 0.f)
		h = px(markHMm);
	drawMark(vg, mark, px(cxMm) - w / 2, px(cxMm) + w / 2, px(yMm + sizeMm * 0.6f), h, k);
}

}  // namespace bb
