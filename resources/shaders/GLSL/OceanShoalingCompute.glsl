#version 460

#define DM_RES 512

struct LinearWave {
	uint wavetype;
	uint depthtype;
	vec2 k;
	float H, omega, d;
};

layout(push_constant) uniform Constants {
	vec2 worldscale, worldoffset;
	float t;
} constants;

layout (std430, set = 0, binding = 0) buffer WaveBuffer {
	LinearWave data[];
} linearwaves;

layout(binding = 1, rgba16f) uniform image2D displacement;

layout(set = 0, binding = 2) uniform sampler2D kmap;

void main() {
	const ivec2 gi = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	const vec2 uv = vec2(gi) / float(DM_RES);
	const vec2 pos = constants.worldscale * uv + constants.worldoffset;

	if (linearwaves.data[gl_GlobalInvocationID.z].wavetype == 0) {
		LinearWave wave = linearwaves.data[gl_GlobalInvocationID.z];
		vec4 value;
		if (gl_GlobalInvocationID.z > 0) value = imageLoad(displacement, gi);
		else value = vec4(0);

		vec2 thisk = texture(kmap, (pos + 50.) / 100.).rg;
		if (length(thisk) == 0. || atanh(pow(wave.omega, 2) / 9.8 / length(thisk)) / length(thisk) < wave.H / 0.78) value = vec4(0.);
		else value += vec4(0, wave.H * cos(dot(thisk, pos) - wave.omega * constants.t), 0, 0);


		imageStore(displacement, gi, value);
	}
}
