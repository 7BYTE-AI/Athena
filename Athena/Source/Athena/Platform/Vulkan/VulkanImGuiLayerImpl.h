#pragma once

#include "Athena/Core/Core.h"
#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct TextureInfo
	{
		VkImageView VulkanImageView = VK_NULL_HANDLE;
		VkSampler VulkanSampler = VK_NULL_HANDLE;
		VkDescriptorSet Set = VK_NULL_HANDLE;
	};
}


namespace std
{
	using namespace Athena;

	template<>
	struct hash<Ref<Texture2D>>
	{
		size_t operator()(const Ref<Texture2D>& value) const
		{
			return (size_t)value.Raw();
		}
	};

	template<>
	struct hash<Ref<TextureView>>
	{
		size_t operator()(const Ref<TextureView>& value) const
		{
			return (size_t)value.Raw();
		}
	};
}


namespace Athena
{
	class VulkanImGuiLayerImpl : public ImGuiLayerImpl
	{
	public:
		virtual void Init(void* windowHandle) override;
		virtual void Shutdown() override;

		virtual void NewFrame() override;
		virtual void RenderDrawData(uint32 width, uint32 height) override;

		virtual void OnSwapChainRecreate() override;

		virtual void* GetTextureID(const Ref<Texture2D>& texture) override;
		virtual void* GetTextureID(const Ref<TextureView>& texture) override;

	private:
		void RecreateFramebuffers();
		void InvalidateDescriptorSets();
		void RemoveDescriptorSet(VkDescriptorSet set);

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		std::unordered_map<Ref<Texture2D>, TextureInfo> m_TexturesMap;
		std::unordered_map<Ref<TextureView>, TextureInfo> m_TextureViewsMap;
	};
}
