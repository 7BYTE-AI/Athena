#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Platform/Vulkan/DescriptorSetManager.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineCreateInfo& info);
		~VulkanPipeline();

		virtual bool Bind(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void SetViewport(uint32 width, uint32 height) override;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) override;
		virtual void Bake() override;

	private:
		void CleanUp();
		void RecreatePipeline();

	private:
		DescriptorSetManager m_DescriptorSetManager;
		VkPipeline m_VulkanPipeline;
		Vector2u m_ViewportSize;
		uint64 m_Hash;
	};
}
