#version 460

// on my machine, 256 is the most the GPU will do
#define MAX_TESS_LEVEL_OUTER 64
#define MAX_TESS_LEVEL_INNER 64

#define EDGE_TESS_SCALING 0.01
#define FACE_TESS_SCALING 0.001

layout(vertices=3) out;

layout(location = 0) in vec2 uvin[];
layout(location = 1) in vec3 normin[];

layout(location = 0) out vec2 uvout[];
layout(location = 1) out vec3 normout[];

void main() {
	/*
	 * below is semi-working dynamic tessellation based off camera space geometry. face 
	 * tessellation seems to be working fine but edge is not.
	 */
	/*
	float a = abs(distance(gl_in[gl_InvocationID].gl_Position.xy, gl_in[(gl_InvocationID + 1) % 3].gl_Position.xy));
	gl_TessLevelOuter[gl_InvocationID] = max(a * EDGE_TESS_SCALING, 1);
	if (gl_InvocationID == 0) {
		float b = abs(distance(gl_in[1].gl_Position.xy, gl_in[2].gl_Position.xy)), 
		      c = abs(distance(gl_in[2].gl_Position.xy, gl_in[0].gl_Position.xy));
		float s = (a + b + c) / 2;
		float A = sqrt(s * (s - a) * (s - b) * (s - c));
		// i'm actually thinking that face tessellation should get more agressive with a shallower view
		// angle, to better demonstrate bumps
		gl_TessLevelInner[0] = max(A * FACE_TESS_SCALING, 1);
	}
	*/

	if (true) {
		gl_TessLevelOuter[0] = MAX_TESS_LEVEL_OUTER;
		gl_TessLevelOuter[1] = MAX_TESS_LEVEL_OUTER;
		gl_TessLevelOuter[2] = MAX_TESS_LEVEL_OUTER;
		gl_TessLevelInner[0] = MAX_TESS_LEVEL_INNER;
	}
	else {
		gl_TessLevelOuter[0] = 1;
		gl_TessLevelOuter[1] = 1;
		gl_TessLevelOuter[2] = 1;
		gl_TessLevelInner[0] = 1;
	}

	// below is beginning of face-incidence scaling, but realized i need to hack together some PCs lol
	/*
	if (gl_InvocationID == 0) {
		float scaling = dot(distance(gl_in[0].gl_Position, constants.camera), cross(gl_in[0].gl_Position, gl_in[1].gl_Position));
	}
	*/

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	uvout[gl_InvocationID] = uvin[gl_InvocationID];
	normout[gl_InvocationID] = normin[gl_InvocationID];
}
