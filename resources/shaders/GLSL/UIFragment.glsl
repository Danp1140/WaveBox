#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 pos;

layout(location = 0) out vec4 color;

void main() {
	color = vec4(1, uv, 1);
}
