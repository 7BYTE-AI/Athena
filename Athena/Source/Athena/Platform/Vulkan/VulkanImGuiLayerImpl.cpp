#include "VulkanImGuiLayerImpl.h"

#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureView.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Renderer/TextureGenerator.h"

#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	void VulkanImGuiLayerImpl::Init(void* windowHandle)
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetDevice()->GetLogicalDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool));

		// Create Render Pass
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>()->GetFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetDevice()->GetLogicalDevice(), &renderPassInfo, nullptr, &m_ImGuiRenderPass));
			Vulkan::SetObjectDebugName(m_ImGuiRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, "ImGuiRenderPass");
		}

		RecreateFramebuffers();

		ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanContext::GetInstance();
		init_info.PhysicalDevice = VulkanContext::GetDevice()->GetPhysicalDevice();
		init_info.Device = VulkanContext::GetDevice()->GetLogicalDevice();
		init_info.QueueFamily = VulkanContext::GetDevice()->GetQueueFamily();
		init_info.Queue = VulkanContext::GetDevice()->GetQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_ImGuiDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = Renderer::GetFramesInFlight();
		init_info.ImageCount = Renderer::GetFramesInFlight();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = [](VkResult result) { Vulkan::CheckResult(result); ATN_CORE_ASSERT(result == VK_SUCCESS) };

		ImGui_ImplVulkan_Init(&init_info, m_ImGuiRenderPass);

		VkCommandBuffer vkCommandBuffer = Vulkan::BeginSingleTimeCommands();
		{
			ImGui_ImplVulkan_CreateFontsTexture(vkCommandBuffer);
		}
		Vulkan::EndSingleTimeCommands(vkCommandBuffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void VulkanImGuiLayerImpl::Shutdown()
	{
		m_TexturesMap.clear();
		m_TextureViewsMap.clear();

		Renderer::SubmitResourceFree([descPool = m_ImGuiDescriptorPool, renderPass = m_ImGuiRenderPass, 
			framebuffers = m_SwapChainFramebuffers]()
		{
			vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), descPool, nullptr);
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, nullptr);

			for (auto framebuffer : framebuffers)
				vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, nullptr);

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void VulkanImGuiLayerImpl::NewFrame()
	{
		ATN_PROFILE_FUNC();

		InvalidateDescriptorSets();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void VulkanImGuiLayerImpl::RenderDrawData(uint32 width, uint32 height)
	{
		ATN_PROFILE_FUNC()

		Ref<SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain();
		VkCommandBuffer commandBuffer = Renderer::GetRenderCommandBuffer().As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_ImGuiRenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[swapChain->GetCurrentImageIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { { 0.f, 0.f, 0.f, 1.0f } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		Renderer::BeginDebugRegion(Renderer::GetRenderCommandBuffer(), "UIOverlayPass", { 0.9f, 0.1f, 0.2f, 1.f });
		{
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

			vkCmdEndRenderPass(commandBuffer);
		}
		Renderer::EndDebugRegion(Renderer::GetRenderCommandBuffer());
	}

	void VulkanImGuiLayerImpl::OnSwapChainRecreate()
	{
		ATN_PROFILE_FUNC();
		RecreateFramebuffers();
	}

	void VulkanImGuiLayerImpl::RecreateFramebuffers()
	{
		Renderer::SubmitResourceFree([framebuffers = m_SwapChainFramebuffers]()
		{
			for (auto framebuffer : framebuffers)
				vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, nullptr);
		});

		m_SwapChainFramebuffers.resize(Renderer::GetFramesInFlight());

		if (!m_ImGuiRenderPass)
			return;

		Ref<VulkanSwapChain> vkSwapChain = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>();;

		for (size_t i = 0; i < vkSwapChain->GetVulkanImageViews().size(); i++)
		{
			VkImageView attachment = vkSwapChain->GetVulkanImageViews()[i];

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_ImGuiRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &attachment;
			framebufferInfo.width = Application::Get().GetWindow().GetWidth();
			framebufferInfo.height = Application::Get().GetWindow().GetHeight();
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]));
		}
	}

	void* VulkanImGuiLayerImpl::GetTextureID(const Ref<TextureView>& view)
	{
		if (view == nullptr)
			return GetTextureID(TextureGenerator::GetWhiteTexture());

		if (m_TextureViewsMap.contains(view))
			return m_TextureViewsMap.at(view).Set;

		TextureInfo info;
		info.VulkanImageView = view.As<VulkanTextureView>()->GetVulkanImageView();
		info.VulkanSampler = view.As<VulkanTextureView>()->GetVulkanSampler();

		if (info.VulkanImageView == VK_NULL_HANDLE)
			return GetTextureID(TextureGenerator::GetWhiteTexture());

		info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_TextureViewsMap[view] = info;
		return info.Set;
	}

	void* VulkanImGuiLayerImpl::GetTextureID(const Ref<Texture2D>& texture)
	{
		if (texture == nullptr)
			return GetTextureID(TextureGenerator::GetWhiteTexture());

		if (m_TexturesMap.contains(texture))
			return m_TexturesMap.at(texture).Set;

		TextureInfo info;
		info.VulkanImageView = texture.As<VulkanTexture2D>()->GetVulkanImageView();
		info.VulkanSampler = texture.As<VulkanTexture2D>()->GetVulkanSampler();

		if (info.VulkanImageView == VK_NULL_HANDLE)
			return GetTextureID(TextureGenerator::GetWhiteTexture());

		info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_TexturesMap[texture] = info;
		return info.Set;
	}

	void VulkanImGuiLayerImpl::InvalidateDescriptorSets()
	{
		std::vector<Ref<TextureView>> viewsToRemove;
		for (auto& [view, info] : m_TextureViewsMap)
		{
			VkImageView imageView = view.As<VulkanTextureView>()->GetVulkanImageView();

			// All instances of texture has been deleted except one in texture map
			if (view.GetCount() == 1 || imageView == VK_NULL_HANDLE)
			{
				RemoveDescriptorSet(info.Set);
				viewsToRemove.push_back(view);
				continue;
			}

			VkSampler sampler = view.As<VulkanTextureView>()->GetVulkanSampler();

			// Check if texture has been modified and update descriptor set
			if (info.VulkanImageView != imageView || info.VulkanSampler != sampler)
			{
				RemoveDescriptorSet(info.Set);

				info.VulkanImageView = imageView;
				info.VulkanSampler = sampler;
				info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		for (const auto& view : viewsToRemove)
			m_TextureViewsMap.erase(view);


		std::vector<Ref<Texture2D>> texturesToRemove;
		for (auto& [texture, info] : m_TexturesMap)
		{
			VkImageView imageView = texture.As<VulkanTexture2D>()->GetVulkanImageView();

			// All instances of texture has been deleted except one in texture map
			if (texture.GetCount() == 1 || imageView == VK_NULL_HANDLE)
			{
				RemoveDescriptorSet(info.Set);
				texturesToRemove.push_back(texture);
				continue;
			}

			VkSampler sampler = texture.As<VulkanTexture2D>()->GetVulkanSampler();

			// Check if texture has been modified and update descriptor set
			if (info.VulkanImageView != imageView || info.VulkanSampler != sampler)
			{
				RemoveDescriptorSet(info.Set);

				info.VulkanImageView = imageView;
				info.VulkanSampler = sampler;
				info.Set = ImGui_ImplVulkan_AddTexture(info.VulkanSampler, info.VulkanImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		for (const auto& texture : texturesToRemove)
			m_TexturesMap.erase(texture);

	}

	void VulkanImGuiLayerImpl::RemoveDescriptorSet(VkDescriptorSet set)
	{
		Renderer::SubmitResourceFree([set = set]()
		{
			ImGui_ImplVulkan_RemoveTexture(set);
		});
	}
}
