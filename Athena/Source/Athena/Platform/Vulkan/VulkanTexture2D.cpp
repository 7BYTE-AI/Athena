#include "VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTexture2D::VulkanTexture2D(const Texture2DCreateInfo& info)
	{
		m_Info = info;
		m_Sampler = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		if (info.MipLevels == 0)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		ImageCreateInfo imageInfo;
		imageInfo.Name = m_Info.Name;
		imageInfo.Format = m_Info.Format;
		imageInfo.Usage = m_Info.Usage;
		imageInfo.Type = ImageType::IMAGE_2D;
		imageInfo.InitialData = m_Info.InitialData;
		imageInfo.Width = m_Info.Width;
		imageInfo.Height = m_Info.Height;
		imageInfo.Layers = m_Info.Layers;
		imageInfo.MipLevels = m_Info.MipLevels;

		m_Image = Image::Create(imageInfo);

		SetSampler(m_Info.SamplerInfo);
	}

	VulkanTexture2D::VulkanTexture2D(const Ref<Image>& image, const TextureSamplerCreateInfo& samplerInfo)
	{
		const ImageCreateInfo& imageInfo = image->GetInfo();

		m_Info.Name = imageInfo.Name;
		m_Info.Format = imageInfo.Format;
		m_Info.Usage = imageInfo.Usage;
		m_Info.InitialData = imageInfo.InitialData;
		m_Info.Width = imageInfo.Width;
		m_Info.Height = imageInfo.Height;
		m_Info.Layers = imageInfo.Layers;
		m_Info.MipLevels = imageInfo.MipLevels;
		m_Info.SamplerInfo = samplerInfo;

		m_Image = image;

		m_Sampler = VK_NULL_HANDLE;

		m_DescriptorInfo.imageLayout = m_Info.Usage & ImageUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		SetSampler(m_Info.SamplerInfo);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([vkSampler = m_Sampler]()
		{
			vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
		});
	}

	void VulkanTexture2D::Resize(uint32 width, uint32 height)
	{
		if (m_Info.Width == width && m_Info.Height == height)
			return;

		m_Info.Width = width;
		m_Info.Height = height;

		m_Image->Resize(width, height);
	}

	void VulkanTexture2D::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_Sampler]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		Renderer::Submit([this]()
		{
			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = Vulkan::GetFilter(m_Info.SamplerInfo.MagFilter);
			vksamplerInfo.minFilter = Vulkan::GetFilter(m_Info.SamplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = Vulkan::GetMipMapMode(m_Info.SamplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeV = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeW = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.f;
			vksamplerInfo.compareEnable = m_Info.SamplerInfo.Compare == TextureCompareOperator::NONE ? false : true;
			vksamplerInfo.compareOp = Vulkan::GetCompareOp(m_Info.SamplerInfo.Compare);
			vksamplerInfo.minLod = 0.f;
			vksamplerInfo.maxLod = m_Info.MipLevels;
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &m_Sampler));
			Vulkan::SetObjectDebugName(m_Sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, std::format("Sampler_{}", m_Info.Name));

			m_DescriptorInfo.sampler = m_Sampler;
		});
	}

	VkImage VulkanTexture2D::GetVulkanImage() const 
	{
		return m_Image.As<VulkanImage>()->GetVulkanImage();
	}

	VkImageView VulkanTexture2D::GetVulkanImageView() const
	{ 
		return m_Image.As<VulkanImage>()->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTexture2D::GetVulkanDescriptorInfo(uint32 mip)
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		m_DescriptorInfo.imageLayout = m_Image.As<VulkanImage>()->GetLayout();

		if (mip != 0)
			m_DescriptorInfo.imageView = m_Image.As<VulkanImage>()->GetVulkanImageViewMip(mip);

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = m_Info.Usage & ImageUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}
