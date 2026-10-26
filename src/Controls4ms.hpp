#pragma once
// Panel controls in the graphic language of docs/art/components_4ms.png:
//  - Knob: matte black disc, faint scalloped edge, one white pointer, ring of thick dashes around it
//  - Jack: concentric rings (bevel, white ring, dark ring, black hole)
//  - Button: flat disc with a thick outline that fills with the accent colour when lit
//  - Toggle: round bezel with a capsule lever
// Everything is drawn with NanoVG so each plugin can change colours with its theme.
// Each plugin defines fms::colors() (see the end of its widget file).
#include <rack.hpp>

namespace fms {
using namespace rack;

struct Colors {
	NVGcolor knob, knobEdge, pointer, dash;
	NVGcolor jackOuter, jackRing, jackLine, jackDark, hole;
	NVGcolor btnOff, btnOffHi, btnOn, btnOnHi, outline;
	NVGcolor lever;
};

/** Colours for the module's current theme. Defined by the plugin. */
const Colors& colors(const engine::Module* m);

inline NVGcolor mix(NVGcolor a, NVGcolor b, float t) {
	return nvgRGBAf(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, 1.f);
}

/** Soft contact shadow below a round control. */
inline void softShadow(NVGcontext* vg, float cx, float cy, float r, float dy, float alpha) {
	NVGpaint p = nvgRadialGradient(vg, cx, cy + dy, r * 0.75f, r * 1.25f, nvgRGBAf(0.f, 0.f, 0.f, alpha), nvgRGBAf(0.f, 0.f, 0.f, 0.f));
	nvgBeginPath(vg);
	nvgCircle(vg, cx, cy + dy, r * 1.3f);
	nvgFillPaint(vg, p);
	nvgFill(vg);
}

inline void circle(NVGcontext* vg, float cx, float cy, float r, NVGcolor fill, NVGcolor line, float lineWidth) {
	nvgBeginPath(vg);
	nvgCircle(vg, cx, cy, r);
	nvgFillColor(vg, fill);
	nvgFill(vg);
	if (lineWidth > 0.f) {
		nvgStrokeColor(vg, line);
		nvgStrokeWidth(vg, lineWidth);
		nvgStroke(vg);
	}
}

// ---------------------------------------------------------------- knob
struct Knob : app::Knob {
	float radiusMm = 4.6f;
	bool ring = true;

	Knob() {
		minAngle = -0.83f * (float) M_PI;
		maxAngle = 0.83f * (float) M_PI;
		setRadius(radiusMm);
	}

	/** Size the widget for a disc of this radius plus its ring, keeping the centre where it is. */
	void setRadius(float mm) {
		Vec c = box.getCenter();
		radiusMm = mm;
		float half = mm2px(mm * 1.2f + 1.0f);
		box.size = Vec(2 * half, 2 * half);
		box.pos = c.minus(box.size.div(2));
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, r = mm2px(radiusMm);
		float t = 0.5f;
		if (engine::ParamQuantity* pq = getParamQuantity())
			t = pq->getScaledValue();

		softShadow(vg, cx, cy, r, r * 0.14f, 0.30f);
		circle(vg, cx, cy, r, c.knob, c.knobEdge, 0.f);

		// faint scalloped edge, like the cap of the real knob
		nvgBeginPath(vg);
		for (int i = 0; i <= 120; i++) {
			float a = i * 2.f * (float) M_PI / 120.f;
			float rr = r * 0.9f * (1.f + 0.022f * std::sin(11.f * a));
			float x = cx + rr * std::cos(a), y = cy + rr * std::sin(a);
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		nvgClosePath(vg);
		nvgStrokeColor(vg, c.knobEdge);
		nvgStrokeWidth(vg, std::max(0.6f, r * 0.02f));
		nvgStroke(vg);

		// one white pointer from the centre to the rim
		float a = -(float) M_PI / 2.f + minAngle + t * (maxAngle - minAngle);
		nvgBeginPath(vg);
		nvgMoveTo(vg, cx, cy);
		nvgLineTo(vg, cx + r * 0.96f * std::cos(a), cy + r * 0.96f * std::sin(a));
		nvgStrokeColor(vg, c.pointer);
		nvgStrokeWidth(vg, std::max(1.2f, r * 0.075f));
		nvgLineCap(vg, NVG_ROUND);
		nvgStroke(vg);

		// ring of thick dashes: the scale
		if (ring) {
			const int n = 15;
			float a0 = -(float) M_PI / 2.f + minAngle, a1 = -(float) M_PI / 2.f + maxAngle;
			float span = (a1 - a0) / n;
			nvgStrokeColor(vg, c.dash);
			nvgStrokeWidth(vg, std::max(1.5f, r * 0.11f));
			nvgLineCap(vg, NVG_BUTT);
			for (int i = 0; i < n; i++) {
				nvgBeginPath(vg);
				nvgArc(vg, cx, cy, r * 1.2f, a0 + i * span + span * 0.14f, a0 + (i + 1) * span - span * 0.14f, NVG_CW);
				nvgStroke(vg);
			}
		}
	}
};

inline Knob* knob(Vec pos, engine::Module* module, int paramId, float radiusMm, bool ring = true) {
	Knob* k = createParamCentered<Knob>(pos, module, paramId);
	k->setRadius(radiusMm);
	k->ring = ring;
	return k;
}

// ---------------------------------------------------------------- jack
struct Jack : app::PortWidget {
	float radiusMm = 3.3f;

