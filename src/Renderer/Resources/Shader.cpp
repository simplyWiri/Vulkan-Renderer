#include "Shader.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/DirStackFileIncluder.h"
#include "spirv_glsl.hpp"
#include "spirv_cross.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include "vulkan.h"

std::vector<char> loadFromFile(const char* path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	
	return buffer;
}

static const EShLanguage getEshLangType(ShaderType kind) {
	switch (kind) {
	case ShaderType::Vertex:
		return EShLangVertex;
	case ShaderType::Compute:
		return EShLangCompute;
	case ShaderType::Fragment:
		return EShLangFragment;
	case ShaderType::Mesh:
		return EShLangMeshNV;
	case ShaderType::Tesselation:
		return EShLangTessControl;
	}
	throw std::runtime_error("Unsupported language type");
	return EShLangCallable;
}
static const TBuiltInResource GetDefaultResources() {
	TBuiltInResource resources = {};
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.limits.nonInductiveForLoops = true;
	resources.limits.whileLoops = true;
	resources.limits.doWhileLoops = true;
	resources.limits.generalUniformIndexing = true;
	resources.limits.generalAttributeMatrixVectorIndexing = true;
	resources.limits.generalVaryingIndexing = true;
	resources.limits.generalSamplerIndexing = true;
	resources.limits.generalVariableIndexing = true;
	resources.limits.generalConstantMatrixVectorIndexing = true;
	return resources;
}


ShaderMember* getMembers(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type)
{
	ShaderMember* pFirstMemberInfo = nullptr;
	ShaderMember* pPrevMemberInfo = nullptr;

	for (auto i = 0U; i < type.member_types.size(); ++i)
	{
		const auto& memberType = compiler.get_type(type.member_types[i]);

		auto mi = new ShaderMember;
		mi->offset = compiler.type_struct_member_offset(type, i);
		mi->size = static_cast<uint32_t>(compiler.get_declared_struct_member_size(type, i));
		mi->vecSize = memberType.vecsize;
		mi->columns = memberType.columns;
		mi->arraySize = (memberType.array.size() == 0) ? 1 : memberType.array[0];
		mi->pNext = nullptr;
		mi->pMembers = nullptr;


		// Link current and last member infos.
		if (pPrevMemberInfo == nullptr)
			pPrevMemberInfo = mi;
		else
			pPrevMemberInfo->pNext = mi;

		// Keep pointer to first member info.
		if (pFirstMemberInfo == nullptr)
			pFirstMemberInfo = mi;

		// Update previous member.
		pPrevMemberInfo = mi;

		// Recursively process members that are structs.
		if (memberType.basetype == spirv_cross::SPIRType::Struct)
			mi->pMembers = getMembers(compiler, memberType);
	}

	// Return the first member info created.
	return pFirstMemberInfo;
}

bool Shader::loadFromPath(ShaderType t, const char* path)
{
	auto charVec = loadFromFile(path);
	this->shaderText = *new std::string(charVec.begin(), charVec.end());

	return true;
}

bool Shader::compileGLSL(glslang::TProgram& program)
{
	EShLanguage lang = getEshLangType(this->type);
	glslang::TShader shader(lang);
	auto resources = GetDefaultResources();

	// In essence, verbose debugging
	auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault | EShMsgDebugInfo);

	const char* glsl = this->shaderText.c_str();
	shader.setStrings(&glsl, 1);

	//	Targeting Vulkan 1.2 & SPIRV-1.3	
	auto vulkanVersion = glslang::EShTargetVulkan_1_2;
	shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 110);
	shader.setEnvClient(glslang::EShClientVulkan, vulkanVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

	DirStackFileIncluder includer;
	std::string str;

	// preprocess the shader
	if (!shader.preprocess(&resources, vulkanVersion, ENoProfile, false, false, messages, &str, includer)) {
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
		throw std::runtime_error("Failed to pre-process shader");
	}
	// parse the shader into the intermediate spv
	if (!shader.parse(&resources, vulkanVersion, true, messages, includer)) {
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
		throw std::runtime_error("Failed to parse shader");
	}
	// link shader to a program
	program.addShader(&shader);

	if (!program.link(messages) || !program.mapIO()) {
		std::cout << shader.getInfoLog() << std::endl;
		std::cout << shader.getInfoDebugLog() << std::endl;
		throw std::runtime_error("Failed to link shader");
	}
	// convert the intermediate spv into the binary
	spv::SpvBuildLogger logger;

	glslang::SpvOptions spvOptions;
	spvOptions.generateDebugInfo = true;
	spvOptions.disableOptimizer = false;
	spvOptions.optimizeSize = true;

	glslang::GlslangToSpv(*program.getIntermediate(lang), this->spv, &logger, &spvOptions);


	program.buildReflection();


	return false;
}

