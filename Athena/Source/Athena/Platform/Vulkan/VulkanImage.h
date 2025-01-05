#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanImage
	{
	public:
		VulkanImage(const TextureCreateInfo& info, TextureType type, Buffer data);
		~VulkanImage();

		void Resize(uint32 width, uint32 height);

		void WriteContentToBuffer(Buffer* dstBuffer);

		uint32 GetMipLevelsCount() const { return m_MipLevels; }

		VkImage GetVulkanImage() const { return m_Image.GetImage(); }
		VkImageView GetVulkanImageView() const { return m_ImageView; }
		VkImageLayout GetLayout() const { return m_Layout; }

		void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
		void RenderPassUpdateLayout(VkImageLayout newLayout);

	private:
		void UploadData(Buffer data, uint32 width, uint32 height);
		void CleanUp();

	private:
		TextureCreateInfo m_Info;
		TextureType m_Type;
		uint32 m_MipLevels;
		VkImageLayout m_Layout;
		VulkanImageAllocation m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
	};
}
