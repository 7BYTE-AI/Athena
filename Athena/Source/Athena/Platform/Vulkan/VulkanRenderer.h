#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RendererAPI.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanRenderer: public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void OnUpdate() override;

		virtual void RenderGeometryInstanced(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 instanceCount, uint32 firstInstance) override;
		virtual void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 offset, uint32 count) override;
		virtual void BindInstanceRateBuffer(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<VertexBuffer> vertexBuffer) override;

		virtual void Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material) override;
		virtual void InsertMemoryBarrier(const Ref<RenderCommandBuffer>& cmdBuffer) override;
		virtual void InsertExecutionBarrier(const Ref<RenderCommandBuffer>& cmdBuffer) override;

		virtual void BeginDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) override;
		virtual void EndDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer) override;
		virtual void InsertDebugMarker(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) override;

		virtual void BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture>& texture) override;
		virtual void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture2D>& texture) override;

		virtual void GetRenderCapabilities(RenderCapabilities& caps) override;
		virtual uint64 GetMemoryUsage() override;
		virtual void WaitDeviceIdle() override;

	private:
		PFN_vkCmdDebugMarkerBeginEXT m_DebugMarkerBeginPFN;
		PFN_vkCmdDebugMarkerEndEXT m_DebugMarkerEndPFN;
		PFN_vkCmdDebugMarkerInsertEXT m_DebugMarkerInsertPFN;
	};
}
