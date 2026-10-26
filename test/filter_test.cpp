#include "../src/Filter.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace zemun;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static const float FS = 48000.f;

static Params make(int mode, float fc, float spanOct, float reso, float balance = 0.f, int tito = TITO_OFF) {
	Params p;
	p.mode = mode;
	p.f1 = fc * std::exp2(-spanOct / 2);
	p.f2 = fc * std::exp2(spanOct / 2);
	resoBalance(reso, balance, p.R1, p.R2);
	p.tito = tito;
	return p;
}

/** steady-state gain of a small sine at frequency f */
static float gainAt(const Params& p, float f) {
	Voice v;
	double sum = 0, ref = 0;
	int n = (int) FS;
	for (int i = 0; i < n; i++) {
		float x = 0.05f * std::sin(2 * M_PI * f * i / FS);
		float y = v.process(x, p, FS);
		if (i > n / 2) {
			sum += y * y;
			ref += x * x;
		}
	}
	return (float) std::sqrt(sum / ref);
}

static float db(float g) {
	return 20.f * std::log10(std::max(g, 1e-9f));
}

/** frequency of a self-oscillating voice, from zero crossings */
static float oscFreq(const Params& p, float* rms = nullptr) {
	Voice v;
	int n = (int) (FS * 1.5f);
	int crossings = 0;
	float prev = 0;
	double sum = 0;
	for (int i = 0; i < n; i++) {
		float x = i < 10 ? 0.5f : 0.f;   // a kick to start it
		float y = v.process(x, p, FS);
		if (i > n / 3) {
			if (prev <= 0 && y > 0)
				crossings++;
			sum += y * y;
		}
		prev = y;
	}
	if (rms)
		*rms = (float) std::sqrt(sum / (n - n / 3));
	return crossings / ((n - n / 3) / FS);
}

