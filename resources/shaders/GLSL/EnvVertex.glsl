#version 460

layout(push_constant) uniform Constants {
	mat4 cameravp;
} constants;

layout(location = 0) out vec3 pos;

vec3 v[8] = {
	vec3(1, 1, 1),
	vec3(-1, 1, 1),
	vec3(1, -1, 1),
	vec3(-1, -1, 1),
	vec3(1, 1, -1),
	vec3(-1, 1, -1),
	vec3(1, -1, -1),
	vec3(-1, -1, -1)
};

uint i[36] = {
	1, 0, 2,
	1, 2, 3,
	4, 5, 6,
	6, 5, 7,
	0, 1, 4,
	5, 4, 1,
	3, 2, 7,
	6, 7, 2,
	2, 0, 4,
	2, 4, 6,
	1, 3, 5,
	5, 3, 7
};

void main() {
	pos = v[i[gl_VertexIndex]];
	gl_Position = constants.cameravp * vec4(pos, 1);
}
