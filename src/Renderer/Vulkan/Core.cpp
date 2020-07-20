#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace Renderer
{
	Core::Core(int x, int y, const char* name) { TempLogger::Init(); }

	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		bool success = false;
		swapchain.Initialise(device.getDevice(), device.getInstance(), device.getPhysicalDevice());

		swapchain.BuildWindow(640, 400, "Vulk");

		device.BuildInstance(true);
		swapchain.BuildSurface();
		device.PickPhysicalDevice(swapchain.getSurface());
		device.BuildLogicalDevice(swapchain.getPresentQueue());
		device.BuildAllocator();

		swapchain.BuildSwapchain();

		VerboseLog("Building renderpass Cache");
		renderpassCache.buildCache(device.getDevice());
		RenderpassKey key = RenderpassKey(
			{ {swapchain.getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR} }
		, {});

		VerboseLog("Adding to renderpass Cache");
		success = renderpassCache.add(key);
		VerboseLog("Building pipeline Cache");

		pipelineCache.buildCache(device.getDevice());

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
		auto* dump = pipelineCache[gpKey];

		VerboseLog("Creating framebuffer Key");
		auto fKey = FramebufferKey(swapchain.getImageViews(), *renderpassCache[key], swapchain.getExtent());

		VerboseLog("Building framebuffer cache");
		framebufferCache.buildCache(device.getDevice());
		auto* frame = framebufferCache.get(fKey);

		VerboseLog("Building command buffer pool");
		buffers.resize(swapchain.getImageViews().size());
		initialiseCommandPool();

		VerboseLog("Building descriptor pool");
		initialiseDescriptorPool(gpKey);

		switch (settings.buffering)
		{
		case RendererBufferSettings::SwapchainSync: bufferCopies = maxFramesInFlight;
		case RendererBufferSettings::SingleBuffered: bufferCopies = 1;
		case RendererBufferSettings::DoubleBuffered: bufferCopies = 2;
		case RendererBufferSettings::TripleBuffered: bufferCopies = 3;
		}

		vertexBuffer = new Buffer(device.getAllocator(), VkDeviceSize(static_cast<uint64_t>(64 * 64 * bufferCopies)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		indexBuffer = new Buffer(device.getAllocator(), VkDeviceSize(static_cast<uint64_t>(64 * 64 * bufferCopies)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		ubo = new Buffer(device.getAllocator(), VkDeviceSize(static_cast<uint64_t>(192 * bufferCopies)), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);



		auto dst = vertexBuffer->map();
		memcpy(dst, vertices.data(), sizeof(Vertex) * vertices.size());
		vertexBuffer->unMap();

		dst = indexBuffer->map();
		memcpy(dst, indices.data(), sizeof(uint16_t) * vertices.size());
		indexBuffer->unMap();

		VerboseLog("Building descriptor sets");
		initialiseDescriptorSets(gpKey);


		for (size_t i = 0; i < buffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(buffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderpassCache[key]->getHandle();
			renderPassInfo.framebuffer = framebufferCache[fKey]->getHandle()[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapchain.getExtent();

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineCache[gpKey]->getPipeline());

			VkBuffer vertexBuffers[] = { vertexBuffer->buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(buffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(buffers[i], indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);

			uint32_t offset = 0;
			vkCmdBindDescriptorSets(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gpKey.pLayout, 0, 1, &descriptorSets[i], 1, &offset);

			vkCmdDrawIndexed(buffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(buffers[i]);

			if (vkEndCommandBuffer(buffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}

		swapchain.InitialiseSyncObjects();

		return true;
	}

	bool Core::Run()
	{
		if (glfwWindowShouldClose(swapchain.getWindow())) return false;

		glfwPollEvents();

		auto frameInfo = swapchain.BeginFrame(buffers[swapchain.getIndex()]);

		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapchain.getExtent().width / (float)swapchain.getExtent().height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		auto dist = this->ubo->map();
		memcpy(dist, &ubo, sizeof(ubo));
		this->ubo->unMap();


		auto result = swapchain.EndFrame(frameInfo, device.queues.graphics);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			windowResize();

		return true;
	}

	void Core::windowResize()
	{

	}

	void Core::initialiseCommandPool()
	{
		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = device.getIndices()->graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto success = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
		Assert(success == VK_SUCCESS, "Failed to create command pool");

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = commandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		success = vkAllocateCommandBuffers(device, &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");
	}

	void Core::initialiseDescriptorPool(GraphicsPipelineKey key)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;

		for (const auto shader : key.shaders)
		{
			for (const auto& resource : shader->getResources())
			{
				if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) continue;

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

		auto success = vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool);
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

		auto success = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());

		Assert(success == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < swapSize; i++)
		{
			for (auto& shader : key.shaders)
			{
				for (auto res : shader->getResources())
				{
					VkDescriptorBufferInfo descBufferInfo = {};
					descBufferInfo.buffer = ubo->buffer;
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

					vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
				}
			}
		}
	}

	Core::~Core()
	{
		vertexBuffer->~Buffer();
		indexBuffer->~Buffer();
		ubo->~Buffer();

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr);
		framebufferCache.clearCache();
		pipelineCache.clearCache();
		renderpassCache.clearCache();
	}
}
