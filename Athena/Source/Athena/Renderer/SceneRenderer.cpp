#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/DescriptorSetManager.h"


namespace Athena
{
	Ref<SceneRenderer> SceneRenderer::Create()
	{
		Ref<SceneRenderer> renderer = Ref<SceneRenderer>::Create();
		renderer->Init();

		return renderer;
	}

	SceneRenderer::~SceneRenderer()
	{
		Shutdown();
	}

	void SceneRenderer::Init()
	{
		const uint32 maxTimestamps = 16;
		const uint32 maxPipelineQueries = 1;
		m_Profiler = GPUProfiler::Create(maxTimestamps, maxPipelineQueries);

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));
		m_SceneUBO = UniformBuffer::Create("SceneUBO", sizeof(SceneData));
		m_LightSBO = StorageBuffer::Create("LightSBO", sizeof(LightData));

		// Geometry Pass
		{
			FramebufferCreateInfo fbInfo;
			fbInfo.Name = "GeometryFramebuffer";
			fbInfo.Attachments = { TextureFormat::RGBA16F, TextureFormat::DEPTH24STENCIL8 };
			fbInfo.Width = m_ViewportSize.x;
			fbInfo.Height = m_ViewportSize.y;
			fbInfo.Attachments[0].Name = "GeometryHDRColor";
			fbInfo.Attachments[0].ClearColor = { 0.9f, 0.3f, 0.4f, 1.0f };
			fbInfo.Attachments[1].Name = "GeometryDepth";
			fbInfo.Attachments[1].DepthClearColor = 1.f;
			fbInfo.Attachments[1].StencilClearColor = 1.f;

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Name = "GeometryPass";
			renderPassInfo.Output = Framebuffer::Create(fbInfo);
			renderPassInfo.LoadOpClear = RenderPassLoadOp::CLEAR;

			m_GeometryPass = RenderPass::Create(renderPassInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Static");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_StaticGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_StaticGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_StaticGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_StaticGeometryPipeline->Bake();
		}

		// Composite Pass
		{
			FramebufferCreateInfo fbInfo;
			fbInfo.Name = "SceneCompositeFramebuffer";
			fbInfo.Attachments = { TextureFormat::RGBA8 };
			fbInfo.Width = m_ViewportSize.x;
			fbInfo.Height = m_ViewportSize.y;
			fbInfo.Attachments[0].Name = "SceneCompositeColor";

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Name = "SceneCompositePass";
			renderPassInfo.Output = Framebuffer::Create(fbInfo);
			renderPassInfo.LoadOpClear = RenderPassLoadOp::DONT_CARE;

			m_CompositePass = RenderPass::Create(renderPassInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "SceneCompositePipeline";
			pipelineInfo.RenderPass = m_CompositePass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SceneComposite");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::NONE;
			pipelineInfo.BlendEnable = false;

			m_CompositePipeline = Pipeline::Create(pipelineInfo);

			m_CompositePipeline->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput(0));
			m_CompositePipeline->SetInput("u_SceneData", m_SceneUBO);
			m_CompositePipeline->Bake();
		}
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		return m_CompositePass->GetOutput(0);
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = { width, height };

		m_GeometryPass->GetOutput()->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);

		m_CompositePass->GetOutput()->Resize(width, height);
		m_CompositePipeline->SetViewport(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_CompositePipeline->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput(0));

		m_CameraData.View = cameraInfo.ViewMatrix;
		m_CameraData.Projection = cameraInfo.ProjectionMatrix;
		m_CameraData.Position = Math::AffineInverse(cameraInfo.ViewMatrix)[3];

		m_SceneData.Exposure = m_Settings.LightEnvironmentSettings.Exposure;
		m_SceneData.Gamma = m_Settings.LightEnvironmentSettings.Gamma;
	}

	void SceneRenderer::EndScene()
	{
		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		Renderer::Submit([this]()
		{
			m_CameraUBO->RT_SetData(&m_CameraData, sizeof(CameraData));
			m_SceneUBO->RT_SetData(&m_SceneData, sizeof(SceneData));
			m_LightSBO->RT_SetData(&m_LightData, sizeof(LightData));
		});

		GeometryPass();
		SceneCompositePass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();
		m_StaticGeometryList.clear();
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform)
	{
		if (!vertexBuffer)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit null vertexBuffer!");
			return;
		}

		DrawCall drawCall;
		drawCall.VertexBuffer = vertexBuffer;
		drawCall.Transform = transform;
		drawCall.Material = material;

		m_StaticGeometryList.push_back(drawCall);
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		if (lightEnv.DirectionalLights.size() > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
			return;
		}
		else if (lightEnv.PointLights.size() > ShaderDef::MAX_POINT_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);
			return;
		}

		m_LightData.DirectionalLightCount = lightEnv.DirectionalLights.size();
		for(uint32 i = 0; i < m_LightData.DirectionalLightCount; ++i)
			m_LightData.DirectionalLights[i] = lightEnv.DirectionalLights[i];

		m_LightData.PointLightCount = lightEnv.PointLights.size();
		for (uint32 i = 0; i < m_LightData.PointLightCount; ++i)
			m_LightData.PointLights[i] = lightEnv.PointLights[i];

		m_SceneData.EnvironmentIntensity = lightEnv.EnvironmentMapIntensity;
		m_SceneData.EnvironmentLOD = lightEnv.EnvironmentMapLOD;
	}

	void SceneRenderer::GeometryPass()
	{
		m_Profiler->BeginTimeQuery();

		m_GeometryPass->Begin();
		{
			m_StaticGeometryPipeline->Bind();

			for (const auto& drawCall : m_StaticGeometryList)
			{
				drawCall.Material->Bind();
				drawCall.Material->Set("u_Transform", drawCall.Transform);
				Renderer::RenderMeshWithMaterial(drawCall.VertexBuffer, drawCall.Material);
			}
		}
		m_GeometryPass->End();

		m_Statistics.GeometryPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::SceneCompositePass()
	{
		m_Profiler->BeginTimeQuery();

		m_CompositePass->Begin();
		{
			m_CompositePipeline->Bind();
			Renderer::RenderFullscreenQuad();
		}
		m_CompositePass->End();

		m_Statistics.SceneCompositePass = m_Profiler->EndTimeQuery();
	}
}
