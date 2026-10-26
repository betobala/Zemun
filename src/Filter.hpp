#pragma once
// Dual peak multimode state variable filter: two nonlinear SVF cores combined in series or in
// parallel to give ten responses. Written from the published behaviour of the Xaoc Belgrad
// (operator's manual rev. 1976/1.0); no code or schematics of the original were used.
// Independent of Rack so it can be unit tested.
#include <cmath>
#include <algorithm>

namespace zemun {

enum Mode { LL, LB, NL, NN, LH, BB, HL, HN, BH, HH, NUM_MODES };
static const char* const MODE_NAMES[NUM_MODES] = {"LL", "LB", "NL", "NN", "LH", "BB", "HL", "HN", "BH", "HH"};

// TITO switch: down = cross modulation, middle = off, up = self modulation
enum Tito { TITO_XM = 0, TITO_OFF = 1, TITO_SM = 2 };

static const int OVERSAMPLE = 2;
static const float SAT = 1.5f;  // soft limit of the integrator states (normalised units, 1.0 = 5 V)

inline float sat(float x) {
	return SAT * std::tanh(x / SAT);
}

/** Trapezoidal (zero delay feedback) state variable filter with saturating integrators.
R is the damping: 1 = no resonance, 0 = infinite Q, below 0 = self oscillation,
which the saturating states keep bounded and progressively distort. */
struct Svf {
	float s1 = 0.f, s2 = 0.f;
	float lp = 0.f, bp = 0.f, hp = 0.f;

	void reset() {
		s1 = s2 = lp = bp = hp = 0.f;
	}

	void process(float x, float g, float R) {
		float den = std::max(1.f + 2.f * R * g + g * g, 0.05f);
		hp = (x - (2.f * R + g) * s1 - s2) / den;
		float v1 = g * hp;
		bp = v1 + s1;
		s1 = sat(bp + v1);
		float v2 = g * bp;
		lp = v2 + s2;
		s2 = sat(lp + v2);
	}

	float notch() const {
		return lp + hp;
	}
};

/** Everything the filter needs for one sample, already in physical units. */
struct Params {
	float f1 = 1000.f, f2 = 1000.f;   // core frequencies (Hz)
	float R1 = 1.f, R2 = 1.f;         // core damping
	int mode = LL;
	int tito = TITO_OFF;
	float titoDepth = 2.5f;           // octaves of audio-rate modulation at full swing
};

// ---- mapping of the front panel to physical parameters ----

/** freq knob 0..1 -> 4 Hz .. 28 kHz */
inline float knobToFreq(float knob) {
	return 4.f * std::pow(7000.f, knob);
}

/** span knob 0..1 (deliberately non linear) plus CV -> distance between the peaks in octaves (0..8) */
inline float spanOctaves(float knob, float cvOctaves) {
	return std::max(0.f, std::min(8.f, 8.f * knob * knob + cvOctaves));
}

/** reso 0..12 (knob 0..10 plus CV) -> damping; self oscillation starts around 8 */
inline float resoToDamping(float reso) {
	float rho = std::max(reso, 0.f) / 8.f;
	if (rho <= 1.f)
		return 1.f - rho;
	return -std::min((rho - 1.f) * 0.6f, 0.3f);
}

/** reso and balance (-1 lower peak .. +1 higher peak) -> damping of each core */
inline void resoBalance(float reso, float balance, float& R1, float& R2) {
	balance = std::max(-1.f, std::min(1.f, balance));
	R1 = resoToDamping(reso * std::min(1.f, 1.f - balance));
	R2 = resoToDamping(reso * std::min(1.f, 1.f + balance));
}

/** Input stage: level 0..1 is a gain up to 2.4 with soft overdrive. Volts in, normalised out. */
inline float inputStage(float volts, float level) {
	float x = volts / 5.f * level * 2.4f;
	return 1.4f * std::tanh(x / 1.4f);
}

/** One voice: two cores, the mode routing and 2x oversampling. */
struct Voice {
	Svf c1, c2;
	float xPrev = 0.f;

	void reset() {
		c1.reset();
		c2.reset();
		xPrev = 0.f;
	}

	static float gain(float f, float fsOs) {
		f = std::max(1.f, std::min(f, 0.45f * fsOs));
		return std::tan(float(M_PI) * f / fsOs);
	}

	float step(float x, const Params& p, float fsOs, float g1, float g2) {
		if (p.tito != TITO_OFF) {
			// double non-linear audio-rate modulation of the two cores
			float m1 = p.tito == TITO_SM ? c1.bp : c2.bp;
			float m2 = p.tito == TITO_SM ? c2.bp : c1.bp;
			g1 = gain(p.f1 * std::exp2(p.titoDepth * std::tanh(3.f * m1)), fsOs);
			g2 = gain(p.f2 * std::exp2(p.titoDepth * std::tanh(3.f * m2)), fsOs);
		}
		switch (p.mode) {
			case LL: c1.process(x, g1, p.R1); c2.process(c1.lp, g2, p.R2); return c2.lp;
			case LB: c1.process(x, g1, p.R1); c2.process(x, g2, p.R2); return c1.lp + c2.bp;
			case NL: c1.process(x, g1, p.R1); c2.process(c1.notch(), g2, p.R2); return c2.lp;
			case NN: c1.process(x, g1, p.R1); c2.process(c1.notch(), g2, p.R2); return c2.notch();
			case LH: c1.process(x, g1, p.R1); c2.process(x, g2, p.R2); return c1.lp + c2.hp;
			case BB: c1.process(x, g1, p.R1); c2.process(x, g2, p.R2); return c1.bp + c2.bp;
			case HL: c1.process(x, g1, p.R1); c2.process(c1.hp, g2, p.R2); return c2.lp;
			case HN: c1.process(x, g1, p.R1); c2.process(c1.hp, g2, p.R2); return c2.notch();
			case BH: c1.process(x, g1, p.R1); c2.process(x, g2, p.R2); return c1.bp + c2.hp;
			case HH: c1.process(x, g1, p.R1); c2.process(c1.hp, g2, p.R2); return c2.hp;
		}
		return 0.f;
	}

	/** x: normalised input (1.0 = 5 V); fs: host sample rate. Returns the normalised output. */
	float process(float x, const Params& p, float fs) {
		float fsOs = fs * OVERSAMPLE;
		float g1 = gain(p.f1, fsOs), g2 = gain(p.f2, fsOs);
		float acc = 0.f;
		for (int i = 0; i < OVERSAMPLE; i++) {
			float t = float(i + 1) / OVERSAMPLE;
			float xi = xPrev + (x - xPrev) * t;
			acc += step(xi, p, fsOs, g1, g2);
		}
		xPrev = x;
		float y = acc / OVERSAMPLE;
		if (!std::isfinite(y)) {
			reset();
			return 0.f;
		}
		return y;
	}
};

}  // namespace zemun
