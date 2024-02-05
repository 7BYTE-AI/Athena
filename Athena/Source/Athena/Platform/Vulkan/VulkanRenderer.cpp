#include "VulkanRenderer.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	void VulkanRenderer::Init()
	{
		VulkanContext::Init();

		// Create Command buffers
		{
			VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
			cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
			cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufAllocInfo.commandBufferCount = Renderer::GetFramesInFlight();

			m_VkCommandBuffers.resize(Renderer::GetFramesInFlight());
			VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, m_VkCommandBuffers.data()));
		}
	}

	void VulkanRenderer::Shutdown()
	{
		Renderer::SubmitResourceFree([vkCommandBuffers = m_VkCommandBuffers]()
		{
			vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), VulkanContext::GetCommandPool(), vkCommandBuffers.size(), vkCommandBuffers.data());
			VulkanContext::Shutdown();
		});
	}

	void VulkanRenderer::OnUpdate()
	{
		VulkanContext::GetAllocator()->OnUpdate();
	}

	void VulkanRenderer::RenderGeometry(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<VertexBuffer>& mesh, const Ref<Material>& material)
	{
		Renderer::Submit([commandBuffer, mesh, material]()
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			if(material)
				material->RT_UpdateForRendering(commandBuffer);
			
			Ref<VulkanVertexBuffer> vkVertexBuffer = mesh.As<VulkanVertexBuffer>();
			VkBuffer vertexBuffer = vkVertexBuffer->GetVulkanVertexBuffer();

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(vkcmdBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(vkcmdBuffer, vkVertexBuffer->GetVulkanIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(vkcmdBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::WaitDeviceIdle()
	{
		vkDeviceWaitIdle(VulkanContext::GetDevice()->GetLogicalDevice());
	}

	void VulkanRenderer::BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image)
	{
		Renderer::Submit([cmdBuffer, image]()
		{
			VkCommandBuffer commandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			VkImage sourceImage = image.As<VulkanImage>()->GetVulkanImage();
			VkImage swapChainImage = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>()->GetCurrentVulkanImage();

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			{
				barrier.image = sourceImage;
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				barrier.image = swapChainImage;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_NONE;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}

			VkImageBlit imageBlitRegion = {};
			imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.srcSubresource.layerCount = 1;
			imageBlitRegion.srcOffsets[1] = { (int)image->GetInfo().Width, (int)image->GetInfo().Height, 1 };
			imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.dstSubresource.layerCount = 1;
			imageBlitRegion.dstOffsets[1] = { (int)image->GetInfo().Width, (int)image->GetInfo().Height, 1 };

			vkCmdBlitImage(
				commandBuffer,
				sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlitRegion,
				VK_FILTER_NEAREST);


			{
				barrier.image = sourceImage;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_NONE;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);


				barrier.image = swapChainImage;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_NONE;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}
		});
	}

	void VulkanRenderer::GetRenderCapabilities(RenderCapabilities& caps)
	{
		VulkanContext::GetDevice()->GetDeviceCapabilities(caps);
	}

	uint64 VulkanRenderer::GetMemoryUsage()
	{
		return VulkanContext::GetAllocator()->GetMemoryUsage();
	}
}
