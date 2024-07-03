#version 460

#define MAX_TESS_LEVEL_OUTER 16
#define MAX_TESS_LEVEL_INNER 16

layout(vertices=3) out;

layout(location = 0) in vec2 uvin[];

layout(location = 0) out vec2 uvout[];

void main() {
	gl_TessLevelOuter[0] = MAX_TESS_LEVEL_OUTER;
	gl_TessLevelOuter[1] = MAX_TESS_LEVEL_OUTER;
	gl_TessLevelOuter[2] = MAX_TESS_LEVEL_OUTER;
	gl_TessLevelInner[0] = MAX_TESS_LEVEL_INNER;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	uvout[gl_InvocationID] = uvin[gl_InvocationID];
}
