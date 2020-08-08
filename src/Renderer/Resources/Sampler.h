#pragma once

namespace Renderer
{
	class Sampler
	{
		private:
			VkSampler sampler;
			VkDevice* device;
		public:

			Sampler(VkDevice* device, VkSamplerAddressMode addressMode, VkFilter filter, VkSamplerMipmapMode mipFilter) : device(device)
			{
				VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
				samplerInfo.maxAnisotropy = 1.0f;
				samplerInfo.magFilter = filter;
				samplerInfo.minFilter = filter;
				samplerInfo.mipmapMode = mipFilter;
				samplerInfo.addressModeU = addressMode;
				samplerInfo.addressModeV = addressMode;
				samplerInfo.addressModeW = addressMode;
				samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				samplerInfo.minLod = -1000;
				samplerInfo.maxLod = 1000;

				vkCreateSampler(*device, &samplerInfo, nullptr, &sampler);
			}

			~Sampler() { if (device) vkDestroySampler(*device, sampler, nullptr); }

			VkSampler getSampler() { return sampler; }
	};
}
