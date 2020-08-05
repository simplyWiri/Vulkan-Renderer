#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include <string>
#include "../../Resources/Buffer.h"
#include "../../Resources/Shader.h"
#include "../Wrappers/Pipeline.h"
#include "../Wrappers/Swapchain.h"
#include "Tether.h"
#include "GraphContext.h"
#include "../Caches/FramebufferCache.h"
#include "../Caches/PipelineCache.h"
#include "../Caches/RenderpassCache.h"
#include "../Wrappers/Renderpass.h"
#include "PassDesc.h"
#include "../Caches/DescriptorSetCache.h"


namespace Renderer
{
	/* Naming:
	 *
	 * Rendergraph:
	 * Rendergraph -> an object which stores a collection of renderpasses, on initialisation orders them, and during runtime, executes the pass
	 * PassDesc -> a description of a 'renderpass' which occurs inside the rendergraph, it holds the initialisation and execution function for the pass
	 *
	 * Setup:
	 * Tether -> a struct which holds information about the resources a 'PassDesc' reads and writes to
	 * GraphContext -> a struct which holds pointers to the resources stored inside a renderpass, it is passed to the execution function in the PassDesc
	 * FrameInfo -> a struct which holds information about the current frame, index, delta, time, etc
	 *
	 * Resources:
	 * Resource -> a struct which is inherited from for specific resources, contains an id
	 * ImageResource -> a struct containing information about an 'image'
	 * BufferResource -> a struct containing information about a 'buffer'
	 */

	 // How is the image attachment sized relative to swapchain?
	enum SizeClass { Absolute, SwapchainRelative, Input };

	class Core;
	class Rendergraph;

	// Image Attachment
	struct Resource
	{
		uint16_t id;
	};

	struct ImageResource : Resource
	{
		SizeClass sizeClass = SwapchainRelative;
		glm::vec3 size = { 1.0f, 1.0f, 0.0f };
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t samples = 1, levels = 1, layers = 1;
		VkImageUsageFlags usage = 0;
	};

	// Buffer Attachment
	struct BufferResource : Resource
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
		VkMemoryPropertyFlags props;
		VmaMemoryUsage memUsage;
	};

	class Rendergraph
	{
	private:
		bool initialised = false;
		uint16_t curIndex = 1;
		ShaderManager shaderManager;

		std::unordered_map<std::string, uint16_t> strToIndex;

		std::vector<PassDesc> passes;
		std::vector<PassDesc> uniquePasses;


		Core* core;

		RenderpassCache renderCache;
		GraphicsPipelineCache graphicsPipelineCache;
		FramebufferCache framebufferCache;
		DescriptorSetCache descriptorSetCache;

		VkCommandPool pool;
		Image* depthImage;
		std::vector<VkCommandBuffer> buffers;


	public:
		Rendergraph(Core* core);

		void Initialise();
		void Execute();
		void Rebuild();

		void AddPass(PassDesc passDesc);

		GraphicsPipelineCache& getGraphicsPipelineCache() { return graphicsPipelineCache; }
		RenderpassCache& getRenderpassCache() { return renderCache; }
		FramebufferCache& getFramebufferCache() { return framebufferCache; }
		ShaderManager& getShaderManger() { return shaderManager; }
		DescriptorSetCache& getDescriptorSetCache() { return descriptorSetCache; }


	private:

		// Baking utility (initialisation)
		void extractGraphInformation();
		// assigns id's, locates resources required, builds a dependancy graph
		// builds the required resources
		void validateGraph(); // debug

		void mergePasses(); // merges passes which are compatible
		void buildTransients(); // 'merge' or 'reuse' images which are compatible
		void buildBarriers(); // build synchronisation barriers between items

	private:
		uint16_t assignId() { return ++curIndex; }
		void processBuffer(const std::string& resName, VkBufferUsageFlags usage);
		void processImage(std::string resName, VkImageUsageFlags usage);




	};


}
