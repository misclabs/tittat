#include "misc_pasta.h"

float MpClampF(float val, float min, float max) {
	float t = val < min ? min : val;
	return t > max ? max : t;
}

int MpClampI(int val, int min, int max) {
	int t = val < min ? min : val;
	return t > max ? max : t;
}
