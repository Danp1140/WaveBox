#version 460 

layout(push_constant) uniform Constants {
	mat4 cameravp;
	vec3 camerapos;
	uint flags;
} constants;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 norm;

layout(location = 0) out vec2 uvout;
layout(location = 1) out vec3 normout;

void main() {
	//gl_Position = constants.cameravp * vec4(position, 1.); // doing this multiplication here results in more
							       // total multiplications, but allows us to do
							       // scree-space-ish math in tesscontrol
	gl_Position = vec4(position, 1.);
	uvout = uv;
	normout = norm;
}
