#include "VulkanTexture2D.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	static VkImageUsageFlags GetVulkanImageUsage(TextureUsage usage, TextureFormat format)
	{
		bool depthStencil = Texture::IsDepthFormat(format) || Texture::IsStencilFormat(format);

		switch (usage)
		{
		case TextureUsage::SHADER_READ_ONLY: return VK_IMAGE_USAGE_SAMPLED_BIT;
		case TextureUsage::ATTACHMENT: return depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkImageUsageFlags)0;
	}


	VulkanTexture2D::VulkanTexture2D(const TextureCreateInfo& info)
	{
		m_Info = info;

		Buffer localBuffer;
		if(info.Data != nullptr)
			localBuffer = Buffer::Copy(info.Data, info.Width * info.Height * Texture::BytesPerPixel(info.Format));

		Ref<VulkanTexture2D> instance(this);
		Renderer::Submit([instance, localBuffer]() mutable
		{
			auto& info = instance->m_Info;

			instance->m_AddImGuiTexture = info.GenerateSampler && Application::Get().GetConfig().EnableImGui;

			uint32 imageUsage = GetVulkanImageUsage(info.Usage, info.Format);

			if (info.Usage == TextureUsage::ATTACHMENT)
			{
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;	// TODO (this usage added for copying into swapchain)
			}

			if (info.Data != nullptr)
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;


			if (info.GenerateMipMap && info.MipLevels == 0)
				info.MipLevels = Math::Floor(Math::Log2(Math::Max(info.Width, info.Height))) + 1;

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = info.Width;
			imageInfo.extent.height = info.Height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = info.MipLevels;
			imageInfo.arrayLayers = info.Layers;
			imageInfo.format = VulkanUtils::GetFormat(info.Format, info.sRGB);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = imageUsage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			VK_CHECK(vkCreateImage(VulkanContext::GetLogicalDevice(), &imageInfo, VulkanContext::GetAllocator(), &instance->m_VkImage));


			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(VulkanContext::GetLogicalDevice(), instance->m_VkImage, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VulkanUtils::GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VK_CHECK(vkAllocateMemory(VulkanContext::GetLogicalDevice(), &allocInfo, VulkanContext::GetAllocator(), &instance->m_ImageMemory));

			vkBindImageMemory(VulkanContext::GetLogicalDevice(), instance->m_VkImage, instance->m_ImageMemory, 0);

			if (info.Data != nullptr)
			{
				instance->UploadData(localBuffer.Data(), info.Width, info.Height);

				localBuffer.Release();
				info.Data = nullptr;
			}

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = instance->m_VkImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VulkanUtils::GetFormat(info.Format, info.sRGB);
			viewInfo.subresourceRange.aspectMask = VulkanUtils::GetImageAspectFlags(info.Format);
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = info.MipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = info.Layers;

			VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, VulkanContext::GetAllocator(), &instance->m_VkImageView));
		});

		if(m_Info.GenerateSampler)
			ResetSampler(m_Info.SamplerInfo);

		if (m_Info.GenerateMipMap)
			GenerateMipMap(m_Info.MipLevels);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([set = m_DescriptorSet, vkSampler = m_Sampler, vkImageView = m_VkImageView, 
				vkImage = m_VkImage, imageMemory = m_ImageMemory]()
		{
			if(set != VK_NULL_HANDLE)
				ImGui_ImplVulkan_RemoveTexture(set);

			vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, VulkanContext::GetAllocator());
			vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, VulkanContext::GetAllocator());

			vkDestroyImage(VulkanContext::GetLogicalDevice(), vkImage, VulkanContext::GetAllocator());
			vkFreeMemory(VulkanContext::GetLogicalDevice(), imageMemory, VulkanContext::GetAllocator());
		});
	}

	void VulkanTexture2D::ResetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_Sampler, set = m_DescriptorSet]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, VulkanContext::GetAllocator());

				if(set != VK_NULL_HANDLE)
					ImGui_ImplVulkan_RemoveTexture(set);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		Ref<VulkanTexture2D> instance(this);
		Renderer::Submit([instance]()
		{
			auto& info = instance->m_Info;

			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = VulkanUtils::GetFilter(info.SamplerInfo.MagFilter);
			vksamplerInfo.minFilter = VulkanUtils::GetFilter(info.SamplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = VulkanUtils::GetMipMapMode(info.SamplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeV = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeW = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.0f;
			vksamplerInfo.compareEnable = false;
			vksamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			vksamplerInfo.minLod = -1000;
			vksamplerInfo.maxLod = 1000;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, VulkanContext::GetAllocator(), &instance->m_Sampler));

			if (instance->m_AddImGuiTexture)
				instance->m_DescriptorSet = ImGui_ImplVulkan_AddTexture(instance->m_Sampler, instance->m_VkImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});
	}

	void VulkanTexture2D::UploadData(const void* data, uint32 width, uint32 height)
	{
		VkDeviceSize imageSize = width * height * Texture::BytesPerPixel(m_Info.Format);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VulkanUtils::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		void* mappedMemory;
		vkMapMemory(VulkanContext::GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedMemory);
		memcpy(mappedMemory, data, imageSize);
		vkUnmapMemory(VulkanContext::GetLogicalDevice(), stagingBufferMemory);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_VkImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkCommandBuffer vkCommandBuffer = VulkanUtils::BeginSingleTimeCommands();
		{
			// Barrier
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					vkCommandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VulkanUtils::GetImageAspectFlags(m_Info.Format);
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(vkCommandBuffer, stagingBuffer, m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			// Barrier
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(
					vkCommandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}
		}
		VulkanUtils::EndSingleTimeCommands(vkCommandBuffer);

		vkDestroyBuffer(VulkanContext::GetLogicalDevice(), stagingBuffer, VulkanContext::GetAllocator());
		vkFreeMemory(VulkanContext::GetLogicalDevice(), stagingBufferMemory, VulkanContext::GetAllocator());
	}

	void VulkanTexture2D::GenerateMipMap(uint32 levels)
	{
		// TODO
	}
}
