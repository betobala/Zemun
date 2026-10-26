// Zemun: dual peak multimode state variable filter for VCV Rack.
// Recreated from the published behaviour of the Xaoc Belgrad (see docs/manual.txt); the DSP in
// Filter.hpp is our own.
#include <rack.hpp>
#include "Filter.hpp"
#include "Controls4ms.hpp"

using namespace rack;

Plugin* pluginInstance;

static const char* const MODE_DESCRIPTIONS[zemun::NUM_MODES] = {
	"LL: double slope lowpass", "LB: lowpass + bandpass", "NL: lowpass with notch", "NN: double notch",
	"LH: lowpass + highpass (band reject)", "BB: double bandpass", "HL: bandpass (highpass into lowpass)",
	"HN: highpass with notch", "BH: bandpass + highpass", "HH: double slope highpass"};

struct Zemun : Module {
	enum ParamId { FREQ_PARAM, LEVEL_PARAM, SPAN_PARAM, RESO_PARAM, BALANCE_PARAM, MODE_PARAM, TITO_PARAM, FM_ATT_PARAM, SPAN_ATT_PARAM, PARAMS_LEN };
	enum InputId { IN_INPUT, VOCT_INPUT, FM_INPUT, SPAN_INPUT, RESO_INPUT, BALANCE_INPUT, INPUTS_LEN };
	enum OutputId { OUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { FM_LIGHT, SPAN_LIGHT, LIGHTS_LEN };

	zemun::Voice voices[16];
	int theme = 0;  // 0 = Bone (light), 1 = Slate (dark)

	Zemun() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "Frequency", " Hz", 7000.f, 4.f);
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.42f, "Input level", "%", 0.f, 240.f);
		configParam(SPAN_PARAM, 0.f, 1.f, 0.f, "Span (distance between the peaks)", "%", 0.f, 100.f);
		configParam(RESO_PARAM, 0.f, 10.f, 0.f, "Resonance");
		configParam(BALANCE_PARAM, -1.f, 1.f, 0.f, "Balance (lower / higher peak)", "%", 0.f, 100.f);
		configSwitch(MODE_PARAM, 0.f, 9.f, 0.f, "Mode",
			std::vector<std::string>(MODE_DESCRIPTIONS, MODE_DESCRIPTIONS + zemun::NUM_MODES));
		configSwitch(TITO_PARAM, 0.f, 2.f, 1.f, "TITO", {"XM: cross modulation", "Off", "SM: self modulation"});
		configParam(FM_ATT_PARAM, 0.f, 1.f, 0.f, "FM amount", "%", 0.f, 100.f);
		configParam(SPAN_ATT_PARAM, 0.f, 1.f, 0.f, "Span CV amount", "%", 0.f, 100.f);
		configInput(IN_INPUT, "Audio");
		configInput(VOCT_INPUT, "V/oct");
		configInput(FM_INPUT, "FM");
		configInput(SPAN_INPUT, "Span CV (-5..+5 V)");
		configInput(RESO_INPUT, "Resonance CV (0..8 V)");
		configInput(BALANCE_INPUT, "Balance CV (-5..+5 V)");
		configOutput(OUT_OUTPUT, "Filter");
		configBypass(IN_INPUT, OUT_OUTPUT);
	}

	void onReset() override {
		for (zemun::Voice& v : voices)
			v.reset();
	}

	void process(const ProcessArgs& args) override {
		int channels = std::max({1, inputs[IN_INPUT].getChannels(), inputs[VOCT_INPUT].getChannels()});
		float freqKnob = params[FREQ_PARAM].getValue();
		float level = params[LEVEL_PARAM].getValue();
		float spanKnob = params[SPAN_PARAM].getValue();
		float reso = params[RESO_PARAM].getValue();
		float balance = params[BALANCE_PARAM].getValue();
		int mode = clamp((int) std::round(params[MODE_PARAM].getValue()), 0, zemun::NUM_MODES - 1);
		int tito = clamp((int) std::round(params[TITO_PARAM].getValue()), 0, 2);
		float fmAtt = params[FM_ATT_PARAM].getValue();
		float spanAtt = params[SPAN_ATT_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			float fm = inputs[FM_INPUT].getPolyVoltage(c) * fmAtt;
			float fc = zemun::knobToFreq(freqKnob) * std::exp2(inputs[VOCT_INPUT].getPolyVoltage(c) + fm);
			// span CV: +/-5 V covers the whole 8 octaves when the slider is at the top
			float span = zemun::spanOctaves(spanKnob, inputs[SPAN_INPUT].getPolyVoltage(c) / 5.f * spanAtt * 8.f);
			zemun::Params p;
			p.f1 = fc * std::exp2(-span / 2.f);
			p.f2 = fc * std::exp2(span / 2.f);
			// reso CV 0..8 V spans the whole knob (and a bit beyond, into distortion)
			zemun::resoBalance(reso + inputs[RESO_INPUT].getPolyVoltage(c) * 1.5f,
			                   balance + inputs[BALANCE_INPUT].getPolyVoltage(c) / 5.f, p.R1, p.R2);
			p.mode = mode;
			p.tito = tito;
			float x = zemun::inputStage(inputs[IN_INPUT].getPolyVoltage(c), level);
			float y = voices[c].process(x, p, args.sampleRate);
			outputs[OUT_OUTPUT].setVoltage(clamp(y * 5.f, -12.f, 12.f), c);
		}
		outputs[OUT_OUTPUT].setChannels(channels);

		// the sliders light up with the CV they pass
		lights[FM_LIGHT].setBrightnessSmooth(std::fabs(inputs[FM_INPUT].getVoltage() * fmAtt) / 5.f, args.sampleTime);
		lights[SPAN_LIGHT].setBrightnessSmooth(std::fabs(inputs[SPAN_INPUT].getVoltage() * spanAtt) / 5.f, args.sampleTime);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "theme", json_integer(theme));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* t = json_object_get(root, "theme"))
			theme = clamp((int) json_integer_value(t), 0, 1);
	}
};

