#version 460

layout(push_constant) uniform Constants {
	vec2 position, extent;
	vec4 bgcolor;
} constants;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 pos;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 color;

void main() {
	// color = vec4(1, uv, 0.1);
	color = mix(constants.bgcolor, vec4(1, 1, 1, 1), texture(tex, uv).r);
}
