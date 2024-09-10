#version 460 

#define X_SCALE 100. // should be made into a spec const later
#define X_RESOLUTION 1024.
#define DX (X_SCALE / X_RESOLUTION)

#define G 9.8
#define RK4_STEP 0.001
#define RK4_DEFAULT_INIT 0.01 // TODO: more specific naming

#define ENCKE_NUM_DIVS 2.

#define CNOIDAL_WAVE_NUM_MACLAURIN_TERMS 7

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

float maclaurinSn(float u, uint i) {
	if (u == 0) return 0;
	float s = 0;
	for (uint j = 0; j < CNOIDAL_WAVE_NUM_MACLAURIN_TERMS; j++) {
		s += cnoidalwaves.data[i].snMaclaurinTerms[j] * pow(u, 2 * float(j) + 1);
	}
	return s;
}

float salasSn(float u, uint i) {
	const float m = cnoidalwaves.data[i].ellipticalk;
	float lambda = (sqrt(pow(m, 2) - 144 * m + 144) - m - 12) / 14;
	return sin(sqrt(1 + lambda) * u) / sqrt(1 + lambda * pow(cos(sqrt(1 + lambda) * u), 2));
}

float maclaurinCn(float u, uint i) {
	if (u == 0) return 1;
	float c = 0;
	for (uint j = 0; j < CNOIDAL_WAVE_NUM_MACLAURIN_TERMS; j++) {
		c += cnoidalwaves.data[i].cnMaclaurinTerms[j] * pow(u, 2 * float(j));
	}
	return c;
}

float salasCn(float u, uint i) {
	const float m = cnoidalwaves.data[i].ellipticalk;
	float lambda = (sqrt(pow(m, 2) - 144 * m + 144) - m - 12) / 14;
	return (sqrt(1 + lambda) * cos(sqrt(1 + lambda) * u)) / sqrt(1 + lambda * pow(cos(sqrt(1 + lambda) * u), 2));
}

float maclaurinDn(float u, uint i) {
	if (u == 0) return 1;
	float d = 0;
	for (uint j = 0; j < CNOIDAL_WAVE_NUM_MACLAURIN_TERMS; j++) {
		d += cnoidalwaves.data[i].dnMaclaurinTerms[j] * pow(u, 2 * float(j));
	}
	return d;
}

float salasDn(float u, uint i) {
	const float m = cnoidalwaves.data[i].ellipticalk;
	return sqrt(1 - m * pow(salasSn(u, i), 2));
}

// algorithm we use for this actually solves for sn, cn, and dn, so if we ever need those we could just pass out all three...
// this method is somewhat slow, but maybe as good as we get in terms of ratio of precision to # ops
// k is in [0, 1]
float cn2(float u, uint i) {
	const float K = cnoidalwaves.data[i].bigK; 
	const float k = cnoidalwaves.data[i].ellipticalk;
	if (k == 0) return cos(u);
	if (k == 1) return 1 / cosh(u);
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
	/*
	float littlesn = maclaurinSn(littleu, i),
	      littlecn = maclaurinCn(littleu, i),
	      littledn = maclaurinDn(littleu, i);
	      */
	float littlesn = salasSn(littleu, i),
	      littlecn = salasCn(littleu, i),
	      littledn = salasDn(littleu, i);
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
		littlesn = littleu - Dsn;
		littlecn = 1 - Dcn;
		littledn = 1 - Ddn;
	}
	// consider reuse of above variables to prevent conditional float alloc
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
	return littledn;
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
		return;
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
			//   + vec4(cn2(float(gicoords.x) / X_RESOLUTION * 4 * cnoidalwaves.data[cnidx].bigK, cnidx) * 10.)
			// + vec4(salasCn(float(gicoords.x) / X_RESOLUTION * 4 * cnoidalwaves.data[cnidx].bigK, cnidx)) * 10
			 + vec4(wave.H * pow(cn2(
				cnoidalwaves.data[cnidx].bigK * 2 * (dot(wave.k, vec2(gicoords) * DX) - wave.omega * constants.t), 
				//cnoidalwaves.data[cnidx].bigK * 2 * (dot(texture(kmap, vec2(gicoords) / X_RESOLUTION).rg, vec2(gicoords) * DX) - wave.omega * constants.t), 
				cnidx), 
				2))
		);

	}
}
