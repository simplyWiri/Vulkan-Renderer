#pragma once
#include <algorithm>

#include "glm/glm.hpp"
#include "vulkan.h"
#include <vector>
#include <unordered_map>
#include <string>
#include "RenderGraphBuilder.h"
#include "PassDesc.h"
#include "Resource.h"


namespace Renderer
{
	class Core;

	namespace Memory
	{
		class Image;
	}

	/* Naming:
	 *
	 * RenderGraph:
	 * RenderGraph -> an object which stores a collection of renderpasses, on initialisation orders them, and during runtime, executes the pass
	 * RenderGraphBuilder -> a description of a 'renderpass' which occurs inside the rendergraph, it holds the initialisation and execution function for the pass
	 *
	 * Setup:
	 * Tether -> a struct which holds information about the resources a 'RenderGraphBuilder' reads and writes to
	 * GraphContext -> a struct which holds pointers to the resources stored inside a renderpass, it is passed to the execution function in the RenderGraphBuilder
	 * FrameInfo -> a struct which holds information about the current frame, index, delta, time, etc
	 *
	 * Resources:
	 * Resource -> a struct which is inherited from for specific resources, contains an id
	 * ImageResource -> a struct containing information about an 'image'
	 * BufferResource -> a struct containing information about a 'buffer'
	 */

	
	enum class ResourceType { Buffer, Image };
	enum class RenderGraphQueue { Graphics, Compute, AsyncCompute };
	enum class ImageSizeClass { Swapchain, Fixed }; // Swapchain means the image is going to be the same size as the swapchain; fixed means you specify size

	class RenderGraph
	{
	private:
		const std::string backBuffer = "_backBuffer";
		
		std::vector<RenderGraphBuilder> builders;
		std::vector<std::unique_ptr<PassDesc>> passes;
		std::vector<std::unique_ptr<Resource>> resources;

		std::unordered_map<std::string, uint32_t> passToIndex;
		std::unordered_map<std::string, uint32_t> resourceToIndex;


		Core* core;

		VkCommandPool pool;
		std::vector<VkCommandBuffer> buffers;

	public:
		RenderGraph(Core* core);
		~RenderGraph();


		ImageResource& GetImage(const std::string& name);
		BufferResource& GetBuffer(const std::string& name);

		void Initialise();
		void Execute();
		void Rebuild();

		Core* GetCore() { return core; }
		std::string GetBackBuffer() const { return backBuffer; }
		
		RenderGraphBuilder& AddPass(const std::string& name, RenderGraphQueue type);
	private:

		// Baking utility (initialisation)
		void ExtractGraphInformation();
		void ValidateGraph(); // debug

		void MergePasses(); // merges passes which are compatible
		void BuildTransients(); // 'merge' or 'reuse' images which are compatible
		void BuildBarriers(); // build synchronisation barriers between items

		void ShowDebugVisualisation();

	};
}
