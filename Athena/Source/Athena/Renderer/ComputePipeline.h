#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/RenderResource.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	class ATHENA_API ComputePipeline
	{
	public:
		static Ref<ComputePipeline> Create(const Ref<Shader>& shader);
		virtual ~ComputePipeline() = default;
		
		virtual bool Bind(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void SetInput(const String& name, const Ref<RenderResource>& resource) = 0;
		virtual void Bake() = 0;

		Ref<Shader> GetShader() const { return m_Shader; }
		const String& GetName() const { return m_Name; }

	protected:
		Ref<Shader> m_Shader;
		String m_Name;
	};
}
