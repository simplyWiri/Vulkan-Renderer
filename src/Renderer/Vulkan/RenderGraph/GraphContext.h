#pragma once
#include "vulkan.h"
#include <string>
#include <unordered_map>

#include "../Wrappers/Renderpass.h"

namespace Renderer
{
	class Rendergraph;

	struct GraphContext
	{
		Rendergraph* graph;
		std::string passId;
		VkExtent2D extent;
		Renderpass* renderpass;

		VkExtent2D getExtent() { return extent; }
		Renderpass getDefaultRenderpass() { return *renderpass; }
	};
}

