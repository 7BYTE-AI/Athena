#include "VulkanTextureView.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"


namespace Athena
{
	VulkanTextureView::VulkanTextureView(Texture* texture, const TextureViewCreateInfo& info)
	{
		m_Info = info;
		m_Texture = texture;
		m_Image = Vulkan::GetImage(texture);

		if (m_Info.Name.empty())
		{
			m_Info.Name = std::format("{}_Mip[{},{}]_Layer[{},{}]", texture->GetName(),
				info.BaseMipLevel, info.BaseMipLevel + info.MipLevelCount - 1, 
				info.BaseLayer, info.BaseLayer + info.LayerCount - 1);
		}

		if (m_Info.OverrideSampler)
		{
			m_Sampler = VulkanContext::GetAllocator()->CreateSampler(m_Info.Sampler);
			m_DescriptorInfo.sampler = m_Sampler;
		}
		else
		{
			m_Info.Sampler = m_Texture->GetInfo().Sampler;
			m_Sampler = VulkanContext::GetAllocator()->CreateSampler(m_Info.Sampler);
			m_DescriptorInfo.sampler = m_Sampler;
		}

		Invalidate();
	}

	VulkanTextureView::~VulkanTextureView()
	{
		CleanUp();

		Renderer::SubmitResourceFree([this, samplerInfo = m_Info.Sampler, vkSampler = m_Sampler]()
		{
			VulkanContext::GetAllocator()->DestroySampler(samplerInfo, vkSampler);
		});
	}

	void VulkanTextureView::Invalidate()
	{
		if (m_ImageView)
		{
			CleanUp();
		}

		if (m_Texture == nullptr)
		{
			ATN_CORE_ASSERT(false);
			return;
		}

		TextureFormat format = m_Texture->GetFormat();

		VkComponentMapping swizzling = {};
		swizzling.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		swizzling.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		swizzling.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		swizzling.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		if (!m_Info.EnableAlphaBlending)
			swizzling.a = VK_COMPONENT_SWIZZLE_ONE;

		if (m_Info.GrayScale)
		{
			swizzling.g = VK_COMPONENT_SWIZZLE_R;
			swizzling.b = VK_COMPONENT_SWIZZLE_R;
		}

		uint32 texMipLevelsCount = m_Texture->GetMipLevelsCount();
		uint32 viewBaseMipLevel = m_Info.BaseMipLevel;
		uint32 viewMipLevelCount = m_Info.MipLevelCount;
		
		if (viewBaseMipLevel >= texMipLevelsCount)
			viewBaseMipLevel = texMipLevelsCount - 1;

		if (viewBaseMipLevel + viewMipLevelCount > texMipLevelsCount)
			viewMipLevelCount = texMipLevelsCount - viewBaseMipLevel;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image->GetVulkanImage();
		viewInfo.viewType = Vulkan::GetImageViewType(m_Texture->GetType(), m_Info.LayerCount);
		viewInfo.format = Vulkan::GetFormat(format);
		viewInfo.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(format);
		viewInfo.subresourceRange.baseMipLevel = viewBaseMipLevel;
		viewInfo.subresourceRange.levelCount = viewMipLevelCount;
		viewInfo.subresourceRange.baseArrayLayer = m_Info.BaseLayer;
		viewInfo.subresourceRange.layerCount = m_Info.LayerCount;
		viewInfo.components = swizzling;

		VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));
		Vulkan::SetObjectDebugName(m_ImageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, m_Info.Name);

		if (viewInfo.viewType == VK_IMAGE_VIEW_TYPE_2D || viewInfo.viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
			m_ResourceType = RenderResourceType::TextureView2D;

		else if (viewInfo.viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewInfo.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			m_ResourceType = RenderResourceType::TextureViewCube;
	}

	void VulkanTextureView::CleanUp()
	{
		Renderer::SubmitResourceFree([this, imageView = m_ImageView]()
		{
			vkDestroyImageView(VulkanContext::GetLogicalDevice(), imageView, nullptr);
		});
	}

	const VkDescriptorImageInfo& VulkanTextureView::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		m_DescriptorInfo.imageLayout = m_Image->GetLayout();

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = m_Texture->GetInfo().Usage & TextureUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}
