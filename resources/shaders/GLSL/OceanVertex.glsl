#version 460 

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 uvout;

void main() {
	gl_Position = vec4(position, 1.);
	uvout = uv;
}
