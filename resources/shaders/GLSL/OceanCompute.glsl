#version 460 

#define X_SCALE 100. // should be made into a spec const later
#define X_RESOLUTION 2048.
#define DX (X_SCALE / X_RESOLUTION)

#define G 9.8
#define RK4_STEP 0.001
#define RK4_DEFAULT_INIT 0.01 // TODO: more specific naming

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
			+ vec4(wave.H * cos(dot(texture(kmap, vec2(gicoords) / X_RESOLUTION).rg, vec2(gicoords) * DX) - wave.omega * constants.t))
		);
		if (wave.H / -texture(depth, vec2(gicoords) / X_RESOLUTION).r > 0.78
			|| texture(depth, vec2(gicoords) / X_RESOLUTION).r >= 0 ) {
			imageStore(height, gicoords, vec4(0.));
		}
	}
}
