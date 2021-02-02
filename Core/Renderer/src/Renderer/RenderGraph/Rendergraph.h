#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include <vector>
#include <unordered_map>
#include <string>
#include "PassDesc.h"


namespace Renderer
{
	namespace Memory {
		class Image;
	}

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
	class Core;

	class Rendergraph
	{
	private:
		bool initialised = false;
		uint16_t curIndex = 1;
		std::unordered_map<std::string, uint16_t> strToIndex;

		std::vector<PassDesc> passes;
		std::vector<PassDesc> uniquePasses;


		Core* core;

		VkCommandPool pool;
		Memory::Image* depthImage;
		std::vector<VkCommandBuffer> buffers;

	public:
		Rendergraph(Core* core);
		~Rendergraph();

		void Initialise();
		void Execute();
		void Rebuild();
		Core* GetCore() { return core; };

		void AddPass(PassDesc passDesc);
	private:

		// Baking utility (initialisation)
		void extractGraphInformation();
		// assigns id's, locates resources required, builds a dependancy graph
		// builds the required resources
		void validateGraph(); // debug

		void mergePasses(); // merges passes which are compatible
		void buildTransients(); // 'merge' or 'reuse' images which are compatible
		void buildBarriers(); // build synchronisation barriers between items

		void ShowDebugVisualisation();
	private:
		uint16_t assignId() { return ++curIndex; }
	};
}
