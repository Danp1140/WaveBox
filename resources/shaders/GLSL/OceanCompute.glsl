#version 460 

#define X_SCALE 100. // should be made into a spec const later
#define X_RESOLUTION 2048.
#define DX (X_SCALE / X_RESOLUTION)

struct LinearWave {
	uint wavetype;
	uint depthtype;
	vec2 k;
	float H, omega, d;
};

layout(push_constant) uniform Constants {
	float t;
	vec2 dmuvmin, dmuvmax;
} constants;

layout (binding = 0, r32f) uniform image2D height;

layout (std430, set = 0, binding = 1) buffer WaveBuffer {
	LinearWave data[];
} linearwaves;

layout (set = 0, binding = 2) uniform sampler2D depthmap;

layout (set = 0, binding = 3) uniform sampler2D kmap;

layout (binding = 4, rgba16f) uniform image2D displacement;

void main() {
	const ivec2 gicoords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	const vec2 uv = vec2(gicoords) / X_RESOLUTION;
	const vec2 pos = (uv - vec2(0.5)) * X_SCALE;
	const ivec2 dmcoords = ivec2((uv - constants.dmuvmin) / (constants.dmuvmax - constants.dmuvmin) * 512.);
	if (gl_GlobalInvocationID.z == 0) {
		imageStore(height, gicoords, vec4(0.));
	}
	if (linearwaves.data[gl_GlobalInvocationID.z].wavetype == 0) {
		LinearWave wave = linearwaves.data[gl_GlobalInvocationID.z];
		float depth = -texture(depthmap, vec2(gicoords) / X_RESOLUTION).r;

		vec4 value;
		if (gl_GlobalInvocationID.z > 0) value = imageLoad(height, gicoords);
		else value = vec4(0);

		if (depth <= 0) value = vec4(0.);
		else value += vec4(wave.H * cos(dot(texture(kmap, uv).rg, pos) - wave.omega * constants.t));

		if (uv.x >= constants.dmuvmax.x || uv.y >= constants.dmuvmax.y
			|| uv.x <= constants.dmuvmin.x || uv.y <= constants.dmuvmin.y) {
			imageStore(
				height,
				gicoords,
				value
			);

		}
		// imageStore(height, gicoords, vec4(10));
	}
}
