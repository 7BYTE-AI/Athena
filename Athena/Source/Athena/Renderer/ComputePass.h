#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	class RenderPass;
	class ComputePass;

	struct ComputePassCreateInfo
	{
		String Name;
		LinearColor DebugColor = LinearColor(0.f);
		Ref<RenderPass> InputRenderPass;
		Ref<ComputePass> InputComputePass;
	};

	class ATHENA_API ComputePass
	{
	public:
		static Ref<ComputePass> Create(const ComputePassCreateInfo& info);
		virtual ~ComputePass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void Bake() = 0;

		void SetOutput(const Ref<RenderResource>& resource);

		Ref<RenderResource> GetOutput(const String& name) const;
		Ref<RenderResource> GetOutput(uint32_t index) const;
		const std::vector<Ref<RenderResource>>& GetAllOutputs() const { return m_Outputs; }

		const ComputePassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		ComputePassCreateInfo m_Info;
		std::vector<Ref<RenderResource>> m_Outputs;
	};
}
