#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>

namespace Renderer
{
	Core::Core(int x, int y, const char* name)
	{
		TempLogger::Init();
	}

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		bool success = false;
		swapchain.Initialise(&device.device, &device.instance, &device.physDevice);

		swapchain.BuildWindow(640, 400, "Vulk");

		device.BuildInstance(true);
		swapchain.BuildSurface();
		device.PickPhysicalDevice(swapchain.getSurface());
		device.BuildLogicalDevice(swapchain.getPresentQueue());

		initialiseAllocator();

		swapchain.BuildSwapchain();

		VerboseLog("Building renderpass Cache");
		renderpassCache.buildCache(&device.device);
		RenderpassKey key = RenderpassKey(
			{ { swapchain.getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }
		, {});

		VerboseLog("Adding to renderpass Cache");
		success = renderpassCache.add(key);
		VerboseLog("Building pipeline Cache");

		pipelineCache.buildCache(&device.device);

		VerboseLog("Baking pipeline Key");
		auto gpKey = pipelineCache.bakeKey(
			renderpassCache[key]->getHandle(),
			swapchain.getExtent(),
			DepthSettings::Disabled(),
			{ BlendSettings::Add() },
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			{
				std::make_shared<Shader>(ShaderType::Vertex, "resources/VertexShader.vert"),
				std::make_shared<Shader>(ShaderType::Fragment, "resources/FragmentShader.frag")
			});

		VerboseLog("Inserting pipeline Key");
		auto dump = pipelineCache[gpKey];


		VerboseLog("Creating framebuffer Key");
		auto fKey = FramebufferKey(swapchain.getImageViews(), *renderpassCache[key], swapchain.getExtent());

		VerboseLog("Building framebuffer cache");
		framebufferCache.buildCache(&device.device);
		auto frame = framebufferCache.get(fKey);

		VerboseLog("Building command buffer pool");
		initialiseCommandPool();

		VerboseLog("Building descriptor pool");
		initialiseDescriptorPool(gpKey);

		VerboseLog("Building descriptor sets");
		initialiseDescriptorSets(gpKey);

		maxFramesInFlight = static_cast<uint32_t>(swapchain.getImageViews().size());

		return true;
	}

	void Core::initialiseAllocator()
	{
		VmaAllocatorCreateInfo createInfo = {};
		createInfo.device = device.device;
		createInfo.physicalDevice = device.physDevice;
		createInfo.instance = device.instance;
		createInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;

		auto success = vmaCreateAllocator(&createInfo, &allocator);

		Assert(success == VK_SUCCESS, "Failed to initialise VMA allocator");
	}

	void Core::initialiseCommandPool()
	{
		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = device.indices.graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device.device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool");
	}

	void Core::initialiseDescriptorPool(GraphicsPipelineKey key)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;

		for (const std::shared_ptr<Shader> shader : key.shaders) {
			for (const auto& resource : shader->getResources()) {
				if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) 
					continue;

				VkDescriptorPoolSize poolSize = {};
				poolSize.type = resource.type;
				poolSize.descriptorCount = static_cast<uint32_t>(swapchain.getImageViews().size());

				poolSizes.push_back(poolSize);
			}
		}

		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();
		poolCreateInfo.maxSets = static_cast<uint32_t>(swapchain.getImageViews().size());

		auto success = vkCreateDescriptorPool(device.device, &poolCreateInfo, nullptr, &descriptorPool);
		Assert(success == VK_SUCCESS, "Failed to create descriptor pool");
	}

	void Core::initialiseDescriptorSets(GraphicsPipelineKey key)
	{
		size_t swapSize = swapchain.getImageViews().size();
		std::vector<VkDescriptorSetLayout> layouts(swapSize, key.dLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapSize);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapSize);

		auto success = vkAllocateDescriptorSets(device.device, &allocInfo, descriptorSets.data());
		
		Assert(success == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < swapSize; i++) {
			for (auto& shader : key.shaders)
			{
				for (auto res : shader->getResources())
				{
					VkDescriptorBufferInfo descBufferInfo = {};
					descBufferInfo.buffer = vertexBuffer->buffer;
					descBufferInfo.offset = res.offset;
					descBufferInfo.range = res.size;

					VkWriteDescriptorSet writeDescSet = {};
					writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescSet.dstSet = descriptorSets[i];
					writeDescSet.dstBinding = res.binding;
					writeDescSet.dstArrayElement = 0;
					writeDescSet.descriptorType = res.type;
					writeDescSet.descriptorCount = res.descriptorCount;
					writeDescSet.pBufferInfo = &descBufferInfo;

					vkUpdateDescriptorSets(device.device, 1, &writeDescSet, 0, nullptr);
				}
			}
		}
	}

	Core::~Core()
	{
		vkDestroyDescriptorPool(device.device, descriptorPool, nullptr);
		vkDestroyCommandPool(device.device, commandPool, nullptr);
		framebufferCache.clearCache();
		pipelineCache.clearCache();
		renderpassCache.clearCache();
		vmaDestroyAllocator(allocator);
	}
	
}