#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Renderer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		VkDevice GetLogicalDevice() { return m_LogicalDevice; }
		uint32 GetQueueFamily() { return m_QueueFamily; }
		VkQueue GetQueue() { return m_Queue; }

		void GetDeviceCapabilities(RenderCapabilities& deviceCaps) const;

	private:
		bool CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions);

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;
		uint32 m_QueueFamily;
		VkQueue m_Queue;
	};
}
