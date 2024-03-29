#pragma once
#include <vector>
#include <string>
#include "vulkan.h"
#include <functional>

namespace Renderer
{
	enum class ShaderType
	{
		Vertex, Fragment, Compute
		// todo -- support for RTX / Mesh
	};

	VkShaderStageFlagBits getFlagBits(ShaderType kind);

	// this allows us to avoid compiling the same shader twice
	enum class ShaderStatus
	{
		Uninitialised, Compiled
	};

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

		bool operator <(const ShaderMember& other) const
		{
			return std::tie(offset, size, vecSize, arraySize, columns, pNext, pMembers) < std::tie(other.offset, other.size, other.vecSize, other.columns, other.arraySize, other.pNext, other.pMembers);
		}
	};

	// A struct obtained via reflection which contains enough information to create descriptors 
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

		bool operator ==(const ShaderResources& other) const
		{
			return std::tie(name, flags, type, access, set, binding, location, inputAttachmentIndex, vecSize, columns, descriptorCount, offset, size, pMembers) == std::tie(other.name, other.flags, other.type, other.access, other.set,
				       other.binding, other.location, other.inputAttachmentIndex, other.vecSize, other.columns, other.descriptorCount, other.offset, other.size, other.pMembers);
		}

		bool operator <(const ShaderResources& other) const
		{
			return std::tie(name, flags, type, access, set, binding, location, inputAttachmentIndex, vecSize, columns, descriptorCount, offset, size, pMembers) < std::tie(other.name, other.flags, other.type, other.access, other.set,
				       other.binding, other.location, other.inputAttachmentIndex, other.vecSize, other.columns, other.descriptorCount, other.offset, other.size, other.pMembers);
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
		uint32_t id;

		bool glInitialised = false;

	public:
		Shader(ShaderType t, std::string path, uint32_t id) : type(t), id(id) { loadFromPath(t, path); }

		bool loadFromPath(ShaderType t, std::string path);
		const char* getText() const { return shaderText.c_str(); }

		ShaderType getType() const { return type; }
		ShaderStatus getStatus() const { return status; }

		// For checking if a shader has already been compiled
		uint32_t getSize() const { return static_cast<uint32_t>(spv.size() * 4); }
		std::vector<ShaderResources> getResources() { return resources; }
		std::vector<uint32_t>& getSPV() { return spv; }
		const uint32_t getId() const { return id; }

		/*
			Transformation Functions, these will be called from the pipeline
		*/
		bool compileGLSL(); // glslang
		bool reflectSPIRV(); // SPIRV-Cross
	};
}

namespace std
{
	template <>
	struct hash<Renderer::ShaderResources>
	{
		size_t operator()(const Renderer::ShaderResources& s) const noexcept
		{
			size_t h1 = hash<uint32_t>{}(s.set);
			size_t h2 = hash<uint32_t>{}(s.binding);
			size_t h3 = hash<uint32_t>{}(s.location);
			size_t h4 = hash<std::underlying_type<VkShaderStageFlagBits>::type>{}(s.flags);
			return (h1 ^ (h2 << 1)) ^ (h3 ^ (h4 << 1));
		}
	};
}
