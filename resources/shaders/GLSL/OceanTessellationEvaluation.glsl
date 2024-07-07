#version 460

#define NORM_CALC_DX 0.001
#define NORM_CALC_DY 0.001

layout(set = 0, binding = 0) uniform sampler2D heightsampler;

layout(set = 0, binding = 3) uniform sampler2D displacementsampler;

layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec2 dmuvmin, dmuvmax;
	vec3 camerapos;
} constants;

layout(triangles, equal_spacing, cw) in;

layout(location = 0) in vec2 uvsin[];
layout(location = 1) in vec3 normsin[];

layout(location = 0) out vec2 uvout;
layout(location = 1) out vec3 normout;
layout(location = 2) out vec3 posout;

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
	float height = 0;
	vec3 disp;
	normout = triInterp(normsin[0], normsin[1], normsin[2]);
	//if (vertexposition.y == 0) {
		if (uvout.x > constants.dmuvmax.x || uvout.y > constants.dmuvmax.y
			|| uvout.x < constants.dmuvmin.x || uvout.y < constants.dmuvmin.y) {
			disp = vec3(0, texture(heightsampler, uvout).r, 0);
			normout = normalize(cross(vec3(1., 0., texture(heightsampler, uvout + vec2(NORM_CALC_DX, 0.)).r - disp.y) , 
				vec3(0., 1., texture(heightsampler, uvout + vec2(0, NORM_CALC_DY)).r - disp.y)));
		}
		else {
			vec2 dispuv = (uvout - constants.dmuvmin) / (constants.dmuvmax - constants.dmuvmin);
			disp = texture(displacementsampler, dispuv).xyz;
			normout = normalize(cross(vec3(1., 0., texture(displacementsampler, dispuv + vec2(NORM_CALC_DX, 0.)).g - disp.y) , 
				vec3(0., 1., texture(displacementsampler, dispuv + vec2(0, NORM_CALC_DY)).g - disp.y)));
		}
	//}
	//else disp = vec3(0, 100, 0);
	posout = vertexposition.xyz + disp;
	gl_Position = constants.cameravp * vec4(posout, vertexposition.w);
}
