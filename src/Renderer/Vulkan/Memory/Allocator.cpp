#include "Allocator.h"
#include "Allocation.h"
#include "Block.h"
#include "../../../Utils/Logging.h"
#include "../Device.h"

namespace Renderer::Memory
{
	Allocator::Allocator(Device* device, int framesInFlight) : device(device), framesInFlight(framesInFlight)
	{
		physMemoryProps = device->GetPhysicalDeviceMemoryProperties();
		buffersToClear.resize(framesInFlight);
	}

	Allocator::~Allocator()
	{
		for(const auto& buffs : buffersToClear)
		{
			for (const auto& [alloc, buffer] : buffs)
			{
				vkDestroyBuffer(*device, buffer, nullptr);
			}
		}
		buffersToClear.clear();

		
		for (const auto& buffer : allocatedBuffers)
		{
			vkDestroyBuffer(*device, buffer, nullptr);
		}
		
		allocatedBuffers.clear();

		for (auto& blocks : memoryBlocks)
		{
			for (auto& block : blocks)
			{
				block->Clear();
				delete block;
			}
			blocks.clear();
		}
	}

	Buffer* Allocator::AllocateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& flags)
	{
		VkBuffer buffer;
		VkBufferCreateInfo buffCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffCreateInfo.size = size;
		buffCreateInfo.usage = usage;
		buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto success = vkCreateBuffer(*device, &buffCreateInfo, nullptr, &buffer);
		Assert(success == VK_SUCCESS, "Failed to allocate buffer");

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(*device, buffer, &memReqs);

		auto memory = FindAllocation(memReqs, flags);

		vkBindBufferMemory(*device, buffer, memory.parent->memory, memory.offset);

		auto* buf = new Buffer{
			memory, buffer, usage, flags, size, [=](Buffer* b)
			{
				this->allocatedBuffers.erase(buffer);
				this->DeallocateBuffer(b);
			}
		};

		allocatedBuffers.emplace(buf->resourceHandle);

		return buf;
	}


	//Image Allocator::AllocateImage(const VkExtent2D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags) { }

	void Allocator::DeallocateBuffer(Buffer* buffer)
	{
		buffersToClear[currentFrameOffset].emplace_back(buffer->allocation, buffer->resourceHandle);
	}

	void Allocator::BeginFrame() { }

	void Allocator::EndFrame()
	{
		currentFrameOffset = (currentFrameOffset + 1) % framesInFlight;
		for (const auto& [alloc, buffer] : buffersToClear[currentFrameOffset])
		{
			alloc.parent->FreeAllocation(alloc);
			vkDestroyBuffer(*device, buffer, nullptr);
		}
		buffersToClear[currentFrameOffset].clear();
	}

	void Allocator::DebugView()
	{
		auto& getFormattedBytes = [](const VkDeviceSize& size) -> std::string
		{
			int i = 0;
			long long usedSize = size;
			for (i = 0; usedSize >> 10 > 0; i++) { usedSize >>= 10; }

			const char* suffix;
			switch (i)
			{
				case 0: suffix = "b";
					break;
				case 1: suffix = "kb";
					break;
				case 2: suffix = "mb";
					break;
				case 3: suffix = "gb";
					break;
			}

			std::string string;
			char buffer[10];
			sprintf_s(buffer, "%lld%s", usedSize, suffix);
			string.assign(&buffer[0]);
			return string;
		};

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		for (int j = 0; j < memoryBlocks.size(); j++)
		{
			auto& blocks = memoryBlocks[j];
			for (auto& block : blocks)
			{
				ImGui::BeginChild("Block");
				ImGui::Text("Allocation Memory Heap Index: %d", block->heapIndex);

				ImGui::BeginGroup();
				ImGui::Text("Allocation Block: ");
				ImGui::SameLine();
				char buffer[50];
				sprintf_s(buffer, "%s / %s", getFormattedBytes(block->utilisedSize).c_str(), getFormattedBytes(block->size).c_str());
				ImGui::ProgressBar(block->utilisedSize / static_cast<float>(block->size), ImVec2{ -1, 0 }, buffer);
				ImGui::EndGroup();
				ImGui::Text("Sub Allocation Count %d, coherency %f%%", block->usedAllocs.size(), (block->utilisedSize / (float)block->size) * 100);

				/* Memory Graph */
				char b[30];
				sprintf_s(b, "Memory Graph for %d - id %d", block->heapIndex, block->GetId());
				if (ImGui::CollapsingHeader(b))
				{
					ImVec2 canvas_pos = ImGui::GetCursorScreenPos(); 
					ImVec2 canvas_size = ImGui::GetContentRegionAvail();
					if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
					canvas_size.y = 50.0f;

					ImGui::InvisibleButton(b, canvas_size);
					draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(0, 0, 255, 255));

					static ImU32 colours[3] = { IM_COL32(102, 51, 0, 255), IM_COL32(230, 224, 176, 255), IM_COL32(190, 82, 42, 255) };

					int colourMult = 0;
					for (const auto& alloc : block->GetUsedAllocs())
					{
						auto begin = ImVec2(canvas_pos.x + (alloc.offset / (float)block->size) * canvas_size.x, canvas_pos.y);
						auto end = ImVec2(canvas_pos.x + ((alloc.offset + alloc.size) / (float)block->size) * canvas_size.x, canvas_pos.y + canvas_size.y);

						if (ImGui::IsMouseHoveringRect(begin, end)) ImGui::SetTooltip("Offset: %s\nSize: %s", getFormattedBytes(alloc.GetOffset()).c_str(), getFormattedBytes(alloc.GetSize()).c_str());

						draw_list->AddRectFilled(begin, end, colours[alloc.id % 3]);
						colourMult++;
					}
				}

				sprintf_s(b, "Memory Table for %d - id %d", block->heapIndex, block->GetId());
				if (ImGui::CollapsingHeader(b))
				{
					ImGui::Columns(2, "Sub Allocations", true);
					ImGui::Text("Offset");
					ImGui::NextColumn();
					ImGui::Text("Size");
					ImGui::NextColumn();
					ImGui::Separator();

					for (const auto& alloc : block->GetUsedAllocs())
					{
						ImGui::Text("%s", getFormattedBytes(alloc.GetOffset()).c_str());
						ImGui::NextColumn();
						ImGui::Text("%s", getFormattedBytes(alloc.GetSize()).c_str());
						ImGui::NextColumn();
						ImGui::Separator();
					}
				}
				ImGui::EndChild();

			}
		}
	}

	Allocation Allocator::FindAllocation(const VkMemoryRequirements& memReqs, VkMemoryPropertyFlags requiredProperties)
	{
		const uint32_t memoryTypeIndex = findProperties(memReqs.memoryTypeBits, requiredProperties);
		const uint32_t heapIndex = physMemoryProps.memoryTypes[memoryTypeIndex].heapIndex;
		auto& blocks = memoryBlocks[heapIndex];

		for (auto& block : blocks)
		{
			if (block->GetSize() - block->GetUsedSize() < memReqs.size) continue;

			auto alloc = block->TryFindMemory(memReqs.size);
			if (alloc.has_value()) return alloc.value();
		}

		auto size = physMemoryProps.memoryHeaps[heapIndex].size;
		size = std::min((memReqs.size > 0x4000000) ? memReqs.size : 0x4000000, size - 1);
		auto& buff = memoryBlocks[heapIndex].emplace_back(new Block{this, memoryTypeIndex, heapIndex, static_cast<uint32_t>(memoryBlocks[heapIndex].size()), size});

		const auto alloc = memoryBlocks[heapIndex][memoryBlocks[heapIndex].size() - 1]->TryFindMemory(memReqs.size);
		if (alloc.has_value()) return alloc.value();

		Assert(false, "Failed to find allocation");
		return Allocation{ buff, VkDeviceSize{ 0 }, VkDeviceSize{ 0 } ,0 };
	}
}