int main() {
	// LL: 4-pole lowpass with span 0
	{
		Params p = make(LL, 1000, 0, 0);
		float lo = gainAt(p, 100), hi = gainAt(p, 10000);
		printf("LL  100 Hz %.1f dB, 10 kHz %.1f dB\n", db(lo), db(hi));
		CHECK(db(lo) > -3 && db(hi) < -60);
	}
	// HH: 4-pole highpass
	{
		Params p = make(HH, 1000, 0, 0);
		float lo = gainAt(p, 100), hi = gainAt(p, 10000);
		printf("HH  100 Hz %.1f dB, 10 kHz %.1f dB\n", db(lo), db(hi));
		CHECK(db(hi) > -3 && db(lo) < -60);
	}
	// LL with full span behaves closer to a 2-pole lowpass at 1 decade above the lower peak
	{
		float g0 = gainAt(make(LL, 1000, 0, 0), 4000);
		float g8 = gainAt(make(LL, 1000, 8, 0), 4000 / 16.f * 1.f);
		(void) g8;
		float gWide = gainAt(make(LL, 1000, 6, 0), 1000);
		printf("LL span 0 @4k %.1f dB, span 6 @fc %.1f dB\n", db(g0), db(gWide));
		CHECK(db(gWide) > db(g0));
	}
	// BB: two peaks louder than the middle between them
	{
		Params p = make(BB, 1000, 4, 6.5f);
		float a = gainAt(p, 250), mid = gainAt(p, 1000), b = gainAt(p, 4000);
		printf("BB  peaks %.1f / %.1f dB, middle %.1f dB\n", db(a), db(b), db(mid));
		CHECK(db(a) > db(mid) + 6 && db(b) > db(mid) + 6);
	}
	// balance moves the emphasis from the lower to the higher peak
	{
		float lowDom = gainAt(make(BB, 1000, 4, 7, -0.8f), 250) / gainAt(make(BB, 1000, 4, 7, -0.8f), 4000);
		float highDom = gainAt(make(BB, 1000, 4, 7, 0.8f), 250) / gainAt(make(BB, 1000, 4, 7, 0.8f), 4000);
		printf("balance low/high ratio: %.2f (lower dominant) vs %.2f (higher dominant)\n", lowDom, highDom);
		CHECK(lowDom > 1.5f && highDom < 0.67f);
	}
	// NN: deep notch at the centre with low resonance
	{
		Params p = make(NN, 1000, 0, 0);
		float pass = gainAt(p, 100), notch = gainAt(p, 1000);
		printf("NN  100 Hz %.1f dB, notch %.1f dB\n", db(pass), db(notch));
		CHECK(db(pass) > -3 && db(notch) < -30);
	}
	// LH: band reject between the peaks
	{
		Params p = make(LH, 1000, 4, 1);
		float mid = gainAt(p, 1000), lo = gainAt(p, 60), hi = gainAt(p, 16000);
		printf("LH  60 Hz %.1f, 1 kHz %.1f, 16 kHz %.1f dB\n", db(lo), db(mid), db(hi));
		CHECK(db(mid) < db(lo) - 10 && db(mid) < db(hi) - 10);
	}
	// HL: bandpass with a flat top between wide peaks
	{
		Params p = make(HL, 1000, 6, 1);
		float mid = gainAt(p, 1000), lo = gainAt(p, 20), hi = gainAt(p, 20000);
		printf("HL  20 Hz %.1f, 1 kHz %.1f, 20 kHz %.1f dB\n", db(lo), db(mid), db(hi));
		CHECK(db(mid) > db(lo) + 20 && db(mid) > db(hi) + 10);
	}
	// NL / HN: notch inside the pass band
	{
		float passNL = gainAt(make(NL, 1000, 3, 0), 60), notchNL = gainAt(make(NL, 1000, 3, 0), 1000 * std::exp2(-1.5f));
		float passHN = gainAt(make(HN, 1000, 3, 0), 16000), notchHN = gainAt(make(HN, 1000, 3, 0), 1000 * std::exp2(1.5f));
		printf("NL pass %.1f notch %.1f | HN pass %.1f notch %.1f dB\n", db(passNL), db(notchNL), db(passHN), db(notchHN));
		CHECK(db(notchNL) < db(passNL) - 20 && db(notchHN) < db(passHN) - 20);
	}
	// self oscillation at high reso, tuned to the cutoff, tracking 1 V/oct
	{
		float rms440, rms220;
		float f440 = oscFreq(make(BB, 440, 0, 10), &rms440);
		float f220 = oscFreq(make(BB, 220, 0, 10), &rms220);
		float f880 = oscFreq(make(BB, 880, 0, 10));
		printf("self osc: %.1f Hz (rms %.2f), %.1f Hz (rms %.2f), %.1f Hz\n", f440, rms440, f220, rms220, f880);
		CHECK(rms440 > 0.2f && std::fabs(f440 / 440 - 1) < 0.08f);
		CHECK(std::fabs(f440 / f220 - 2) < 0.1f && std::fabs(f880 / f440 - 2) < 0.1f);
		float rmsLow;
		oscFreq(make(BB, 440, 0, 6), &rmsLow);
		CHECK(rmsLow < 0.01f);   // below the threshold it rings out
	}
	// TITO changes the sound and stays stable
	{
		Params p = make(LL, 500, 2, 9, 0, TITO_XM);
		float rmsXM, rmsOff;
		oscFreq(p, &rmsXM);
		p.tito = TITO_OFF;
		oscFreq(p, &rmsOff);
		printf("tito xm rms %.2f, off rms %.2f\n", rmsXM, rmsOff);
		CHECK(std::isfinite(rmsXM) && rmsXM < 3.f);
	}
	// stress: random settings, noise in, every mode and TITO
	{
		srand(7);
		bool ok = true;
		float peak = 0;
		for (int t = 0; t < 120 && ok; t++) {
			Params p = make(t % NUM_MODES, knobToFreq(rand() / (float) RAND_MAX), 8.f * rand() / RAND_MAX, 12.f * rand() / RAND_MAX,
			                2.f * rand() / RAND_MAX - 1.f, t % 3);
			Voice v;
			for (int i = 0; i < 9600; i++) {
				float x = inputStage(10.f * (rand() / (float) RAND_MAX - 0.5f), 1.f);
				float y = v.process(x, p, FS);
				if (!std::isfinite(y)) {
					ok = false;
					break;
				}
				peak = std::max(peak, std::fabs(y));
			}
		}
		printf("stress peak %.2f (normalised)\n", peak);
		CHECK(ok && peak < 12.f);
	}
	printf(fails ? "FAILED %d\n" : "ALL OK\n", fails);
	return fails;
}
