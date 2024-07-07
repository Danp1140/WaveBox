#version 460

#define G 9.8
#define RK4_STEP 0.001
#define RK4_DEFAULT_INIT 0.01 // TODO: more specific naming

struct LinearWave {
	uint wavetype;
	uint depthtype;
	vec2 k;
	float H, omega, d;
};

layout (std430, set = 0, binding = 0) buffer WaveBuffer {
	LinearWave data[];
} linearwaves;

layout (set = 0, binding = 1) uniform sampler2D depth;

layout (binding = 2, rg16f) uniform image2D kmap;

float dispRel(float k, float d) {
	return sqrt(G * k * tanh(k * d));
}

float dkdomega(float k, float d) {
	return (2 * dispRel(k, d)) 
		/ (G * (tanh(k * d) + k * d * pow(1 / cosh(k * d), 2)));
}

float dkdd(float k, float d) {
	return G * k / 4 * sinh(k * d) * cosh(k * d) * (tanh(k * d) + k * d / cosh(k * d));
}

float invDispRel(float omega, float d) {
	float k1, k2, k3, k4, kn = RK4_DEFAULT_INIT;
	for (float o = dispRel(RK4_DEFAULT_INIT, d); o <= omega; o += RK4_STEP) {
		k1 = dkdomega(kn, d);
		k2 = dkdomega(kn + RK4_STEP * k1 / 2, d);
		k3 = dkdomega(kn + RK4_STEP * k2 / 2, d);
		k4 = dkdomega(kn + RK4_STEP * k3, d);
		kn = kn + RK4_STEP / 6 * (k1 + 2 * k2 + 2 * k3 + k4);
	}
	return kn;
}

float refractTheta(float k, float theta0, float dkdx, float dkdy) {
	float k1, k2, k3, k4, thetan = 0;
	// for (vec2 v = vec2(0); v <
	return theta0;
}

void main() {
	const ivec2 gicoords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	if (linearwaves.data[gl_GlobalInvocationID.z].wavetype == 0) {
		LinearWave wave = linearwaves.data[gl_GlobalInvocationID.z];

		float newd = -texture(depth, vec2(gicoords) / 128.).r;
		float newk = invDispRel(wave.omega, newd);
		float dkdx = dkdd(newk, newd) * (newd - texture(depth, vec2(gicoords.x + 1, gicoords.y) / 128).r);
		float dkdy = dkdd(newk, newd) * (newd - texture(depth, vec2(gicoords.x, gicoords.y + 1) / 128).r);
		float newtheta = refractTheta(newk, atan(wave.k.y, wave.k.x), dkdx, dkdy);
		
		if (newd < 0.) newk = 0.;
		/*
		 * TODO:
		 * Factor in theta, Ks, and Kr
		 */
		imageStore(
			kmap, 
			gicoords, 
			vec4(newk * vec2(cos(newtheta), sin(newtheta)), 0, 0));
	}
}
