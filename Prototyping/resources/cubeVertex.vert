#version 450

layout(binding = 0) uniform MVP {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;


out gl_PerVertex 
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout (location = 0) out vec3 color;

void main() {
	color = vec3(1,1,1);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
