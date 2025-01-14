#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

#include <vma/vk_mem_alloc.h>

#include "VulkanAllocator.h"
#include "Athena/Utils/StringUtils.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	void* VulkanBufferAllocation::MapMemory()
	{
		void* mappedData;
		VK_CHECK(vmaMapMemory(VulkanContext::GetAllocator()->GetInternalAllocator(), m_Allocation, &mappedData));
		return mappedData;
	}

	void VulkanBufferAllocation::UnmapMemory()
	{
		vmaUnmapMemory(VulkanContext::GetAllocator()->GetInternalAllocator(), m_Allocation);
	}

	VulkanAllocator::VulkanAllocator(uint32 vulkanVersion)
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = vulkanVersion;
		allocatorCreateInfo.physicalDevice = VulkanContext::GetPhysicalDevice();
		allocatorCreateInfo.device = VulkanContext::GetLogicalDevice();
		allocatorCreateInfo.instance = VulkanContext::GetInstance();
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator));

		vkGetPhysicalDeviceMemoryProperties(VulkanContext::GetPhysicalDevice(), &m_MemoryProps);
	}

	VulkanAllocator::~VulkanAllocator()
	{
		vmaDestroyAllocator(m_Allocator);
	}

	VulkanBufferAllocation VulkanAllocator::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage, VmaAllocationCreateFlagBits flags, const String& name)
	{
		VkBuffer buffer;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));

		VkDeviceSize size = allocation->GetSize();
		if(!name.empty())
			ATN_CORE_TRACE_TAG("Renderer", "Allocating buffer '{}' {}", name, Utils::MemoryBytesToString(size));

		return VulkanBufferAllocation(buffer, allocation);
	}

	VulkanImageAllocation VulkanAllocator::AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage, VmaAllocationCreateFlagBits flags, const String& name)
	{
		VkImage image;
		VmaAllocation allocation;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.flags = flags;

		VK_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr));

		VkDeviceSize size = allocation->GetSize();
		ATN_CORE_TRACE_TAG("Renderer", "Allocating image '{}' {}", name, Utils::MemoryBytesToString(size));

		return VulkanImageAllocation(image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VulkanBufferAllocation buffer, const String& name)
	{
		VkDeviceSize size = buffer.GetAllocation()->GetSize();
		if(!name.empty())
			ATN_CORE_TRACE_TAG("Renderer", "Destroying buffer '{}' {}", name, Utils::MemoryBytesToString(size));

		vmaDestroyBuffer(m_Allocator, buffer.GetBuffer(), buffer.GetAllocation());
	}

	void VulkanAllocator::DestroyImage(VulkanImageAllocation image, const String& name)
	{
		VkDeviceSize size = image.GetAllocation()->GetSize();
		ATN_CORE_TRACE_TAG("Renderer", "Destroying image '{}' {}", name, Utils::MemoryBytesToString(size));

		vmaDestroyImage(m_Allocator, image.GetImage(), image.GetAllocation());
	}

	VkSampler VulkanAllocator::CreateSampler(const TextureSamplerCreateInfo& info)
	{
		if (m_SamplersMap.contains(info))
		{
			auto& samplerInfo = m_SamplersMap.at(info);
			samplerInfo.RefCount++;
			return samplerInfo.Sampler;
		}

		VkSampler sampler;

		bool enableAnisotropy = Renderer::GetRenderCaps().MaxSamplerAnisotropy != 0.f;
		enableAnisotropy = enableAnisotropy && info.Filter == TextureFilter::LINEAR;

		// Clamp to 2.f for now
		float maxAnisotropy = Math::Min(2.f, Renderer::GetRenderCaps().MaxSamplerAnisotropy);

		VkSamplerCreateInfo vksamplerInfo = {};
		vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		vksamplerInfo.magFilter = Vulkan::GetFilter(info.Filter);
		vksamplerInfo.minFilter = Vulkan::GetFilter(info.Filter);
		vksamplerInfo.mipmapMode = Vulkan::GetMipMapMode(info.Filter);
		vksamplerInfo.addressModeU = Vulkan::GetWrap(info.Wrap);
		vksamplerInfo.addressModeV = Vulkan::GetWrap(info.Wrap);
		vksamplerInfo.addressModeW = Vulkan::GetWrap(info.Wrap);
		vksamplerInfo.anisotropyEnable = enableAnisotropy;
		vksamplerInfo.maxAnisotropy = maxAnisotropy;
		vksamplerInfo.compareEnable = info.Compare == TextureCompareOperator::NONE ? false : true;
		vksamplerInfo.compareOp = Vulkan::GetCompareOp(info.Compare);
		vksamplerInfo.minLod = 0;
		vksamplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		vksamplerInfo.mipLodBias = 0.f;
		vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		vksamplerInfo.unnormalizedCoordinates = false;

		VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &sampler));

		m_SamplersMap[info] = { sampler, 1 };
		return sampler;
	}

	void VulkanAllocator::DestroySampler(const TextureSamplerCreateInfo& info, VkSampler sampler)
	{
		ATN_CORE_ASSERT(m_SamplersMap.contains(info));

		auto& samplerInfo = m_SamplersMap.at(info);
		ATN_CORE_ASSERT(samplerInfo.Sampler == sampler);

		samplerInfo.RefCount--;

		if (samplerInfo.RefCount == 0)
		{
			vkDestroySampler(VulkanContext::GetLogicalDevice(), samplerInfo.Sampler, nullptr);
			m_SamplersMap.erase(info);
		}
	}

	void VulkanAllocator::OnUpdate()
	{
		vmaSetCurrentFrameIndex(m_Allocator, Renderer::GetCurrentFrameIndex());
	}

	uint64 VulkanAllocator::GetMemoryUsage()
	{
		VmaBudget heaps[VK_MAX_MEMORY_HEAPS] = { 0 };
		vmaGetHeapBudgets(m_Allocator, heaps);

		uint64 result = 0;
		for (uint32 i = 0; i < m_MemoryProps.memoryHeapCount; i++)
		{
			if(m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				result += heaps[i].usage;
		}

		return result;
	}


	DescriptorSetAllocator::DescriptorSetAllocator()
	{
		m_CurrentPool = VK_NULL_HANDLE;
	}

	DescriptorSetAllocator::~DescriptorSetAllocator()
	{
		Renderer::SubmitResourceFree([freePools = m_FreePools, usedPools = m_UsedPools]()
		{
			for (auto pool : freePools)
			{
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), pool, nullptr);
			}

			for (auto pool : usedPools)
			{
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), pool, nullptr);
			}
		});
	}

	void DescriptorSetAllocator::ResetPools()
	{
		for (auto p : m_UsedPools) 
		{
			vkResetDescriptorPool(VulkanContext::GetLogicalDevice(), p, 0);
			ATN_CORE_WARN_TAG("Vulkan", "Reseting Descriptor Pool");

			m_FreePools.push_back(p);
		}

		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	}

	bool DescriptorSetAllocator::Allocate(VkDescriptorSet* sets, VkDescriptorSetAllocateInfo allocInfo)
	{
		if (m_CurrentPool == VK_NULL_HANDLE) 
		{
			m_CurrentPool = GrabPool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		allocInfo.descriptorPool = m_CurrentPool;

		VkResult allocResult = vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, sets);
		bool needReallocate = false;

		switch (allocResult) 
		{
		case VK_SUCCESS:
			return true;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			needReallocate = true;
			break;
		default:
			ATN_CORE_ASSERT(false);
			return false;
		}

		if (needReallocate) 
		{
			m_CurrentPool = GrabPool();
			m_UsedPools.push_back(m_CurrentPool);

			allocInfo.descriptorPool = m_CurrentPool;

			allocResult = vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, sets);

			if (allocResult == VK_SUCCESS) 
				return true;
		}
		
		ATN_CORE_ERROR_TAG("Vulkan", "Failed to allocate descriptor set!");
		ATN_CORE_ASSERT(false);

		return false;
	}

	VkDescriptorPool DescriptorSetAllocator::CreatePool(uint32 count, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(s_PoolSizes.size());

		for (auto [type, size] : s_PoolSizes)
		{
			if(size > 0.f)
				sizes.push_back({ type, uint32(size * count) });
		}
		
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = flags;
		poolInfo.maxSets = count;
		poolInfo.poolSizeCount = sizes.size();
		poolInfo.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool;
		vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool);

		ATN_CORE_WARN_TAG("Vulkan", "Allocating Descriptor Pool");

		return descriptorPool;
	}

	VkDescriptorPool DescriptorSetAllocator::GrabPool()
	{
		if (m_FreePools.size() > 0)
		{
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else
		{
			return CreatePool(1000, 0);
		}
	}
}