// ---------------------------------------------------------------- themes
// "Bone" (light) and "Slate" (dark); one accent, crimson. See docs/art/STYLE.md.
struct ZTheme {
	NVGcolor panel, ink, grey, frame, group, plate, plateInk, plateLine, accent;
};
static const int NUM_THEMES = 2;
static const char* const THEME_NAMES[NUM_THEMES] = {"Bone", "Slate"};

static ZTheme zTheme(int t) {
	ZTheme k;
	if (t == 1) {
		k.panel = nvgRGB(0x17, 0x19, 0x1B);
		k.ink = nvgRGB(0xE6, 0xE8, 0xE8);
		k.grey = nvgRGB(0x8A, 0x8D, 0x93);
		k.frame = nvgRGB(0x6B, 0x6F, 0x75);
		k.group = nvgRGB(0x1E, 0x21, 0x24);
		k.plate = nvgRGB(0x0E, 0x11, 0x13);
		k.plateInk = nvgRGB(0xE6, 0xE8, 0xE8);
		k.plateLine = nvgRGB(0x6B, 0x6F, 0x75);
		k.accent = nvgRGB(0xE2, 0x44, 0x5A);
	}
	else {
		k.panel = nvgRGB(0xE8, 0xE6, 0xE1);
		k.ink = nvgRGB(0x0F, 0x12, 0x16);
		k.grey = nvgRGB(0x6E, 0x71, 0x77);
		k.frame = nvgRGB(0x0F, 0x12, 0x16);
		k.group = nvgRGB(0xF3, 0xF1, 0xEC);
		k.plate = nvgRGB(0x1B, 0x1E, 0x24);
		k.plateInk = nvgRGB(0xF6, 0xF4, 0xEF);
		k.plateLine = nvgRGB(0x1B, 0x1E, 0x24);
		k.accent = nvgRGB(0xC8, 0x10, 0x2E);
	}
	return k;
}

namespace fms {
const Colors& colors(const engine::Module* m) {
	static const Colors bone = {
		nvgRGB(0x11, 0x11, 0x11), nvgRGB(0x3A, 0x3A, 0x3A), nvgRGB(0xF6, 0xF4, 0xEF), nvgRGB(0x0F, 0x12, 0x16),
		nvgRGB(0xD3, 0xD3, 0xD0), nvgRGB(0xF7, 0xF7, 0xF5), nvgRGB(0x8E, 0x8E, 0x8A), nvgRGB(0x5C, 0x5C, 0x5A), nvgRGB(0x05, 0x05, 0x05),
		nvgRGB(0xF4, 0xF2, 0xEA), nvgRGB(0xFF, 0xFF, 0xFF), nvgRGB(0xE0, 0x1E, 0x3C), nvgRGB(0xFF, 0x7A, 0x8C), nvgRGB(0x0F, 0x12, 0x16),
		nvgRGB(0xFB, 0xFB, 0xF9)};
	static const Colors slate = {
		nvgRGB(0x0B, 0x0B, 0x0D), nvgRGB(0x3A, 0x3B, 0x3F), nvgRGB(0xF6, 0xF4, 0xEF), nvgRGB(0xE6, 0xE8, 0xE8),
		nvgRGB(0xD3, 0xD3, 0xD0), nvgRGB(0xF7, 0xF7, 0xF5), nvgRGB(0x8E, 0x8E, 0x8A), nvgRGB(0x5C, 0x5C, 0x5A), nvgRGB(0x05, 0x05, 0x05),
		nvgRGB(0xE6, 0xE8, 0xE8), nvgRGB(0xFF, 0xFF, 0xFF), nvgRGB(0xE2, 0x44, 0x5A), nvgRGB(0xFF, 0x9A, 0xA8), nvgRGB(0x6B, 0x6F, 0x75),
		nvgRGB(0xFB, 0xFB, 0xF9)};
	int t = m ? static_cast<const Zemun*>(m)->theme : 0;
	return t == 1 ? slate : bone;
}
}  // namespace fms

