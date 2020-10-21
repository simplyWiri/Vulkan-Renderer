#pragma once
#include <vector>
#include <vulkan.h>

#include "Cache.h"


namespace Renderer
{
	struct ShaderResources;
	class ShaderProgram;

	namespace Memory
	{
		class Allocator;
		class Buffer;
		class Image;
	}

	class Sampler;

	struct DescriptorSetKey
	{
		ShaderProgram* program;

		bool operator ==(const DescriptorSetKey& other) const;
	};

	class DescriptorSetBundle
	{
	public:
		DescriptorSetBundle(VkDevice* device, Memory::Allocator* allocator, DescriptorSetKey key, uint32_t framesInFlight);

		void WriteBuffer(const std::string& resName, Memory::Buffer* buffer);

		void WriteSampler(const std::string& resName, Memory::Image* image, Sampler* sampler, VkImageLayout layout);
		ShaderResources GetShaderResource(const std::string& resName) const;

		VkDescriptorSet* Get(uint32_t offset) { return &sets[offset]; }
		VkDescriptorPool GetPool() { return pool; }

		void* GetResource(const std::string& name, uint32_t offset);
		void SetResource(const std::string& name, void* data, size_t size, size_t offset);

		void Clear();

	private:
		uint32_t framesInFlight;
		std::vector<ShaderResources> resources;
		VkDevice* device;
		Memory::Allocator* allocator;
		std::unordered_map<std::string, Memory::Buffer*> buffers;
		std::unordered_map<std::string, Memory::Image*> images;
		std::unordered_map<std::string, Sampler*> samplers;

		std::vector<VkDescriptorSet> sets;
		VkDescriptorPool pool;
	};
}

namespace std
{
	template <>
	struct hash<Renderer::DescriptorSetKey>
	{
		size_t operator()(const Renderer::DescriptorSetKey& s) const noexcept { return hash<uint64_t>{}(reinterpret_cast<uint64_t>(s.program)); }
	};
}

namespace Renderer
{
	class DescriptorSetCache : public Cache<DescriptorSetBundle, DescriptorSetKey>
	{
	private:
		VkDevice* device;
		Memory::Allocator* allocator;

	public:
		void BuildCache(VkDevice* device, Memory::Allocator* allocator, uint32_t framesInFlight);

		void WriteBuffer(DescriptorSetKey& key, const std::string& resName, Memory::Buffer* buffer);

		void WriteSampler(DescriptorSetKey& key, const std::string& resName, Memory::Image* image, Sampler* sampler, VkImageLayout layout);

		template <typename T>
		T* GetResource(const DescriptorSetKey& key, std::string resName);

		void SetResource(const DescriptorSetKey& key, std::string resName, void* data, size_t size);

		void BindDescriptorSet(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, const DescriptorSetKey& key);

		DescriptorSetBundle* Get(const DescriptorSetKey& key) override;
		bool Add(const DescriptorSetKey& key) override;

	private:

		void ClearEntry(DescriptorSetBundle* set) override;
	};

	template <typename T>
	T* DescriptorSetCache::GetResource(const DescriptorSetKey& key, std::string resName)
	{
		auto descSet = Get(key);

		return static_cast<T*>(descSet->GetResource(resName, currentFrame));
	}
}
