#include "Allocator.h"
#include "Allocation.h"
#include "Block.h"
#include "../../Utils/Logging.h"
#include "../VulkanObjects/Device.h"
#include "Buffer.h"
#include "Image.h"
#include "imgui.h"

namespace Renderer::Memory
{
	Allocator::Allocator(Device* device, int framesInFlight) : device(device), framesInFlight(framesInFlight)
	{
		physMemoryProps = device->GetPhysicalDeviceMemoryProperties();
		cleanups.resize(framesInFlight);
		transferQueue = device->queues.transfer;
	}

	Allocator::~Allocator()
	{
		for (const auto& cleanup : cleanups) { for (const auto& func : cleanup) { func(*device); } }
		cleanups.clear();

		for (const auto& buffer : allocatedBuffers) { vkDestroyBuffer(*device, buffer, nullptr); }
		for (const auto& image : allocatedImages) { vkDestroyImage(*device, image, nullptr); }

		allocatedBuffers.clear();
		allocatedImages.clear();

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

		success = vkBindBufferMemory(*device, buffer, memory.parent->memory, memory.offset);
		Assert(success == VK_SUCCESS, "Failed to bind buffer memory");

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

	Image* Allocator::AllocateImage(const VkExtent3D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags)
	{
		VkImage image;
		VkImageView view;
		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = { extent.width, extent.height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto success = vkCreateImage(*device, &imageInfo, nullptr, &image);
		Assert(success == VK_SUCCESS, "Failed to create vkimage");

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(*device, image, &memReqs);

		auto memory = FindAllocation(memReqs, flags);

		success = vkBindImageMemory(*device, image, memory.parent->memory, memory.offset);
		Assert(success == VK_SUCCESS, "Failed to bind image memory");

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		VkImageSubresourceRange range = {};
		if (format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT
		) range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		viewInfo.subresourceRange = range;

		success = vkCreateImageView(*device, &viewInfo, nullptr, &view);
		Assert(success == VK_SUCCESS, "Failed to create image view");

		auto* img = new Image{
			image, view, memory, range, extent, format, usage, [=](Image* i)
			{
				this->allocatedImages.erase(image);
				this->DeallocateImage(i);
			}
		};

		allocatedImages.emplace(image);

		return img;
	}

	Image* Allocator::AllocateImage(const VkExtent2D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags) { return AllocateImage({ extent.width, extent.height, 1 }, format, usage, flags); }


	void Allocator::DeallocateBuffer(Buffer* buffer)
	{
		auto a = buffer->allocation;
		auto h = buffer->GetResourceHandle();

		cleanups[currentFrameOffset].emplace_back([=](VkDevice d)
		{
			a.parent->FreeAllocation(a);
			vkDestroyBuffer(d, h, nullptr);
		});
	}

	void Allocator::DeallocateImage(Image* image)
	{
		auto a = image->GetAllocation();
		auto h = image->GetResourceHandle();
		auto v = image->GetView();

		cleanups[currentFrameOffset].emplace_back([=](VkDevice d)
		{
			a.parent->FreeAllocation(a);
			vkDestroyImage(d, h, nullptr);
			vkDestroyImageView(d, v, nullptr);
		});
	}

	void Allocator::BeginFrame() { }

	void Allocator::EndFrame()
	{
		currentFrameOffset = (currentFrameOffset + 1) % framesInFlight;
		for (const auto& cleanup : cleanups[currentFrameOffset]) { cleanup(*device); }
		cleanups[currentFrameOffset].clear();
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
				ImGui::Text("Sub Allocation Count %d, coherency %.2f%%", block->allocations.size(), (block->utilisedSize / static_cast<double>(block->size)) * 100.0);

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

					static ImColor colours[3] = { { 102, 51, 0, 255 }, { 230, 224, 176, 255 }, { 190, 82, 42, 255 } };
					static ImColor freeColours[3] = { {52 , 66, 227, 255 }, { 48, 196, 244, 255 }, { 24, 117, 255, 255 } };

					int colourMult = 0;
					for (const auto& alloc : block->GetAllocations())
					{
						auto begin = ImVec2(canvas_pos.x + (alloc.offset / static_cast<float>(block->size)) * canvas_size.x, canvas_pos.y);
						auto end = ImVec2(canvas_pos.x + ((alloc.offset + alloc.size) / static_cast<float>(block->size)) * canvas_size.x, canvas_pos.y + canvas_size.y);

						if (ImGui::IsMouseHoveringRect(begin, end)) ImGui::SetTooltip("Offset: %s\nSize: %s", getFormattedBytes(alloc.GetOffset()).c_str(), getFormattedBytes(alloc.GetSize()).c_str());

						draw_list->AddRectFilled(begin, end, alloc.inUse ? colours[alloc.GetOffset() % 3] : freeColours[alloc.GetOffset() % 3]);
						colourMult++;
					}
				}

				sprintf_s(b, "Memory Table for %d - id %d", block->heapIndex, block->GetId());
				if (ImGui::CollapsingHeader(b))
				{
					ImGui::Columns(3, "Sub Allocations", true);
					ImGui::Text("Offset");
					ImGui::NextColumn();
					ImGui::Text("Size");
					ImGui::NextColumn();
					ImGui::Text("Used");
					ImGui::NextColumn();
					ImGui::Separator();

					for (const auto& alloc : block->GetAllocations())
					{
						ImGui::Text("%s", getFormattedBytes(alloc.GetOffset()).c_str());
						ImGui::NextColumn();
						ImGui::Text("%s", getFormattedBytes(alloc.GetSize()).c_str());
						ImGui::NextColumn();
						ImGui::Text("%s", alloc.inUse ? "True" : "False");
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
		// 0x4000000
		size = std::min((memReqs.size > 0x4000000) ? memReqs.size : 0x4000000, size - 1);
		auto& buff = memoryBlocks[heapIndex].emplace_back(new Block{ this, memoryTypeIndex, heapIndex, static_cast<uint32_t>(memoryBlocks[heapIndex].size()), size });

		const auto alloc = memoryBlocks[heapIndex][memoryBlocks[heapIndex].size() - 1]->TryFindMemory(memReqs.size);
		if (alloc.has_value()) return alloc.value();

		Assert(false, "Failed to find allocation");
		return Allocation{ buff, VkDeviceSize{ 0 }, VkDeviceSize{ 0 }, true };
	}
}
