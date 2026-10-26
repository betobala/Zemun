#include "../src/Filter.hpp"
#include <cstdio>
using namespace zemun;
int main() {
	const float FS = 48000.f;
	for (int mode : {LL, BB, HH}) {
		Params a; a.mode = mode; a.f1 = 500 * 0.5f; a.f2 = 500 * 2; resoBalance(9, 0, a.R1, a.R2);
		Params b = a, c = a; b.tito = TITO_SM; c.tito = TITO_XM; a.tito = TITO_OFF;
		Voice va, vb, vc; double dSM = 0, dXM = 0, e = 0;
		for (int i = 0; i < 48000; i++) {
			float x = 0.4f * std::sin(2 * M_PI * 110 * i / FS);
			float ya = va.process(x, a, FS), yb = vb.process(x, b, FS), yc = vc.process(x, c, FS);
			if (i > 24000) { dSM += (yb - ya) * (yb - ya); dXM += (yc - ya) * (yc - ya); e += ya * ya; }
		}
		printf("%s: rms off %.3f, diff SM %.3f, diff XM %.3f\n", MODE_NAMES[mode], std::sqrt(e / 24000), std::sqrt(dSM / 24000), std::sqrt(dXM / 24000));
	}
}
