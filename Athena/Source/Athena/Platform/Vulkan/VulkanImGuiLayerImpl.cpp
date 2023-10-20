#include "VulkanImGuiLayerImpl.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"

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

		VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetDevice()->GetLogicalDevice(), &pool_info, VulkanContext::GetAllocator(), &m_ImGuiDescriptorPool));

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
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			VK_CHECK(vkCreateRenderPass(VulkanContext::GetDevice()->GetLogicalDevice(), &renderPassInfo, VulkanContext::GetAllocator(), &m_ImGuiRenderPass));
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
		init_info.Allocator = VulkanContext::GetAllocator();
		init_info.CheckVkResultFn = [](VkResult result) { Utils::CheckVulkanResult(result); ATN_CORE_ASSERT(result == VK_SUCCESS) };

		ImGui_ImplVulkan_Init(&init_info, m_ImGuiRenderPass);

		Ref<CommandBuffer> commandBuffer = CommandBuffer::Create(CommandBufferUsage::IMMEDIATE);
		commandBuffer->Begin();
		{
			VkCommandBuffer vkCmdBuf = commandBuffer.As<VulkanCommandBuffer>()->GetVulkanCommandBuffer();
			ImGui_ImplVulkan_CreateFontsTexture(vkCmdBuf);
		}
		commandBuffer->End();
		commandBuffer->Flush();

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void VulkanImGuiLayerImpl::Shutdown()
	{
		Renderer::SubmitResourceFree([descPool = m_ImGuiDescriptorPool, renderPass = m_ImGuiRenderPass, framebuffers = m_SwapChainFramebuffers]()
			{
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), descPool, VulkanContext::GetAllocator());
				vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, VulkanContext::GetAllocator());

				for (auto framebuffer : framebuffers)
					vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());

				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();
			});
	}

	void VulkanImGuiLayerImpl::NewFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}

	void VulkanImGuiLayerImpl::RenderDrawData(uint32 width, uint32 height)
	{
		Ref<SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain();
		VkCommandBuffer commandBuffer = VulkanContext::GetActiveCommandBuffer();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_ImGuiRenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[swapChain->GetCurrentImageIndex()];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		VkClearValue clearColor = { { 0.f, 0.f, 0.f, 1.0f } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanImGuiLayerImpl::OnSwapChainRecreate()
	{
		RecreateFramebuffers();
	}

	void VulkanImGuiLayerImpl::RecreateFramebuffers()
	{
		if (!m_SwapChainFramebuffers.empty())
		{
			Renderer::SubmitResourceFree([framebuffers = m_SwapChainFramebuffers]()
				{
					for (auto framebuffer : framebuffers)
						vkDestroyFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
				});
		}
		else
		{
			m_SwapChainFramebuffers.resize(Renderer::GetFramesInFlight());
		}

		{
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

				VK_CHECK(vkCreateFramebuffer(VulkanContext::GetDevice()->GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &m_SwapChainFramebuffers[i]));
			}
		}
	}
}
