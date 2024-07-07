#version 460

#define LIGHT_POSITION vec3(0.5, 1., 0.5)
#define SPECULAR_EXP 20.

layout(set = 0, binding = 0) uniform sampler2D heightsampler;
layout(set = 0, binding = 1) uniform sampler2D diffusesampler;
layout(set = 0, binding = 2) uniform samplerCube envsampler;


layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec2 dmuvmin, dmuvmax;
	vec3 camerapos;
} constants;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 pos;

layout(location = 0) out vec4 color;

vec4 colorRamp(vec4 c1, vec4 c2, float xmin, float xmax, float x) {
	return mix(c1, c2, (x - xmin) / xmax);
}

void main() {
	vec3 halfway = normalize(-constants.camerapos - LIGHT_POSITION);
	float specular = pow(max(dot(halfway, norm), 0.), SPECULAR_EXP);

	

	float lambertian=max(dot(LIGHT_POSITION, norm), 0);
	// float lambertian=max(dot(LIGHT_POSITION, vec3(0., 0., 1.)), 0);
	// lambertian = 1;
	vec4 diffuse;
	diffuse = colorRamp(vec4(0, 0, 0, 1), vec4(1, 0, 0, 1), 0.775, 0.825, texture(diffusesampler, uv).r);
	color = pow((diffuse * lambertian + vec4(1.) * specular) / length(LIGHT_POSITION), vec4(1. / 2.2));
	color.w = 1.;
	// diffuse = vec4(0.2, 0.5, 0.9, 1.);
	diffuse = texture(envsampler, reflect(pos - constants.camerapos, norm));
	// lots of wasted ops using a vec4 for color only to directly modify w afterward
	float r0=pow((1.-1.33)/(1.+1.33), 2.);
	float fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.camerapos - pos), norm), 5.);
	fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.camerapos - pos), vec3(0, 1, 0)), 5.);
	color=fresnel*diffuse;
	color.a=1./fresnel;
	color = pow((vec4(1) * lambertian + vec4(1.) * specular) / length(LIGHT_POSITION), vec4(1. / 2.2));
	// color = diffuse;
	color.w = 1.;
	// color = vec4(1, uv.x, uv.y, 1);
	// shading the bounds of the disp map
	// if (distance(pos, vec3(0)) < 5.) color = vec4(0, 0, 0, 1);
	/*
	if (uv.x < constants.dmuvmax.x && uv.y < constants.dmuvmax.y
			&& uv.x > constants.dmuvmin.x && uv.y > constants.dmuvmin.y) {
		color = vec4(norm, 1);
	}
	*/
	color = vec4((pos.y + 2.) / 4., 0, 0, 1);
}