static std::shared_ptr<window::Font> pluginFont(const char* file) {
	std::shared_ptr<window::Font> f = APP->window->loadFont(asset::plugin(pluginInstance, std::string("res/fonts/") + file));
	if (!f)
		f = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
	return f;
}

// ---------------------------------------------------------------- layout (mm)
static const float W = 71.12f;
static const float CX = W / 2.f;
static const Vec LEVEL_POS(10.5f, 22.5f), SPAN_POS(60.6f, 22.5f);
static const Vec FREQ_POS(CX, 40.0f);
static const Vec TITO_POS(8.0f, 44.0f);
static const Vec RESO_POS(10.5f, 59.5f), BAL_POS(60.6f, 59.5f);
static const Vec MODE_POS(CX, 73.5f);
static const float SLIDER_TOP = 70.5f, SLIDER_BOTTOM = 94.0f;
static const float FM_SLIDER_X = 9.4f, SPAN_SLIDER_X = 61.7f;
static const Vec RESO_IN(21.9f, 100.6f), VOCT_IN(CX, 103.0f), BAL_IN(49.2f, 100.6f);
static const Vec FM_IN(9.4f, 111.0f), AUDIO_IN(21.9f, 112.8f), OUT_POS(49.2f, 112.8f), SPAN_IN(61.7f, 111.0f);
static const float FREQ_R = 10.0f, KNOB_R = 5.2f, MODE_R = 6.0f;
static const float MIN_ANGLE = -0.83f * (float) M_PI, MAX_ANGLE = 0.83f * (float) M_PI;

/** Point at a knob angle (Rack convention: 0 = up, clockwise) and radius, in mm. */
static Vec polar(Vec c, float angle, float r) {
	return Vec(c.x + r * std::sin(angle), c.y - r * std::cos(angle));
}

// ---------------------------------------------------------------- widgets
/** The logotype, one SVG per theme (bone / slate). */
struct LogoWidget : TransparentWidget {
	Zemun* module = nullptr;
	std::shared_ptr<window::Svg> svg[2];

	void load(float widthMm) {
		svg[0] = window::Svg::load(asset::plugin(pluginInstance, "res/logo_bone.svg"));
		svg[1] = window::Svg::load(asset::plugin(pluginInstance, "res/logo_slate.svg"));
		float w = svg[0]->handle->width, h = svg[0]->handle->height;
		box.size = Vec(mm2px(widthMm), mm2px(widthMm) * h / w);
	}

	void draw(const DrawArgs& args) override {
		int i = (module && module->theme == 1) ? 1 : 0;
		if (!svg[i])
			return;
		float sc = box.size.x / svg[i]->handle->width;
		nvgSave(args.vg);
		nvgScale(args.vg, sc, sc);
		window::svgDraw(args.vg, svg[i]->handle);
		nvgRestore(args.vg);
	}
};

/** Panel background, groups, scales and every label. Drawn in code so the theme can change. */
struct PanelArt : Widget {
	Zemun* module = nullptr;

	void text(NVGcontext* vg, Vec mm, const char* s) {
		nvgText(vg, mm2px(mm.x), mm2px(mm.y), s, NULL);
	}

	void pill(NVGcontext* vg, Vec mm, const char* s, float sizeMm, const ZTheme& k) {
		float w = nvgTextBounds(vg, 0, 0, s, NULL, NULL);
		float h = mm2px(sizeMm) * 1.25f;
		nvgBeginPath(vg);
		nvgRoundedRect(vg, mm2px(mm.x) - w / 2 - h * 0.45f, mm2px(mm.y) - h / 2, w + h * 0.9f, h, h / 2);
		nvgFillColor(vg, k.plate);
		nvgFill(vg);
		nvgFillColor(vg, k.plateInk);
		text(vg, mm, s);
	}

