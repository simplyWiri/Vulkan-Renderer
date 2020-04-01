#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	{
		outColor = inColor;
	}
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 0);
}