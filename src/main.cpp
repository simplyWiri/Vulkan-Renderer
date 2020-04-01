#include "Renderer/Vulkan/Renderer.h"
#include "Renderer/Resources/Shader.h"
#include "glslang/Public/ShaderLang.h"
#include "vulkan.h"

int main() {
	Renderer renderer;
	renderer.initialiseRenderer();

	Pipeline pipeline, pipeline2;
	renderer.addPipeline(pipeline, {
		new Shader(ShaderType::Vertex, "resources/VertexShader.vert", 1),
		new Shader(ShaderType::Fragment, "resources/FragmentShader.frag", 1) });

	renderer.addPipeline(pipeline2, {
		new Shader(ShaderType::Vertex, "resources/VertexShader.vert", 1),
		new Shader(ShaderType::Fragment, "resources/FragmentShader.frag", 1) });

	renderer.buildPipelines();
}