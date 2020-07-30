#pragma once
#include <vector>
#include <string>
#include "vulkan.h"

namespace Renderer
{
	enum class ShaderType : unsigned
	{
		Vertex,
		Fragment,
		Compute,
		Mesh
		// todo -- support for RTX
	};

	VkShaderStageFlagBits getFlagBits(ShaderType kind);

	// this allows us to avoid compiling the same shader twice
	enum class ShaderStatus
	{
		Uninitialised,
		Compiled
	};

	/*
		Shader is a class which contains a GLSL shader; It should be cleaned up after the compiled SPIR-V is linked into a pipeline
			- Contains functions for getting information about a shader
			- It contains all the information that will be required for reflection to create vulkan objects
	*/

	// If a UBO/SSBO/Push Constant contains a struct, this member will be filled with the details of the contents within the struct
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

	// A generic reflection struct which contains enough information to create descriptors per relevant resource, for parsing into descriptors
	struct ShaderResources
	{
		std::string name;
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
		uint32_t offset = 0;
		uint32_t size;
		const ShaderMember* pMembers;

		bool operator <(const ShaderResources& other) const
		{
			return
				std::tie(name, flags, type, access, set, binding, location, inputAttachmentIndex, vecSize, columns, descriptorCount, offset, size)
				<
				std::tie(other.name, other.flags, other.type, other.access, other.set, other.binding, other.location, other.inputAttachmentIndex, other.vecSize, other.columns, other.descriptorCount, other.offset, other.size);
		}
	};

	/*
		Shader Struct
			- Contains the GLSL equivalent of the shader code
			- (After compilation) it contains the SPIRV equivalent of the shader code
			- This is accessible to the application layer, however, it does not expose the underlying API's used to reflect/compile the shaders.

		It will be used as a class to pass into a pipeline, which then does the internal compilation and reflection, finally (internally) parsing the ShaderResources into the relevant descriptors
	*/

	class Shader
	{
		ShaderType type;
		ShaderStatus status = ShaderStatus::Uninitialised;
		std::vector<ShaderResources> resources;

		std::string shaderText;
		std::vector<uint32_t> spv;

		bool glInitialised = false;

	public:
		Shader(ShaderType t, std::string path = "")
			: type(t) {
			loadFromPath(t, path);
		}

		bool loadFromPath(ShaderType t, std::string path);
		inline const char* getText() const { return shaderText.c_str(); }

		inline ShaderType getType() const { return type; }
		inline ShaderStatus getStatus() const { return status; } // For checking if a shader has already been compiled
		inline uint32_t getSize() const { return static_cast<uint32_t>(spv.size() * 4); }
		inline std::vector<ShaderResources> getResources() { return resources; }
		inline std::vector<uint32_t>& getSPV() { return spv; }

		/*
			Transformation Functions, these will be called from the pipeline
		*/
		bool compileGLSL(); // glslang
		bool reflectSPIRV(); // SPIRV-Cross
	};
}
