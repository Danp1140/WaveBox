#version 460

#define LIGHT_POSITION vec3(0.5, 1., 0.5)
#define SPECULAR_EXP 20.

layout(set = 0, binding = 0) uniform sampler2D heightsampler;
layout(set = 0, binding = 1) uniform sampler2D diffusesampler;
layout(set = 0, binding = 2) uniform samplerCube envsampler;


layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec3 camerapos;
	uint flags;
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
	if ((constants.flags & 0x00000004) != 0) { // if there is no diffuse tex
		diffuse = vec4(1.);
	}
	else {
	// vec4 diffuse = mix(vec4(1., 0., 0., 1.), vec4(0., 0., 1., 1.), texture(heightsampler, uv).r / 5.0);
		diffuse = texture(diffusesampler, uv);
	// vec4 diffuse = vec4(uv, 0, 1);
	// vec4 diffuse = vec4(1.);
	}
	diffuse = colorRamp(vec4(0, 0, 0, 1), vec4(1, 0, 0, 1), 0.775, 0.825, texture(diffusesampler, uv).r);
	color = pow((diffuse * lambertian + vec4(1.) * specular) / length(LIGHT_POSITION), vec4(1. / 2.2));
	color.w = 1.;
	// diffuse = vec4(0.2, 0.5, 0.9, 1.);
	diffuse = texture(envsampler, reflect(pos - constants.camerapos, norm));
	if ((constants.flags & 0x00000003) != 0) { // if either reflection flag is set
		// lots of wasted ops using a vec4 for color only to directly modify w afterward
		float r0=pow((1.-1.33)/(1.+1.33), 2.);
		float fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.camerapos - pos), norm), 5.);
		fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.camerapos - pos), vec3(0, 1, 0)), 5.);
		color=fresnel*diffuse;
		color.a=1./fresnel;
	}
	//color = pow((vec4(1) * lambertian + vec4(1.) * specular) / length(LIGHT_POSITION), vec4(1. / 2.2));
	//color = diffuse;
	// color.w = 1.;
}
