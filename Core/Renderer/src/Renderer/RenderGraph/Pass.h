#pragma once
#include "GraphContext.h"
#include "Renderer/Memory/Image.h"
#include "Renderer/VulkanObjects/Renderpass.h"
#include "Renderer/VulkanObjects/Swapchain.h"

namespace Renderer::RenderGraph
{
	class GraphBuilder;
	
	struct Pass
	{
		friend class GraphBuilder;

		// a list of image view(s), one per frame in flight.
		std::vector<std::vector<VkImageView>> views;

		std::string name;
		RenderpassKey key;
		VkExtent2D renderExtent;
		std::vector<std::vector<VkImageMemoryBarrier>> imageBarriers; 

		std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> execute;

		RenderpassKey GetRenderpassKey() const { return key; }
		std::vector<VkImageView> GetViews(int index) const { return views[index]; }
		VkExtent2D GetExtent() const { return renderExtent; }
		void Execute(VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context) { execute(buffer, frameInfo, context); }
	};
}
