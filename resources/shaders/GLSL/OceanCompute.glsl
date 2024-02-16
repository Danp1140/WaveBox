#version 460 

#define X_SCALE 10. // should be made into a spec const later
#define X_RESOLUTION 2048.
#define DX (X_SCALE / X_RESOLUTION)

struct LinearWave {
	uint wavetype;
	uint depthtype;
	float H, k, omega, d;
};

layout(push_constant) uniform Constants {
	float t;
} constants;

layout (binding = 0, r32f) uniform image2D height;

layout (std430, set = 0, binding = 1) buffer WaveBuffer {
	LinearWave data[];
} linearwaves;

void main() {
	// imageStore(height, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), vec4(0.05 * sin(float(gl_GlobalInvocationID.y) * 0.01 + constants.t * 5.)));
	imageStore(
		height,
		ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y),
		vec4(linearwaves.data[0].H * cos(linearwaves.data[0].k * float(gl_GlobalInvocationID.x) * DX - linearwaves.data[0].omega * constants.t))
	);
}
