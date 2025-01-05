#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanTextureView : public TextureView
	{
	public:
		VulkanTextureView(Texture* texture, const TextureViewCreateInfo& info);
		~VulkanTextureView();

		virtual RenderResourceType GetResourceType() const override { return m_ResourceType; }
		virtual void Invalidate() override;

		Ref<VulkanImage> GetImage() const { return m_Image; }
		VkImageView GetVulkanImageView() const { return m_ImageView; }
		VkSampler GetVulkanSampler() const { return m_Sampler; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo();

	private:
		void CleanUp();

	private:
		Ref<VulkanImage> m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		RenderResourceType m_ResourceType;
		VkDescriptorImageInfo m_DescriptorInfo;
	};
}
