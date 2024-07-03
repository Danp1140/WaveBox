#version 460 

#define X_SCALE 100. // should be made into a spec const later
#define X_RESOLUTION 2048.
#define DX (X_SCALE / X_RESOLUTION)

#define G 9.8
#define RK4_STEP 0.001
#define RK4_DEFAULT_INIT 0.01 // TODO: more specific naming

#define ENCKE_NUM_DIVS 6.

struct LinearWave {
	uint wavetype;
	uint depthtype;
	vec2 k;
	float H, omega, d;
};

layout(push_constant) uniform Constants {
	float t;
	uint flags;
} constants;

layout (binding = 0, r32f) uniform image2D height;

layout (std430, set = 0, binding = 1) buffer WaveBuffer {
	LinearWave data[];
} linearwaves;

layout (set = 0, binding = 2) uniform sampler2D depth;

layout (set = 0, binding = 3) uniform sampler2D kmap;

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

float maclaurinSn(float u, float k) {
	return u - (1 + pow(k, 2)) * pow(u, 3) / fact(3) + (1 + 14 * pow(k, 2) + pow(k, 4)) * pow(u, 5) / fact(5);
}

float maclaurinCn(float u, float k) {
	return 1 - pow(u, 2) / fact(2) + (1 + 4 * pow(k, 2)) * pow(u, 4) / fact(4);
}

float maclaurinDn(float u, float k) {
	return 1 - pow(k, 2) * pow(u, 2) / fact(2) + (4 + pow(k, 2)) * pow(u, 4) / fact(4);
}

// algorithm we use for this actually solves for sn, cn, and dn, so if we ever need those we could just pass out all three...
// this method is somewhat slow, but maybe as good as we get in terms of ratio of precision to # ops
// k is in [0, 1]
//
float cn(float u, float k) {
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

float cn2(float u, float k) {
	// cn is periodic on 4K(k), the std domain for the paper we're referencing is 0 < u < K(k) / 4
	const float K = 1.45126237045; // precalc'd for k = 0.5
	if (u > K / 4.) return 0.;
	float littleu = u / pow(2., ENCKE_NUM_DIVS);
	float littlesn = maclaurinSn(littleu, k),
	      littlecn = maclaurinCn(littleu, k),
	      littledn = maclaurinDn(littleu, k);
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
	return littlecn;
}

void main() {
	const ivec2 gicoords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	// imageStore(height, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), vec4(0.05 * sin(float(gl_GlobalInvocationID.y) * 0.01 + constants.t * 5.)));
	if (gl_GlobalInvocationID.z == 0) {
		imageStore(height, gicoords, vec4(0.));
	}
	if (linearwaves.data[gl_GlobalInvocationID.z].wavetype == 0) {
		LinearWave wave = linearwaves.data[gl_GlobalInvocationID.z];
		imageStore(
			height,
			gicoords,
			imageLoad(height, gicoords) 
			// + vec4(wave.H * cos(dot(wave.k, vec2(gicoords) * DX) - wave.omega * constants.t))
			// + vec4(wave.H * cos(dot(texture(kmap, vec2(gicoords) / X_RESOLUTION).rg, vec2(gicoords) * DX) - wave.omega * constants.t))
			+ vec4(cn2(float(gicoords.x) / X_RESOLUTION, 0.5) * 100.)
		);
		if (wave.H / -texture(depth, vec2(gicoords) / X_RESOLUTION).r > 0.78
			|| texture(depth, vec2(gicoords) / X_RESOLUTION).r >= 0 ) {
			// imageStore(height, gicoords, vec4(0.));
		}
	}
}
