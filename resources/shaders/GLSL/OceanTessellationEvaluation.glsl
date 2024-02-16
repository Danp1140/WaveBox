#version 460

#define NORM_CALC_DX 0.01
#define NORM_CALC_DY 0.01

layout(set = 0, binding = 0) uniform sampler2D heightsampler;

layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec3 camerapos;
} constants;

layout(triangles, equal_spacing, cw) in;

layout(location = 0) in vec2 uvsin[];

layout(location = 0) out vec2 uvout;
layout(location = 1) out vec3 normout;

vec2 triInterp(vec2 a, vec2 b, vec2 c){
    return a * gl_TessCoord.x + b * gl_TessCoord.y + c * gl_TessCoord.z;
}
vec3 triInterp(vec3 a, vec3 b, vec3 c){
    return a * gl_TessCoord.x + b * gl_TessCoord.y + c * gl_TessCoord.z;
}
vec4 triInterp(vec4 a, vec4 b, vec4 c){
    return a * gl_TessCoord.x + b * gl_TessCoord.y + c * gl_TessCoord.z;
}

void main() {
	uvout = triInterp(uvsin[0], uvsin[1], uvsin[2]);
	vec4 vertexposition = triInterp(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
	float height = texture(heightsampler, uvout).r;
	gl_Position = constants.cameravp * (vertexposition + vec4(0., height, 0., 0.));
	normout = cross(vec3(1., 0., texture(heightsampler, uvout + vec2(NORM_CALC_DX, 0.)).r - height) , 
				vec3(0., 1., texture(heightsampler, uvout + vec2(0, NORM_CALC_DY)).r - height));
}
