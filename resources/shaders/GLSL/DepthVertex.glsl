#version 460

layout(push_constant) uniform Constants {
	mat4 cameravp;
} constants;

layout(location = 0) in vec3 position;

void main() {
	gl_Position = constants.cameravp * vec4(position, 1.);
}
