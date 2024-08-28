#version 460 

#define X_SCALE 100. // should be made into a spec const later
#define X_RESOLUTION 2048.
#define DX (X_SCALE / X_RESOLUTION)

#define G 9.8
#define RK4_STEP 0.001
#define RK4_DEFAULT_INIT 0.01 // TODO: more specific naming

#define ENCKE_NUM_DIVS 4.

#define CNOIDAL_WAVE_NUM_MACLAURIN_TERMS 6
#define MACLAURIN_N 6

struct LinearWave {
	uint wavetype;
	uint depthtype;
	vec2 k;
	float H, omega, d;
};
struct CnoidalWave {
	uint correspondinglinidx;
	float ellipticalk, bigK;
	float snMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS], 
	      cnMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS], 
	      dnMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS];
};

layout(push_constant) uniform Constants {
	float t;
	uint flags;
} constants;

layout (binding = 0, r32f) uniform image2D height;

layout (std430, set = 0, binding = 1) readonly buffer LinearWaveBuffer {
	LinearWave data[];
} linearwaves;
layout (std430, set = 0, binding = 2) readonly buffer CnoidalWaveBuffer {
	CnoidalWave data[];
} cnoidalwaves;
layout (set = 0, binding = 3) uniform sampler2D depth;
layout (set = 0, binding = 4) uniform sampler2D kmap;

float fact(int n) {
	if (n == 0) return 1.;
	else if (n == 1) return 1.;
	else if (n == 2) return 2.;
	else if (n == 3) return 6.;
	else if (n == 4) return 24.;
	else if (n == 5) return 120.;
	else if (n == 6) return 720.;
	else if (n == 7) return 5040.;
	else if (n == 8) return 40320.;
	else if (n == 9) return 362880.;
	float f = n;
	while (n != 1) {
		n--;
		f *= n;
	}
	return f;
}

// in the future this should be stored in the same place as K(k)
// these are precalc'd for k = 0.5, like K
#define M 0.5
const float maclaurinSnTerms[6] = {
	1,
	 - (1 + M) / 6,
	(1 + pow(M, 2) + 14 * M) / 120,
	 - (1 + pow(M, 3) + 135 * M * (1 + M)) / 5040,
	(1 + pow(M, 5) + 11069 * M * (1 + pow(M, 3)) + 165826 * pow(M, 2) * (1 + M)) / 39916800,
	(1 + pow(M, 6) + 99642 * M * (1 + pow(M, 4)) + 4494351 * pow(M, 2) * (1 + pow(M, 2)) + 13180268 * pow(M, 3)) / 6227020800.
};
const float maclaurinCnTerms[6] = {
	1,
	 - 0.5,
	(1 + 4 * M) / 24,
	 - (1 + 44 * M + 16 * pow(M, 2)) / 720,
	(1 + 408 * M + 912 * pow(M, 2) + 64 * pow(M, 3)) / 40320,
	(1 + 33212 * M + 870640 * pow(M, 2) + 1538560 * pow(M, 3) + 249328 * pow(M, 4) + 1024 * pow(M, 5)) / 479001600
};
const float maclaurinDnTerms[6] = {
	1,
	 - M / 2,
	(4 * M + pow(M, 2)) / 24,
	 - (16 * M + 44 * pow(M, 2) + pow(M, 3)) / 720,
	(64 * M + 912 * pow(M, 2) + 408 * pow(M, 3) + pow(M, 4)) / 40320,
	(1024 * M + 259328 * pow(M, 2) + 1538560 * pow(M, 3) + 870640 * pow(M, 4) + 33212 * pow(M, 5) + pow(M, 6)) / 479001600
};

float maclaurinSn(float u, uint i) {
	if (u == 0) return 0;
	float s = 0;
	for (uint j = 0; j < MACLAURIN_N; j++) {
		s += cnoidalwaves.data[i].snMaclaurinTerms[j] * pow(u, 2 * float(j) + 1);
	}
	return s;
}

float maclaurinCn(float u, uint i) {
	if (u == 0) return 1;
	float c = 0;
	for (uint j = 0; j < MACLAURIN_N; j++) {
		c += cnoidalwaves.data[i].cnMaclaurinTerms[j] * pow(u, 2 * float(j));
	}
	return c;
}

float maclaurinDn(float u, uint i) {
	if (u == 0) return 1;
	float d = 0;
	for (uint j = 0; j < MACLAURIN_N; j++) {
		d += cnoidalwaves.data[i].dnMaclaurinTerms[j] * pow(u, 2 * float(j));
	}
	return d;
}

// algorithm we use for this actually solves for sn, cn, and dn, so if we ever need those we could just pass out all three...
// this method is somewhat slow, but maybe as good as we get in terms of ratio of precision to # ops
// k is in [0, 1]
//
/*
float cn(float u, float k) {
	// below is not actual period, its the space u should be transformed into according to the paper were referencing
	const float period = 1.45126237045 / 4.; // in reality this is K / 4 (this is K(k = 0.5)
	float littleu = mod(u, period) / pow(2., ENCKE_NUM_DIVS);
	float littlesn = maclaurinSn(littleu, k),
	      littlecn = maclaurinCn(littleu, k),
	      littledn = maclaurinDn(littleu, k);
	float Dsn = littleu - littlesn,
	      Dcn = 1 - littlecn,
	      Ddn = 1 - littledn,
	      newDsn, newDcn, newDdn;
	for (uint i = 0; i < ENCKE_NUM_DIVS; i++) {
		newDsn = 2 * (littlecn * littledn * Dsn + u * (Dcn + Ddn - Dcn * Ddn - k * pow(littlesn, 4))) / (1 - k * pow(littlesn, 4));
		newDcn = (1 + littlecn) * Dcn + (1 - 2 * k * pow(littlesn, 2)) * pow(littlesn, 2) / (1 - k * pow(littlesn, 4));
		newDdn = (1 + littledn) * Ddn + k * (1 - 2 * pow(littlesn, 2)) * pow(littlesn, 2) / (1 - k * pow(littlesn, 4));
		Dsn = newDsn;
		Dcn = newDcn;
		Ddn = newDdn;
		littleu *= 2.;
		littlesn = u - Dsn;
		littlecn = 1 - Dcn;
		littledn = 1 - Ddn;
	}
	return littlecn;
}
*/