	void draw(const DrawArgs& args) override {
		ZTheme k = zTheme(module ? module->theme : 0);
		NVGcontext* vg = args.vg;
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(vg, k.panel);
		nvgFill(vg);

		// the jack field in one rounded group, the output on a dark plate
		nvgBeginPath(vg);
		nvgRoundedRect(vg, mm2px(2.4f), mm2px(95.2f), mm2px(W - 4.8f), mm2px(26.6f), mm2px(3.0f));
		nvgFillColor(vg, k.group);
		nvgFill(vg);
		nvgStrokeColor(vg, k.frame);
		nvgStrokeWidth(vg, mm2px(0.35f));
		nvgStroke(vg);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, mm2px(42.2f), mm2px(107.6f), mm2px(14.0f), mm2px(13.4f), mm2px(2.6f));
		nvgFillColor(vg, k.plate);
		nvgFill(vg);
		if (module && module->theme == 1) {
			nvgStrokeColor(vg, k.plateLine);
			nvgStrokeWidth(vg, mm2px(0.3f));
			nvgStroke(vg);
		}

		// position dots of the mode switch
		for (int i = 0; i < zemun::NUM_MODES; i++) {
			Vec d = polar(MODE_POS, MIN_ANGLE + i * (MAX_ANGLE - MIN_ANGLE) / 9.f, MODE_R + 2.0f);
			nvgBeginPath(vg);
			nvgCircle(vg, mm2px(d.x), mm2px(d.y), mm2px(0.45f));
			nvgFillColor(vg, k.ink);
			nvgFill(vg);
		}

		// slider scales: ticks and 0..10
		for (int s = 0; s < 2; s++) {
			float x = s == 0 ? FM_SLIDER_X : SPAN_SLIDER_X;
			float side = s == 0 ? -1.f : 1.f;
			for (int i = 0; i <= 10; i++) {
				float y = SLIDER_BOTTOM - 2.4f - (SLIDER_BOTTOM - SLIDER_TOP - 4.8f) * i / 10.f;
				nvgBeginPath(vg);
				nvgMoveTo(vg, mm2px(x + side * 3.4f), mm2px(y));
				nvgLineTo(vg, mm2px(x + side * (i % 2 == 0 ? 4.6f : 4.0f)), mm2px(y));
				nvgStrokeColor(vg, k.ink);
				nvgStrokeWidth(vg, mm2px(0.25f));
				nvgStroke(vg);
			}
		}

		std::shared_ptr<window::Font> reg = pluginFont("SpaceMono-Regular.ttf");
		std::shared_ptr<window::Font> bold = pluginFont("SpaceMono-Bold.ttf");
		if (!reg || !bold)
			return;
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// slider numbers
		nvgFontFaceId(vg, reg->handle);
		nvgFontSize(vg, mm2px(2.2f));
		nvgFillColor(vg, k.ink);
		for (int s = 0; s < 2; s++) {
			float x = s == 0 ? FM_SLIDER_X - 6.4f : SPAN_SLIDER_X + 6.4f;
			for (int i = 0; i <= 10; i += 2) {
				float y = SLIDER_BOTTOM - 2.4f - (SLIDER_BOTTOM - SLIDER_TOP - 4.8f) * i / 10.f;
				text(vg, Vec(x, y), string::f("%d", i).c_str());
			}
		}

		// numbers around the frequency knob
		nvgFontSize(vg, mm2px(2.4f));
		for (int i = 0; i <= 10; i++) {
			Vec p = polar(FREQ_POS, MIN_ANGLE + i * (MAX_ANGLE - MIN_ANGLE) / 10.f, FREQ_R + 5.2f);
			text(vg, p, string::f("%d", i).c_str());
		}

		// mode names
		nvgFontFaceId(vg, bold->handle);
		nvgFontSize(vg, mm2px(2.6f));
		for (int i = 0; i < zemun::NUM_MODES; i++) {
			Vec p = polar(MODE_POS, MIN_ANGLE + i * (MAX_ANGLE - MIN_ANGLE) / 9.f, MODE_R + 5.6f);
			text(vg, p, zemun::MODE_NAMES[i]);
		}

