#include "Renderer/Vulkan/Core.h"
#include "Renderer/Resources/Shader.h"

using namespace Renderer;

int main() {
	Core renderer;
	renderer.initialiseRenderer();

	//Pipeline pipeline, pipeline2;
	/*renderer.addPipeline(pipeline, {
		new Shader(ShaderType::Vertex, "resources/VertexShader.vert", 1),
		new Shader(ShaderType::Fragment, "resources/FragmentShader.frag", 1) });

	renderer.addPipeline(pipeline2, {
		new Shader(ShaderType::Vertex, "resources/VertexShader.vert", 1),
		new Shader(ShaderType::Fragment, "resources/FragmentShader.frag", 1) });*/
}