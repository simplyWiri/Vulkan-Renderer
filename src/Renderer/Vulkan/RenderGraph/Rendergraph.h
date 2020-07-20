#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include "../../Resources/Buffer.h"
#include "../../Resources/Shader.h"
#include "../Wrappers/Pipeline.h"

namespace Renderer
{
	// How is the image attachment sized relative to swapchain?
	enum SizeClass { Absolute, SwapchainRelative, Input };

	class Core;

	// Image Attachment
	struct ImageAttachmentInfo
	{
		SizeClass sizeClass = SizeClass::SwapchainRelative;
		glm::vec3 size = {1.0f, 1.0f, 0.0f};
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t samples = 1, levels = 1, layers = 1;
		VkImageUsageFlags usage = 0;
	};

	// Buffer Attachment
	struct BufferAttachmentInfo
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
	};

	struct GraphContext
	{
		std::unordered_map<std::string, Buffer> VertexBuffers;
		std::unordered_map<std::string, Buffer> IndexBuffers;

		Buffer& VertexBuffer(const std::string& key) { return VertexBuffers.at(key); }
		Buffer& IndexBuffer(const std::string& key) { return IndexBuffers.at(key); }
	};

	struct FrameInfo;

	struct Tether
	{
		void RegisterVertexBuffer(const char* id);
		void RegisterIndexBuffer(const char* id);

		void RegisterShader(ShaderType type, const char* path);

		void RegisterPipeline(VkRenderPass renderpass, VkExtent2D extent, DepthSettings depthSettings,
		                      const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology,
		                      std::vector<std::shared_ptr<Shader>> shaders);
	};

	struct PassDesc
	{
		PassDesc() { }

		PassDesc& SetName(const char* name)
		{
			this->taskName = name;
			return *this;
		}

		PassDesc& SetInitialisationFunc(std::function<void(Tether&)> func)
		{
			this->initialisation = func;
			return *this;
		}

		PassDesc& SetRecordFunc(std::function<void(FrameInfo&, GraphContext&)> func)
		{
			this->execute = func;
			return *this;
		}

		std::string taskName;

		VkExtent2D renderExtent;

		std::function<void(Tether&)> initialisation;
		std::function<void(FrameInfo&, GraphContext&)> execute;
	};

	class Rendergraph
	{
	public:

	private:
		bool initialised = false;

	public:
		Rendergraph(Core* core);

		void Initialise();
		void Execute();

		void AddPass(PassDesc passDesc);

	private:

		// Baking utility (initialisation)
		void validateGraph(); // debug
		void extractGraphInformation(); // assigns id's, locates resources required, builds a dependancy graph

		void buildResources(); // builds resources associated by id
		void mergePasses(); // merges passes which are compatible
		void buildTrasients(); // 'merge' or 'reuse' images which are compatible
		void buildBarriers(); // build synchronisation barriers between items
	};
}