float cn2(float u, uint i) {
	const float K = cnoidalwaves.data[i].bigK; 
	const float k = cnoidalwaves.data[i].ellipticalk;
	bool neg = false, foldone = false, foldtwo = false, foldthree = false, foldfour = false;
	// could limit num ops by setting appropriate fold flags and then just doing one modulus at the end...???
	if (u < 0) {
		neg = true;
		u *= -1;
	}
	u = mod(u, K * 4);
	if (u >= K * 2) {
		foldone = true;
		u -= K * 2;
	}
	if (u >= K) {
		foldtwo = true;
		u -= K;
	}
	if (u >= K / 2) {
		foldthree = true;
		u = K - u;
	}
	if (u >= K / 4) {
		foldfour = true;
		u = K / 2 - u;
	}
	float littleu = u / pow(2., ENCKE_NUM_DIVS);
	float littlesn = maclaurinSn(littleu, i),
	      littlecn = maclaurinCn(littleu, i),
	      littledn = maclaurinDn(littleu, i);
	float newsn, newcn, newdn;
	for (uint i = 0; i < ENCKE_NUM_DIVS; i++) {
		newsn = 2. * littlesn * littlecn * littledn / (1. - k * pow(littlesn, 4.));
		newcn = (pow(littlecn, 2.) - pow(littlesn, 2.) * pow(littledn, 2.)) / (1. - k * pow(littlesn, 4.));
		newdn = (pow(littledn, 2.) - k * pow(littlesn, 2.) * pow(littlecn, 2.)) / (1. - k * pow(littlesn, 4.));
		littleu *= 2.;
		littlesn = newsn;
		littlecn = newcn;
		littledn = newdn;
	}
	if (foldfour) {
		float kprime = sqrt(1 - k);
		float tempsn = littlesn, tempcn = littlecn;
		littlesn = sqrt(1 + kprime) * (tempcn * littledn - kprime * tempsn) / (1 + kprime - k * pow(tempsn, 2));
		littlecn = sqrt(kprime * (1 + kprime)) * (tempcn + tempsn * littledn) / (1 + kprime - k * pow(tempsn, 2));
		littledn = sqrt(kprime) * ((1 + kprime) * littledn + k * tempsn * tempcn) / (1 + kprime - k * pow(tempsn, 2));
	}
	if (foldthree) {
		float kprime = sqrt(1 - k);
		float tempsn = littlesn;
		littlesn = littlecn / littledn;
		littlecn = kprime * tempsn / littledn;
		littledn = kprime / littledn;
	}
	if (foldtwo) {
		float kprime = sqrt(1 - k);
		float tempsn = littlesn;
		littlesn = littlecn / littledn;
		littlecn = -kprime * tempsn / littledn; 
		littledn = kprime / littledn;
	}
	if (foldone) {
		littlecn = -littlecn; 
		littlesn = -littlesn;
	}
	if (neg) {
		littlesn = -littlesn;
	}
	return littlecn;
}

void main() {
	const ivec2 gicoords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	if (gl_GlobalInvocationID.z == 0) {
		imageStore(height, gicoords, vec4(0.));
	}
	const LinearWave wave = linearwaves.data[gl_GlobalInvocationID.z];
	if (wave.H / -texture(depth, vec2(gicoords) / X_RESOLUTION).r > 0.78
		|| texture(depth, vec2(gicoords) / X_RESOLUTION).r >= 0) {
		imageStore(height, gicoords, vec4(0.));
		// return;
	}
	if (wave.wavetype == 0) {
		imageStore(
			height,
			gicoords,
			imageLoad(height, gicoords) 
			//  + vec4(wave.H * cos(dot(wave.k, vec2(gicoords) * DX) - wave.omega * constants.t))
			 + vec4(wave.H * cos(dot(texture(kmap, vec2(gicoords) / X_RESOLUTION).rg, vec2(gicoords) * DX) - wave.omega * constants.t))
		);
	}
	if (wave.wavetype == 1) {
		uint cnidx = 0;
		// for (; cnoidalwaves.data[cnidx].correspondinglinidx == gl_GlobalInvocationID.z; cnidx++) {}
		imageStore(
			height,
			gicoords,
			imageLoad(height, gicoords) 
			// + vec4(20)
			// + vec4(cn2(float(gicoords.x) / X_RESOLUTION * 1.45126 * 4, 0.5) * 100.)
			 + vec4(wave.H * pow(cn2(
				// cnoidalwaves.data[cnidx].bigK * 2 * (dot(wave.k, vec2(gicoords) * DX) - wave.omega * constants.t), 
				cnoidalwaves.data[cnidx].bigK * 2 * (dot(texture(kmap, vec2(gicoords) / X_RESOLUTION).rg, vec2(gicoords) * DX) - wave.omega * constants.t), 
				cnidx), 
				2))
		);

	}
}
