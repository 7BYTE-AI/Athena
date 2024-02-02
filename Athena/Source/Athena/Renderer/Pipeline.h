#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ShaderResource.h"


namespace Athena
{
	enum class Topology
	{
		TRIANGLE_LIST = 1,
		LINE_LIST
	};

	enum class CullMode
	{
		NONE = 0,
		BACK,
		FRONT
	};

	enum class DepthCompare
	{
		NONE = 0,
		LESS_OR_EQUAL
	};

	struct PipelineCreateInfo
	{
		String Name;
		Ref<RenderPass> RenderPass;
		Ref<Shader> Shader;
		Topology Topology = Topology::TRIANGLE_LIST;
		float LineWidth = 1.f;
		CullMode CullMode = CullMode::BACK;
		DepthCompare DepthCompare = DepthCompare::NONE;
		bool BlendEnable = true;
	};


	class ATHENA_API Pipeline : public RefCounted
	{
	public:
		static Ref<Pipeline> Create(const PipelineCreateInfo& info);
		virtual ~Pipeline() = default;
		
		virtual void Bind() = 0;
		virtual void SetViewport(uint32 width, uint32 height) = 0;

		virtual void SetInput(const String& name, Ref<ShaderResource> resource) = 0;
		virtual void Bake() = 0;

		const PipelineCreateInfo& GetInfo() const { return m_Info; }

	protected:
		PipelineCreateInfo m_Info;
	};
}
