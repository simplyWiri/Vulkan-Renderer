#pragma once
#include "vulkan.h"
#include "../Core.h"
#include <vector>

namespace Renderer
{

	class Frame
	{
	public:

	private:
		struct FrameResources
		{
			VkSemaphore imageAcquired;
			VkSemaphore renderFinished;
			VkFence inFlightFence;

			VkCommandBuffer buffer;
		};

		Core* core;

		std::vector<FrameResources> resources;

	public:
		Frame(Core* core, uint32_t imageCount, VkPresentModeKHR presentMode)
		{
			this->core = core;
			
			for (auto i = 0; i < imageCount; i++)
			{
				FrameResources frameRes;

				frameRes.inFlightFence = createFence();
				frameRes.imageAcquired = createSemaphore();
				frameRes.renderFinished = createSemaphore();

				frameRes.buffer = core->AllocateCommandBuffer();

				resources.push_back(frameRes);
			}
		}

	private:
			
		VkFence createFence();
		VkSemaphore createSemaphore();
	
	};

}

