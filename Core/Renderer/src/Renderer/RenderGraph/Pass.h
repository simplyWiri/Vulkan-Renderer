#pragma once
#include "GraphContext.h"
#include "Resource.h"
#include "../Memory/Image.h"
#include "../VulkanObjects/Renderpass.h"
#include "../VulkanObjects/Swapchain.h"

namespace Renderer::RenderGraph
{
	class RenderGraph;
	
	struct Pass
	{
		friend class RenderGraph;

		// a list of image view(s), one per frame in flight.
		std::vector<std::vector<VkImageView>> views;

		std::string name;
		RenderpassKey key;
		VkExtent2D renderExtent;

		std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> execute;

		RenderpassKey GetRenderpassKey() const { return key; }
		std::vector<VkImageView> GetViews(int index) const { return views[index]; }
		VkExtent2D GetExtent() const { return renderExtent; }
		void Execute(VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context) { execute(buffer, frameInfo, context); }
	};
}