bool Shader::reflectSPIRV(VkShaderStageFlagBits stage, std::vector<ShaderResources>& resources)
{
	spirv_cross::CompilerGLSL compiler(std::move(this->spv));

	auto shaderRes = compiler.get_shader_resources();
	

	/*
		Finish Reflection 
	*/

	for (auto& res : shaderRes.uniform_buffers)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource; // resources for a uniform buffer

		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding); // shader binding
		resource.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // This can be either UNIFORM_BUFFER or UNIFORM_BUFFER_DYNAMIC
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0]; // For the potential of arrayed inputs
		resource.flags = stage; // this is dependent on the type of shader that is being reflected
		resource.access = VK_ACCESS_UNIFORM_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resource.pMembers = getMembers(compiler, spirType);
		resources.push_back(resource);
	}

	for (auto& res : shaderRes.storage_buffers)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource; 

		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding); 
		resource.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; // This can be either STORAGE_BUFFER or STORAGE_BUFFER_DYNAMIC
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0]; 
		resource.flags = stage; 
		resource.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		auto nonReadable = compiler.get_decoration(res.id, spv::DecorationNonReadable);
		auto nonWriteable = compiler.get_decoration(res.id, spv::DecorationNonWritable);
		if (nonReadable) resource.access = VK_ACCESS_SHADER_WRITE_BIT;
		else if (nonWriteable) resource.access = VK_ACCESS_SHADER_READ_BIT;

		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resource.pMembers = getMembers(compiler, spirType);
		resources.push_back(resource);

		
	}

	for (auto& res : shaderRes.separate_samplers)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource; 
		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding); 
		resource.type = VK_DESCRIPTOR_TYPE_SAMPLER; 
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0]; 
		resource.flags = stage; 
		resource.access = VK_ACCESS_SHADER_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resources.push_back(resource);
	}

	for (auto& res : shaderRes.sampled_images)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource;
		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		resource.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0];
		resource.flags = stage;
		resource.access = VK_ACCESS_SHADER_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resources.push_back(resource);
	}

	for (auto& res : shaderRes.separate_images)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource;
		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		resource.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0];
		resource.flags = stage;
		resource.access = VK_ACCESS_SHADER_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resources.push_back(resource);
	}

	for (auto& res : shaderRes.storage_images)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource;

		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		resource.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; 
		resource.descriptorCount = (spirType.array.size() == 0) ? 1 : spirType.array[0];
		resource.flags = stage;
		resource.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		auto nonReadable = compiler.get_decoration(res.id, spv::DecorationNonReadable);
		auto nonWriteable = compiler.get_decoration(res.id, spv::DecorationNonWritable);
		if (nonReadable) resource.access = VK_ACCESS_SHADER_WRITE_BIT;
		else if (nonWriteable) resource.access = VK_ACCESS_SHADER_READ_BIT;

		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resource.pMembers = getMembers(compiler, spirType);
		resources.push_back(resource);
	}

	// Extract subpass inputs.
	for (auto& res : shaderRes.subpass_inputs)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		ShaderResources resource;
		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		resource.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		resource.descriptorCount = 1; 
		resource.flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		resource.access = VK_ACCESS_SHADER_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resource.inputAttachmentIndex = compiler.get_decoration(res.id, spv::DecorationInputAttachmentIndex);

		resources.push_back(resource);
	}

	// Extract push constants.
	for (auto& res : shaderRes.push_constant_buffers)
	{
		const auto& spirType = compiler.get_type_from_variable(res.id);

		uint32_t offset = ~0;
		for (auto i = 0U; i < spirType.member_types.size(); ++i)
		{
			auto memberType = compiler.get_type(spirType.member_types[i]);
			offset = std::min(offset, compiler.get_member_decoration(spirType.self, i, spv::DecorationOffset));
		}
		
		ShaderResources resource;
		resource.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
		resource.offset = offset;
		resource.type = VK_DESCRIPTOR_TYPE_MAX_ENUM; // (we need to handle push constants differently to other descriptor type layouts
		resource.flags = stage;
		resource.access = VK_ACCESS_SHADER_READ_BIT;
		resource.set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		resource.size = static_cast<uint32_t>(compiler.get_declared_struct_size(spirType));
		resource.pMembers = getMembers(compiler, spirType);

		resources.push_back(resource);
	}

	return false;
}
