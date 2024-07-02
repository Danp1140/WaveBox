#version 460

layout(location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform samplerCube env;

layout(location = 0) out vec4 color;

void main() {
	color = vec4(0.4, 0.2, 0.8, 1.);
	color = texture(env, pos);
}
