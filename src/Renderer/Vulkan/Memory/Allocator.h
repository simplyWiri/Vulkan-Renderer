#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "../../Resources/Image.h"
#include "PriorityQueue.h"
#include "imgui.h"
#include <stdio.h>
#include <optional>

#include "Buffer.h"

namespace Renderer::Memory
{
	/*
		{
			VkBuffer handle;
			SubAllocation memory;
		}

		var buffer = renderer->GetAllocator->AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags);

		// in the update loop

		vkBindBuffer(buffer, buffer.size, buffer.offset);

		buffer.Destroy();
		// This can't just be destroyed
		// we need to wait until the
		// frame it was last used in
		// has been consumed by the gpu

		{
			var stagingBuffer = renderer->GetAllocator->AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags);

			stagingBuffer.load(xxx);

			vkCommandBuffer(stagingBuffer);
		}
	*/

	class Allocator
	{
		friend class Allocation;
		friend class Buffer;
	private:
		Device* device;
		VkPhysicalDeviceMemoryProperties physMemoryProps;
		VkQueue transferQueue;
		uint32_t currentFrameOffset = 0;
		uint32_t framesInFlight;

		std::array<PriorityQueue<Allocation>, VK_MAX_MEMORY_TYPES> allocations;
		std::vector<VkBuffer> buffers;

	public:

		Allocator(Device* device, int framesInFlight = 3) : device(device), framesInFlight(framesInFlight)
		{
			physMemoryProps = device->GetPhysicalDeviceMemoryProperties();
			transferQueue = device->queues.transfer;
		}

		~Allocator()
		{
			for (auto& heap : allocations)
			{
				for (auto& allocation : heap)
				{
					for (auto& buffer : allocation.buffersToClear)
					{
						vkDestroyBuffer(*device, std::get<1>(buffer), nullptr);
					}
					vkFreeMemory(*device, allocation.memory, nullptr);
				}
			}


		}

		Buffer AllocBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
		{
			VkBuffer buffer;
			VkBufferCreateInfo buffCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			buffCreateInfo.size = size;
			buffCreateInfo.usage = usage;
			buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			auto success = vkCreateBuffer(*device, &buffCreateInfo, nullptr, &buffer);
			Assert(success == VK_SUCCESS, "Failed to allocate buffer");

			auto memory = RequestMemory(buffer, flags);

			vkBindBufferMemory(*device, buffer, memory.parent->memory, memory.offset);

			return Buffer{ buffer, memory, usage, flags };
		}

		void BeginFrame(uint32_t index)
		{
			this->currentFrameOffset = index;
		}

		void EndFrame()
		{

		}

		void DebugView()
		{
			auto& getFormattedBytes = [](const VkDeviceSize& size) -> std::string
			{
				int i = 0;
				long long usedSize = size;
				for (i = 0; usedSize >> 10 > 0; i++)
				{
					usedSize >>= 10;
				}
				const char* suffix;
				switch (i)
				{
				case 0: suffix = "b"; break;
				case 1: suffix = "kb"; break;
				case 2: suffix = "mb"; break;
				case 3: suffix = "gb"; break;
				}

				std::string string;
				char buffer[10];
				sprintf_s(buffer, "%lld%s", usedSize, suffix);
				string.assign(&buffer[0]);
				return string;
			};

			int i = 0;
			for (auto& queue : allocations)
			{

				if (queue.Size() == 0)
				{
					i++;
					continue;
				}

				ImGui::Text("Allocation Memory Heap Index: %d", i);

				for (auto& alloc : queue)
				{
					ImGui::BeginGroup();
					ImGui::Text("Allocation Block: ");
					ImGui::SameLine();
					char buffer[50];
					sprintf_s(buffer, "%s / %s", getFormattedBytes(alloc.usedSize).c_str(), getFormattedBytes(alloc.size).c_str());
					ImGui::ProgressBar(alloc.usedSize / (float)alloc.size, ImVec2{ -1, 0 }, buffer);
					ImGui::EndGroup();
					ImGui::Text("Sub Allocation Count %d", alloc.subAllocations.size());

					ImGui::Columns(2, "Sub Allocations", true);
					ImGui::Text("Offset");
					ImGui::NextColumn();
					ImGui::Text("Size");
					ImGui::NextColumn();
					ImGui::Separator();

					for (auto& subAlloc : alloc.subAllocations)
					{
						ImGui::Text("%s", getFormattedBytes(subAlloc.offset).c_str());
						ImGui::NextColumn();
						ImGui::Text("%s", getFormattedBytes(subAlloc.range).c_str());
						ImGui::NextColumn();
						ImGui::Separator();
					}
				}

				i++;
			}
		}

	private:





		SubAllocation RequestMemory(VkBuffer buffer, VkMemoryPropertyFlags flags)
		{
			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(*device, buffer, &memReqs); // any alignment requirements are applied here
			const uint32_t memoryTypeIndex = findProperties(memReqs.memoryTypeBits, flags);
			const uint32_t heapIndex = physMemoryProps.memoryTypes[memoryTypeIndex].heapIndex;

			for (auto& allocation : allocations[heapIndex])
			{
				if (allocation.size - allocation.usedSize > memReqs.size) continue;

				auto subAlloc = TryFindMemory(allocation, memReqs.size);
				if (subAlloc.has_value()) return subAlloc.value();
			}

			auto alloc = AllocateMemory(memReqs.size, memoryTypeIndex);
			alloc.freeSubAllocations.Push(SubAllocation{&alloc, 0, alloc.size});
			auto subAlloc = TryFindMemory(alloc, memReqs.size);

			if (subAlloc.has_value())
			{
				allocations[heapIndex].Push(alloc);
				return subAlloc.value();
			}

			LogError("Out of Memory");
			return SubAllocation{};
		}

		std::optional<SubAllocation> TryFindMemory(Allocation& allocation, VkDeviceSize size)
		{
			for (auto& freeLocation : allocation.freeSubAllocations)
			{
				if (freeLocation.range > size)
				{
					auto& subAlloc = SubAllocation{ &allocation, freeLocation.offset, size };
					allocation.subAllocations.push_back(subAlloc);
					freeLocation.offset += size;
					freeLocation.range -= size;

					allocation.usedSize += size;

					return subAlloc;
				}
			}
			return std::nullopt;
		}

		Allocation AllocateMemory(const VkDeviceSize& size, const uint32_t& memoryTypeIndex)
		{
			Allocation allocation = {};
			// min for dedicated is 64MB, default is 256MB
			// 1024*1024*64 and 1024*1024*256 respectively
			allocation.parent = this;
			allocation.size = (size > 0x4000000) ? size : 0x10000000;
			allocation.memoryTypeIndex = memoryTypeIndex;

			VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			info.allocationSize = allocation.size;
			info.memoryTypeIndex = allocation.memoryTypeIndex;

			vkAllocateMemory(*device, &info, nullptr, &allocation.memory);
			return allocation;
		}

		uint32_t findProperties(uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties) const
		{
			for (uint32_t i = 0; i < physMemoryProps.memoryTypeCount; i++)
				if (memoryTypeBitsRequirement & (1 << i) && (physMemoryProps.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties)
					return i;

			return ~0;
		}

	};

}
