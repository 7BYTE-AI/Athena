#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace Athena
{
	class VulkanBufferAllocation
	{
	public:
		VulkanBufferAllocation() = default;

		VulkanBufferAllocation(VkBuffer buffer, VmaAllocation alloc)
			: m_Buffer(buffer), m_Allocation(alloc)
		{}

		VkBuffer GetBuffer() const { return m_Buffer; }
		VmaAllocation GetAllocation() const { return m_Allocation; }

		void* MapMemory();
		void UnmapMemory();

		operator bool() const { return !(m_Buffer == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE); }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};
	
	class VulkanImageAllocation
	{
	public:
		VulkanImageAllocation() = default;

		VulkanImageAllocation(VkImage image, VmaAllocation alloc)
			: m_Image(image), m_Allocation(alloc)
		{}

		VkImage GetImage() const { return m_Image; }
		VmaAllocation GetAllocation() const { return m_Allocation; }

		operator bool() const { return !(m_Image == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE); }

	private:
		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};

	struct VulkanSamplerAllocation
	{
		VkSampler Sampler = VK_NULL_HANDLE;
		uint64 RefCount = 0;
	};


	class VulkanAllocator
	{
	public:
		VulkanAllocator(uint32 vulkanVersion);
		~VulkanAllocator();

		VulkanBufferAllocation AllocateBuffer(const VkBufferCreateInfo& bufferInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0, const String& name = "");
		VulkanImageAllocation AllocateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits flags = (VmaAllocationCreateFlagBits)0, const String& name = "");

		void DestroyBuffer(VulkanBufferAllocation buffer, const String& name = "");
		void DestroyImage(VulkanImageAllocation image, const String& name = "");

		VmaAllocator GetInternalAllocator() { return m_Allocator; }

		VkSampler CreateSampler(const TextureSamplerCreateInfo& info);
		void DestroySampler(const TextureSamplerCreateInfo& info, VkSampler sampler);

		void OnUpdate();

		// return in Kb
		uint64 GetMemoryUsage();

	private:
		VmaAllocator m_Allocator;
		VkPhysicalDeviceMemoryProperties m_MemoryProps;
		std::unordered_map<TextureSamplerCreateInfo, VulkanSamplerAllocation> m_SamplersMap;
	};

	class DescriptorSetAllocator
	{
	public:
		inline static std::vector<std::pair<VkDescriptorType, float>> s_PoolSizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1.f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.f }
		};

	public:
		DescriptorSetAllocator();
		~DescriptorSetAllocator();

		void ResetPools();
		bool Allocate(VkDescriptorSet* sets, VkDescriptorSetAllocateInfo allocInfo);

	private:
		VkDescriptorPool CreatePool(uint32 count, VkDescriptorPoolCreateFlags flags);
		VkDescriptorPool GrabPool();

	private:
		VkDescriptorPool m_CurrentPool;
		std::vector<VkDescriptorPool> m_UsedPools;
		std::vector<VkDescriptorPool> m_FreePools;
	};
}
