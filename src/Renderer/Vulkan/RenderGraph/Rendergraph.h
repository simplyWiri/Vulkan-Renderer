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
#include "../Wrappers/Swapchain.h"
#include "Tether.h"

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
	struct GraphContext;

	// Image Attachment
	struct Resource
	{
		uint16_t id;
	};
	
	struct ImageResource : Resource
	{
		SizeClass sizeClass = SwapchainRelative;
		glm::vec3 size = {1.0f, 1.0f, 0.0f};
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t samples = 1, levels = 1, layers = 1;
		VkImageUsageFlags usage = 0;
	};

	// Buffer Attachment
	struct BufferResource : Resource
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
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
		std::function<void(Tether&)> initialisation;
		std::function<void(FrameInfo&, GraphContext&)> execute;
	};

	class Rendergraph
	{
	public:
		
	struct GraphContext
	{
		GraphContext(Rendergraph* graph)
		{
			this->graph = graph;
		}
		
		Rendergraph* graph;
		Resource& getResource(const std::string& key) const
		{
			return graph->resources[graph->strToIndex[key]];
		}

	};
	private:
		bool initialised = false;

		std::unordered_map<std::string, uint16_t> strToIndex;
		std::vector<Resource> resources;
		
	public:
		Rendergraph(Core* core);

		static std::string GetBackBufferSourceID() { return "graph:backbuffer"; }

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
