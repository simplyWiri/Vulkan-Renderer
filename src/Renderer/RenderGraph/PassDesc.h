#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vulkan.h>

namespace Renderer
{
	struct Tether;
	struct RenderGraphBuilder;
	struct FrameInfo;
	struct GraphContext;
	enum class ResourceType;

	struct AccessedResource
	{
		VkPipelineStageFlags stages = 0;
		VkAccessFlags access = 0;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		ResourceType type = static_cast<ResourceType>(0);
		std::string name = "";
	};

	class PassDesc
	{
	private:
		uint32_t id;
		std::string name;

		std::vector<std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext&)>> executes;

		std::vector<AccessedResource> readResources;
		std::vector<AccessedResource> writtenResources;

	public:
		PassDesc(RenderGraphBuilder& builder, Tether& tether, uint32_t id);

		std::string GetName() const { return name; }
		uint32_t GetId() const { return id; }

		std::vector<AccessedResource> GetReadResources() const { return readResources; }
		std::vector<AccessedResource> GetWrittenResources() const { return writtenResources; }

		void Execute(VkCommandBuffer& buffer, const FrameInfo& frameInfo, GraphContext& context) const
		{
			for(const auto& exec : executes)
				exec(buffer, frameInfo, context);
		}		
	};
}