	Jack() {
		box.size = mm2px(Vec(7.4f, 7.4f));
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, R = mm2px(radiusMm);
		circle(vg, cx, cy, R, c.jackOuter, c.jackLine, std::max(0.6f, R * 0.04f));
		circle(vg, cx, cy, R * 0.82f, c.jackRing, c.jackLine, std::max(0.5f, R * 0.03f));
		circle(vg, cx, cy, R * 0.62f, c.jackDark, c.jackDark, 0.f);
		circle(vg, cx, cy, R * 0.50f, c.hole, c.hole, 0.f);
		circle(vg, cx - R * 0.12f, cy - R * 0.14f, R * 0.09f, nvgRGB(0x2b, 0x2b, 0x2b), nvgRGB(0x2b, 0x2b, 0x2b), 0.f);
		PortWidget::draw(args);
	}
};

// ---------------------------------------------------------------- button
struct Button : app::Switch {
	int lightId = -1;       // light that shows the "on" state, or -1
	float discMm = 6.2f;    // disc diameter

	Button() {
		momentary = true;
		box.size = mm2px(Vec(9.4f, 9.4f));
	}

	float level() {
		float b = 0.f;
		if (module && lightId >= 0)
			b = clamp(module->lights[lightId].getBrightness(), 0.f, 1.f);
		engine::ParamQuantity* pq = getParamQuantity();
		if (pq && pq->getValue() > 0.5f)
			b = std::max(b, 0.45f);
		return b;
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, r = mm2px(discMm / 2);
		float b = level();
		softShadow(vg, cx, cy, r, r * 0.12f, 0.26f);
		circle(vg, cx, cy, r, mix(c.btnOff, c.btnOn, b), c.outline, std::max(1.f, r * 0.12f));
		// soft highlight, off-centre
		NVGpaint hi = nvgRadialGradient(vg, cx - r * 0.18f, cy - r * 0.2f, r * 0.05f, r * 0.8f,
		                                nvgRGBAf(mix(c.btnOffHi, c.btnOnHi, b).r, mix(c.btnOffHi, c.btnOnHi, b).g, mix(c.btnOffHi, c.btnOnHi, b).b, 0.85f),
		                                nvgRGBAf(0.f, 0.f, 0.f, 0.f));
		nvgBeginPath(vg);
		nvgCircle(vg, cx, cy, r * 0.88f);
		nvgFillPaint(vg, hi);
		nvgFill(vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;
		float b = level();
		if (b < 0.02f)
			return;
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, r = mm2px(discMm / 2);
		nvgBeginPath(vg);
		nvgCircle(vg, cx, cy, r * 2.0f);
		nvgFillPaint(vg, nvgRadialGradient(vg, cx, cy, r * 0.7f, r * 2.0f, nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.28f * b),
		                                   nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.f)));
		nvgFill(vg);
	}
};

// ---------------------------------------------------------------- toggle
/** Two-position switch: value 1 = lever up. */
struct Toggle : app::Switch {
	float radiusMm = 3.1f;

	Toggle() {
		box.size = mm2px(Vec(7.6f, 7.6f));
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, R = mm2px(radiusMm);
		bool up = true;
		if (engine::ParamQuantity* pq = getParamQuantity())
			up = pq->getValue() > 0.5f;
		circle(vg, cx, cy, R, c.jackOuter, c.jackLine, std::max(0.6f, R * 0.04f));
		circle(vg, cx, cy, R * 0.74f, c.jackRing, c.jackLine, std::max(0.5f, R * 0.03f));
		circle(vg, cx, cy, R * 0.5f, c.jackDark, c.jackDark, 0.f);
		float dy = up ? -R * 0.34f : R * 0.34f;
		softShadow(vg, cx, cy + dy, R * 0.36f, R * 0.1f, 0.30f);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, cx - R * 0.3f, cy + dy - R * 0.55f, R * 0.6f, R * 1.1f, R * 0.3f);
		nvgFillColor(vg, c.lever);
		nvgFill(vg);
		nvgStrokeColor(vg, c.jackLine);
		nvgStrokeWidth(vg, std::max(0.5f, R * 0.03f));
		nvgStroke(vg);
	}
};

