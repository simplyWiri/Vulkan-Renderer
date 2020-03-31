#pragma once
#include <vector>
#include <string>

enum class ShaderType {
	Vertex, Fragment, Compute, Mesh, Tesselation
	// todo -- support for RTX
};

// this allows us to hypothetically re-use shaders, that have already been compiled,
enum class ShaderStatus {
	Uninitialised, Compiled, Reflected
};

/*
	Shader is a class which contains a GLSL shader; It should be cleaned up after the compiled SPIR-V is linked into a pipeline
		- Contains functions for getting information about a shader
		- It contains all the information that will be required for reflection to create vulkan objects
		

	Pipeline.addShader(new Shader(ShaderType::Vertex, glslChunk));
*/

// forward declare the relevant structs which are passed as func parameters

namespace glslang {
	class TProgram;
}

enum VkShaderStageFlagBits;
enum VkDescriptorType;
typedef uint32_t VkFlags;
typedef VkFlags VkAccessFlags;

struct ShaderMember
{
	uint32_t offset;
	uint32_t size;
	uint32_t vecSize;
	uint32_t columns;
	uint32_t arraySize;
	const ShaderMember* pNext;
	const ShaderMember* pMembers;
};

struct ShaderResources 
{
	VkShaderStageFlagBits flags;
	VkDescriptorType type;
	VkAccessFlags access;
	uint32_t set;
	uint32_t binding;
	uint32_t location;
	uint32_t inputAttachmentIndex;
	uint32_t vecSize;
	uint32_t columns;
	uint32_t descriptorCount;
	uint32_t offset;
	uint32_t size;
	const ShaderMember* pMembers;
};

class Shader
{
public:
	Shader(ShaderType t, const char* text)
		: type(t), shaderText(text) { }
	
	bool loadFromPath(ShaderType t, const char* path);
	inline const char* getText() const { return shaderText.c_str(); }

	inline ShaderStatus getStatus() const { return status; }
	inline ShaderStatus setStatus(ShaderStatus s) { status = s; }

	inline uint32_t getSize() const { return static_cast<uint32_t>(spv.size()); }
	
	inline std::vector<uint32_t> getSPV() const { return spv; }

	/*
		Transformation Functions, these will be called from the vulkan pipeline
	*/
	bool compileGLSL(glslang::TProgram& program); // glslang
	bool reflectSPIRV(VkShaderStageFlagBits stage, std::vector<ShaderResources>& resources); // SPIRV-Cross

private:
	ShaderType type;
	ShaderStatus status = ShaderStatus::Uninitialised;

	std::string shaderText;
	std::vector<uint32_t> spv;
	uint32_t spvSize;

	void* resources; // ShaderResources, so you can potentially reuse shaders over multiple pipelines
};
