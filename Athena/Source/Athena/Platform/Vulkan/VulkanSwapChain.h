#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SwapChain.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanSwapChain : public SwapChain
	{
	public:
		VulkanSwapChain(void* windowHandle, bool vsync = false);
		~VulkanSwapChain();

		void CleanUp(VkSwapchainKHR swapChain, bool cleanupSurface = false);

		virtual void OnWindowResize() override;
		virtual bool Recreate() override;
		virtual void SetVSync(bool enabled) override;

		virtual void AcquireImage() override;
		virtual void Present() override;

		virtual uint32 GetCurrentImageIndex() override { return m_ImageIndex; }

		VkImage GetCurrentVulkanImage();

		const std::vector<VkImageView>& GetVulkanImageViews() const { return m_SwapChainImageViews; }
		VkFormat GetFormat() const { return m_Format.format; }

	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_VkSwapChain;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		VkPresentModeKHR m_PresentMode;
		VkSurfaceFormatKHR m_Format;

		uint32 m_ImageIndex;
		bool m_VSync;
		bool m_Dirty;
	};
}