// ---------------------------------------------------------------- 3-position toggle
/** Three-position switch: value 0 = lever down, 1 = middle, 2 = up. */
struct Toggle3 : app::Switch {
	float radiusMm = 3.1f;

	Toggle3() {
		box.size = mm2px(Vec(7.6f, 7.6f));
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float cx = box.size.x / 2, cy = box.size.y / 2, R = mm2px(radiusMm);
		int v = 1;
		if (engine::ParamQuantity* pq = getParamQuantity())
			v = (int) std::round(pq->getValue());
		circle(vg, cx, cy, R, c.jackOuter, c.jackLine, std::max(0.6f, R * 0.04f));
		circle(vg, cx, cy, R * 0.74f, c.jackRing, c.jackLine, std::max(0.5f, R * 0.03f));
		circle(vg, cx, cy, R * 0.5f, c.jackDark, c.jackDark, 0.f);
		if (v == 1) {
			// lever pointing at the viewer: a round cap
			softShadow(vg, cx, cy, R * 0.34f, R * 0.08f, 0.30f);
			circle(vg, cx, cy, R * 0.32f, c.lever, c.jackLine, std::max(0.5f, R * 0.03f));
			return;
		}
		float dy = v >= 2 ? -R * 0.34f : R * 0.34f;
		softShadow(vg, cx, cy + dy, R * 0.36f, R * 0.1f, 0.30f);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, cx - R * 0.3f, cy + dy - R * 0.55f, R * 0.6f, R * 1.1f, R * 0.3f);
		nvgFillColor(vg, c.lever);
		nvgFill(vg);
		nvgStrokeColor(vg, c.jackLine);
		nvgStrokeWidth(vg, std::max(0.5f, R * 0.03f));
		nvgStroke(vg);
	}
};

// ---------------------------------------------------------------- slider
/** Vertical slider with an illuminated handle; the light shows the attenuated CV. */
struct Slider : app::SliderKnob {
	int lightId = -1;
	float lengthMm = 24.f;

	Slider() {
		box.size = mm2px(Vec(6.4f, lengthMm));
	}

	void setLength(float mm) {
		Vec c = box.getCenter();
		lengthMm = mm;
		box.size = mm2px(Vec(6.4f, mm));
		box.pos = c.minus(box.size.div(2));
	}

	float level() {
		if (!module || lightId < 0)
			return 0.f;
		return clamp(module->lights[lightId].getBrightness(), 0.f, 1.f);
	}

	Rect handle() {
		float t = 0.f;
		if (engine::ParamQuantity* pq = getParamQuantity())
			t = pq->getScaledValue();
		float hh = mm2px(4.8f), hw = mm2px(5.6f);
		float y = (box.size.y - hh) * (1.f - t);
		return Rect(Vec((box.size.x - hw) / 2, y), Vec(hw, hh));
	}

	void draw(const DrawArgs& args) override {
		const Colors& c = colors(module);
		NVGcontext* vg = args.vg;
		float w = box.size.x, h = box.size.y;
		// slot
		float sw = mm2px(1.7f);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, (w - sw) / 2, mm2px(1.2f), sw, h - mm2px(2.4f), sw / 2);
		nvgFillColor(vg, c.hole);
		nvgFill(vg);
		nvgStrokeColor(vg, c.jackLine);
		nvgStrokeWidth(vg, 0.6f);
		nvgStroke(vg);
		// handle with a light window
		Rect r = handle();
		softShadow(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, r.size.x * 0.5f, r.size.y * 0.15f, 0.28f);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, mm2px(0.8f));
		nvgFillColor(vg, c.lever);
		nvgFill(vg);
		nvgStrokeColor(vg, c.outline);
		nvgStrokeWidth(vg, std::max(0.8f, mm2px(0.25f)));
		nvgStroke(vg);
		float b = level();
		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.pos.x + mm2px(1.2f), r.pos.y + mm2px(1.3f), r.size.x - mm2px(2.4f), r.size.y - mm2px(2.6f), mm2px(0.4f));
		nvgFillColor(vg, mix(c.jackDark, c.btnOn, b));
		nvgFill(vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;
		float b = level();
		if (b < 0.02f)
			return;
		const Colors& c = colors(module);
		Rect r = handle();
		float cx = r.pos.x + r.size.x / 2, cy = r.pos.y + r.size.y / 2;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, cx, cy, r.size.x);
		nvgFillPaint(args.vg, nvgRadialGradient(args.vg, cx, cy, r.size.x * 0.2f, r.size.x, nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.35f * b),
		                                        nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.f)));
		nvgFill(args.vg);
	}
};

inline Slider* slider(Vec center, engine::Module* module, int paramId, int lightId, float lengthMm) {
	Slider* s = createParamCentered<Slider>(center, module, paramId);
	s->setLength(lengthMm);
	s->lightId = lightId;
	return s;
}

}  // namespace fms
