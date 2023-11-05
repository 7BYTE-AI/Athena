#include "VulkanUniformBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanUniformBuffer::VulkanUniformBuffer(uint32 size)
	{
		m_VulkanUBOSet.resize(Renderer::GetFramesInFlight());

		Renderer::Submit([this, size]()
		{
			for (auto& ubo : m_VulkanUBOSet)
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

				ubo = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
			}
		});

	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		Renderer::SubmitResourceFree([vulkanUBOSet = m_VulkanUBOSet]()
		{
			for (auto ubo: vulkanUBOSet)
			{
				VulkanContext::GetAllocator()->DestroyBuffer(ubo);
			}
		});
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32 size, uint32 offset)
	{
		auto& ubo = m_VulkanUBOSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory = ubo.MapMemory();
		memcpy(mappedMemory, data, size);
		ubo.UnmapMemory();
	}
}
