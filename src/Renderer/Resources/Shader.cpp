#include "Shader.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/DirStackFileIncluder.h"
#include "spirv_glsl.hpp"
#include "spirv_cross.hpp"
#include <iostream>
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

bool Shader::reflectSPIRV(VkShaderStageFlagBits stage)
{
	spirv_cross::CompilerGLSL compiler(std::move(this->spv));

	auto shaderRes = compiler.get_shader_resources();

	/*
		Finish Reflection 
	*/

	for (auto& resource : shaderRes.uniform_buffers)
	{
		for (auto& range : compiler.get_active_buffer_ranges(resource.id)) {
			printf("Accessing member # %u, offset %u, size %u\n", range.index, range.offset, range.range);
		}
		unsigned set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		unsigned binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		printf("Uniform buffer: Named: %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);
	}
	

	return false;
}
