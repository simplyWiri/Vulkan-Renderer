#include "Renderer/Vulkan/Renderer.h"
#include "Renderer/Resources/Shader.h"
#include "glslang/Public/ShaderLang.h"
#include "vulkan.h"



int main() {
	//Renderer renderer;
	//renderer.initialiseRenderer();
	glslang::InitializeProcess();
	glslang::TProgram program;


	Shader shader(ShaderType::Vertex, "");
	shader.loadFromPath(ShaderType::Vertex, "resources/VertexShader.vert");

	shader.compileGLSL(program);

	shader.reflectSPIRV(VK_SHADER_STAGE_VERTEX_BIT);
}