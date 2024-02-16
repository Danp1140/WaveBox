#version 460

#define LIGHT_POSITION vec3(0.5, 1., 0.5)
#define SPECULAR_EXP 5.

layout(set = 0, binding = 0) uniform sampler2D heightsampler;

layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec3 camerapos;
} constants;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec4 color;

void main() {
	vec3 halfway = normalize(-constants.camerapos - LIGHT_POSITION);
	float specular = pow(max(dot(halfway, norm), 0.), SPECULAR_EXP);

	float lambertian=max(dot(LIGHT_POSITION, norm), 0);
	// float lambertian=max(dot(LIGHT_POSITION, vec3(0., 0., 1.)), 0);
	color = pow((vec4(1.) * lambertian + vec4(1.) * specular) / length(LIGHT_POSITION), vec4(1. / 2.2));
	color.w = 1.;
}