		// knob and jack labels
		nvgFontFaceId(vg, reg->handle);
		nvgFontSize(vg, mm2px(3.0f));
		text(vg, Vec(LEVEL_POS.x, 31.0f), "LEVEL");
		text(vg, Vec(SPAN_POS.x, 31.0f), "SPAN");
		text(vg, Vec(RESO_POS.x, 68.0f), "RESO");
		text(vg, Vec(BAL_POS.x, 68.0f), "BALANCE");
		text(vg, Vec(RESO_IN.x, RESO_IN.y + 5.2f), "RESO");
		text(vg, Vec(VOCT_IN.x, VOCT_IN.y + 5.2f), "V/OCT");
		text(vg, Vec(BAL_IN.x, BAL_IN.y + 5.2f), "BALANCE");
		text(vg, Vec(FM_IN.x, FM_IN.y + 5.2f), "FM");
		text(vg, Vec(AUDIO_IN.x, AUDIO_IN.y + 5.2f), "INPUT");
		text(vg, Vec(SPAN_IN.x, SPAN_IN.y + 5.2f), "SPAN");
		nvgFillColor(vg, k.plateInk);
		text(vg, Vec(OUT_POS.x, OUT_POS.y + 5.2f), "OUTPUT");

		// TITO switch
		nvgFillColor(vg, k.ink);
		nvgFontSize(vg, mm2px(2.6f));
		text(vg, Vec(TITO_POS.x, 38.0f), "TITO");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		text(vg, Vec(TITO_POS.x + 4.4f, TITO_POS.y - 2.6f), "SM");
		text(vg, Vec(TITO_POS.x + 4.4f, TITO_POS.y + 2.6f), "XM");
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// pills under the two big controls
		nvgFontFaceId(vg, bold->handle);
		nvgFontSize(vg, mm2px(2.8f));
		pill(vg, Vec(FREQ_POS.x, 56.6f), "FREQ", 2.8f, k);
		pill(vg, Vec(MODE_POS.x, 87.4f), "MODE", 2.8f, k);
	}
};

struct ZemunWidget : ModuleWidget {
	ZemunWidget(Zemun* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Zemun.svg")));

		PanelArt* art = new PanelArt;
		art->module = module;
		art->box.size = box.size;
		addChild(art);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		LogoWidget* logo = new LogoWidget;
		logo->module = module;
		logo->load(26.0f);
		logo->box.pos = Vec((box.size.x - logo->box.size.x) / 2, mm2px(3.0f));
		addChild(logo);

		addParam(fms::knob(mm2px(LEVEL_POS), module, Zemun::LEVEL_PARAM, KNOB_R));
		addParam(fms::knob(mm2px(SPAN_POS), module, Zemun::SPAN_PARAM, KNOB_R));
		addParam(fms::knob(mm2px(FREQ_POS), module, Zemun::FREQ_PARAM, FREQ_R));
		addParam(fms::knob(mm2px(RESO_POS), module, Zemun::RESO_PARAM, KNOB_R));
		addParam(fms::knob(mm2px(BAL_POS), module, Zemun::BALANCE_PARAM, KNOB_R));
		fms::Knob* mode = fms::knob(mm2px(MODE_POS), module, Zemun::MODE_PARAM, MODE_R, false);
		mode->snap = true;
		addParam(mode);
		addParam(createParamCentered<fms::Toggle3>(mm2px(TITO_POS), module, Zemun::TITO_PARAM));

		float sy = (SLIDER_TOP + SLIDER_BOTTOM) / 2.f, sl = SLIDER_BOTTOM - SLIDER_TOP;
		addParam(fms::slider(mm2px(Vec(FM_SLIDER_X, sy)), module, Zemun::FM_ATT_PARAM, Zemun::FM_LIGHT, sl));
		addParam(fms::slider(mm2px(Vec(SPAN_SLIDER_X, sy)), module, Zemun::SPAN_ATT_PARAM, Zemun::SPAN_LIGHT, sl));

		addInput(createInputCentered<fms::Jack>(mm2px(RESO_IN), module, Zemun::RESO_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(VOCT_IN), module, Zemun::VOCT_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(BAL_IN), module, Zemun::BALANCE_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(FM_IN), module, Zemun::FM_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(AUDIO_IN), module, Zemun::IN_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(SPAN_IN), module, Zemun::SPAN_INPUT));
		addOutput(createOutputCentered<fms::Jack>(mm2px(OUT_POS), module, Zemun::OUT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Zemun* m = getModule<Zemun>();
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexPtrSubmenuItem("Theme", std::vector<std::string>(THEME_NAMES, THEME_NAMES + NUM_THEMES), &m->theme));
	}
};

Model* modelZemun = createModel<Zemun, ZemunWidget>("Zemun");

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelZemun);
}
