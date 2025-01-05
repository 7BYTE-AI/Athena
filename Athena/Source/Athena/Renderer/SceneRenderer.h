#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/GPUProfiler.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Light.h"
#include "Athena/Renderer/DrawList.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/SceneRenderer2D.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	enum class Antialising
	{
		NONE = 0,
		FXAA,
		SMAA
	};

	enum class DebugView
	{
		NONE = 0,
		SHADOW_CASCADES,
		LIGHT_COMPLEXITY,
		GBUFFER,
	};

	enum class TonemapMode
	{
		NONE = 0,
		ACES_FILMIC,
		ACES_TRUE,
	};

	struct PostProcessingSettings
	{
		TonemapMode TonemapMode = TonemapMode::ACES_FILMIC;
		float Exposure = 1.6f;
		Antialising AntialisingMethod = Antialising::SMAA;
	};

	struct ShadowSettings
	{
		bool SoftShadows = true;
		float MaxDistance = 200.f;
		float FadeOut = 15.f;
		float BiasGradient = 1.f;
		float CascadeBlendDistance = 0.5f;
		float CascadeSplit = 0.91f;
		float NearPlaneOffset = -70.f;
		float FarPlaneOffset = 15.f;
	};

	struct BloomSettings
	{
		bool Enable = true;
		float Intensity = 0.8f;
		float Threshold = 1.5f;
		float Knee = 0.1f;
		float DirtIntensity = 2.f;
		Ref<Texture2D> DirtTexture;
	};

	struct AmbientOcclusionSettings
	{
		bool Enable = true;
		float Intensity = 1.3f;
		float Radius = 1.f;
		float Bias = 0.1f;
		float BlurSharpness = 12.f;
	};

	struct SSRSettings
	{
		bool Enable = false;
		bool HalfRes = false;
		bool ConeTrace = false;
		float Intensity = 1.f;
		uint32 MaxSteps = 128;
		float MaxRoughness = 0.5f;
		float ScreenEdgesFade = 0.1f;
		bool BackwardRays = true;
	};

	struct QualitySettings
	{
		float RendererScale = 1.f;
	};

	struct SceneRendererSettings
	{
		ShadowSettings ShadowSettings;
		BloomSettings BloomSettings;
		AmbientOcclusionSettings AOSettings;
		SSRSettings SSRSettings;
		PostProcessingSettings PostProcessingSettings;
		DebugView DebugView = DebugView::NONE;
		QualitySettings Quality;
	};


	struct CameraData
	{
		Matrix4 View;
		Matrix4 InverseView;
		Matrix4 Projection;
		Matrix4 InverseProjection;
		Matrix4 ViewProjection;
		Matrix4 InverseViewProjection;
		Vector3 Position;
		float NearClip;
		float FarClip;
		float FOV;
		Vector2 _Pad0;
		Vector4 ProjInfo;
	};

	struct SceneRendererData
	{
		Vector2 ViewportSize;
		Vector2 InverseViewportSize;
		Vector4i ViewportTilesCount;	// xy - per width and height, z - all tiles, w - empty
		float EnvironmentIntensity;
		float EnvironmentLOD;
		int32 DebugShadowCascades;
		int32 DebugLightComplexity;
	};

	struct LightData
	{
		DirectionalLight DirectionalLights[ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLights[ShaderDef::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;

		SpotLight SpotLights[ShaderDef::MAX_SPOT_LIGHT_COUNT];
		uint32 SpotLightCount = 0;
	};

	struct TileVisibleLights
	{
		uint32 Count;
		uint32 LightIndices[ShaderDef::MAX_POINT_LIGHT_COUNT_PER_TILE];
	};

	struct ShadowsData
	{
		Matrix4 DirLightViewProjection[ShaderDef::SHADOW_CASCADES_COUNT];
		Vector4 CascadePlanes[ShaderDef::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float CascadeBlendDistance = 0.5f;
		float BiasGradient = 1.f;
		int SoftShadows = true;
		Vector3 _Pad0;
	};

	struct HBAOData
	{
		Vector4 Float2Offsets[16];
		Vector4 Jitters[16];
		Vector2 InvResolution;
		Vector2 InvQuarterResolution;

		Vector4 ProjInfo;
		float NegInvR2;
		float RadiusToScreen;
		float AOMultiplier;

		float Intensity;
		float Bias;
		float BlurSharpness;
		Vector2 _Pad0;
	};

	struct SSRData
	{
		float Intensity;
		uint32 MaxSteps;
		float MaxRoughness;
		float ScreenEdgesFade;
		uint32 ConeTrace;
		uint32 BackwardRays;
		Vector2 _Pad0;
	};

	struct SceneRendererStatistics
	{
		Time GPUTime;
		Time DirShadowMapPass;
		Time GBufferPass;
		Time HiZPass;
		Time LightCullingPass;
		Time HBAODeinterleavePass;
		Time HBAOComputePass;
		Time HBAOBlurPass;
		Time DeferredLightingPass;
		Time SkyboxPass;
		Time PreConvolutionPass;
		Time SSRComputePass;
		Time SSRCompositePass;
		Time BloomPass;
		Time SceneCompositePass;
		Time JumpFloodPass;
		Time Render2DPass;
		Time AAPass;
		PipelineStatistics PipelineStats;

		uint32 Meshes;
		uint32 Instances;
		uint32 AnimMeshes;
	};

	using Render2DCallback = std::function<void()>;
	using OnViewportResizeCallback = std::function<void(uint32, uint32)>;

	class ATHENA_API SceneRenderer
	{
	public:
		static Ref<SceneRenderer> Create();
		~SceneRenderer();

		void Init();
		void Shutdown();

		SceneRendererSettings& GetSettings() { return m_Settings; }
		const SceneRendererStatistics& GetStatistics() { return m_Statistics; }
		Vector2u GetViewportSize() { return m_ViewportSize; }

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const CameraInfo& cameraInfo);
		void EndScene();

		void Submit(const Ref<StaticMesh>& mesh, const Matrix4& transform = Matrix4::Identity());
		void SubmitLightEnvironment(const LightEnvironment& lightEnv);

		void SubmitSelectionContext(const Ref<StaticMesh>& mesh, const Matrix4& transform = Matrix4::Identity());

		void SetOnRender2DCallback(const Render2DCallback& callback);
		void SetOnViewportResizeCallback(const OnViewportResizeCallback& callback);

		Ref<Texture2D> GetFinalImage();
		Ref<Texture2D> GetShadowMap();
		Ref<Texture2D> GetBloomTexture() { return m_HiColorBuffer; }

		Ref<RenderPass> GetGBufferPass() { return m_GBufferPass; }
		Ref<RenderPass> GetRender2DPass() { return m_Render2DPass; }
		Ref<RenderPass> GetAOPass() { return m_HBAOBlurYPass; }
		Ref<Pipeline> GetSkyboxPipeline() { return m_SkyboxPipeline; }

		Antialising GetAntialising() { return m_Settings.PostProcessingSettings.AntialisingMethod; }
		void ApplySettings();

	private:
		void DirShadowMapPass();
		void GBufferPass();
		void HiZPass();
		void LightCullingPass();
		void HBAOPass();
		void LightingPass();
		void SkyboxPass();
		void PreConvolutionPass();
		void SSRPass();
		void BloomPass();
		void SceneCompositePass();
		void JumpFloodPass();
		void Render2DPass();
		void FXAAPass();
		void SMAAPass();

		void CalculateInstanceTransforms();
		void CalculateCascadeLightSpaces(DirectionalLight& light);

		void ResetStats();

		void SubmitStaticMesh(DrawListStatic& list, const Ref<StaticMesh>& mesh, const Matrix4& transform);
		void SubmitAnimMesh(DrawListAnim& list, const Ref<StaticMesh>& mesh, const Ref<Animator>& animator, const Matrix4& transform);

	private:
		const uint32 m_ShadowMapResolution = 2048;

		const float m_OutlineWidth = 0.8f;
		const Vector4 m_OutlineColor = { 1.f, 0.5f, 0.f, 1.f };

	private:
		// DrawLists
		DrawListStatic m_StaticGeometryList;
		DrawListAnim m_AnimGeometryList;

		DrawListStatic m_SelectStaticGeometryList;
		DrawListAnim m_SelectAnimGeometryList;

		// Render Passes
		Ref<RenderPass> m_DirShadowMapPass;
		Ref<Pipeline> m_DirShadowMapStaticPipeline;
		Ref<Pipeline> m_DirShadowMapAnimPipeline;

		Ref<RenderPass> m_GBufferPass;
		Ref<Pipeline> m_StaticGeometryPipeline;
		Ref<Pipeline> m_AnimGeometryPipeline;

		Ref<Texture2D> m_HiZBuffer;
		Ref<ComputePass> m_HiZPass;
		Ref<ComputePipeline> m_HiZPipeline;
		std::vector<Ref<Material>> m_HiZMaterials;

		Ref<ComputePass> m_LightCullingPass;
		Ref<ComputePipeline> m_LightCullingPipeline;

		Ref<ComputePass> m_HBAODeinterleavePass;
		Ref<ComputePipeline> m_HBAODeinterleavePipeline;
		Ref<ComputePass> m_HBAOComputePass;
		Ref<ComputePipeline> m_HBAOComputePipeline;
		Ref<RenderPass> m_HBAOBlurXPass;
		Ref<Pipeline> m_HBAOBlurXPipeline;
		Ref<RenderPass> m_HBAOBlurYPass;
		Ref<Pipeline> m_HBAOBlurYPipeline;

		Ref<RenderPass> m_DeferredLightingPass;
		Ref<Pipeline> m_DeferredLightingPipeline;

		Ref<RenderPass> m_SkyboxPass;
		Ref<Pipeline> m_SkyboxPipeline;

		Ref<Texture2D> m_HiColorBuffer;
		Ref<Texture2D> m_BlurTmpTexture;
		Ref<ComputePass> m_PreConvolutionPass;
		Ref<ComputePipeline> m_PreConvolutionPipeline;
		std::vector<Ref<Material>> m_PreConvolutionMaterials;

		Ref<ComputePass> m_SSRComputePass;
		Ref<ComputePipeline> m_SSRComputePipeline;
		Ref<ComputePass> m_SSRCompositePass;
		Ref<ComputePipeline> m_SSRCompositePipeline;

		Ref<ComputePass> m_BloomPass;
		Ref<ComputePipeline> m_BloomDownsample;
		Ref<ComputePipeline> m_BloomUpsample;
		std::vector<Ref<Material>> m_BloomMaterials;

		Ref<RenderPass> m_SceneCompositePass;
		Ref<Pipeline> m_SceneCompositePipeline;
		Ref<Material> m_SceneCompositeMaterial;

		Ref<RenderPass> m_JumpFloodSilhouettePass;
		Ref<Pipeline> m_JFSilhouetteStaticPipeline;
		Ref<Pipeline> m_JFSilhouetteAnimPipeline;
		Ref<RenderPass> m_JumpFloodInitPass;
		Ref<Pipeline> m_JumpFloodInitPipeline;
		Ref<RenderPass> m_JumpFloodPasses[2];
		Ref<Pipeline> m_JumpFloodPipelines[2];
		Ref<Material> m_JumpFloodMaterial;
		Ref<RenderPass> m_JumpFloodCompositePass;
		Ref<Pipeline> m_JumpFloodCompositePipeline;
		Ref<Material> m_JumpFloodCompositeMaterial;

		Ref<RenderPass> m_Render2DPass;
		Render2DCallback m_Render2DCallback;

		Ref<Texture2D> m_PostProcessTextures[2];

		Ref<ComputePass> m_FXAAPass;
		Ref<ComputePipeline> m_FXAAPipeline;

		Ref<RenderPass> m_SMAAEdgesPass;
		Ref<Pipeline> m_SMAAEdgesPipeline;
		Ref<RenderPass> m_SMAAWeightsPass;
		Ref<Pipeline> m_SMAAWeightsPipeline;
		Ref<RenderPass> m_SMAABlendingPass;
		Ref<Pipeline> m_SMAABlendingPipeline;

		// CPU Data
		CameraData m_CameraData;
		SceneRendererData m_RendererData;
		LightData m_LightData;
		ShadowsData m_ShadowsData;
		HBAOData m_HBAOData;
		SSRData m_SSRData;
		uint32 m_BonesDataOffset;

		// GPU Data
		Ref<UniformBuffer> m_CameraUBO;
		Ref<UniformBuffer> m_RendererUBO;
		Ref<StorageBuffer> m_LightSBO;
		Ref<StorageBuffer> m_VisibleLightsSBO;
		Ref<UniformBuffer> m_ShadowsUBO;
		Ref<UniformBuffer> m_HBAO_UBO;
		Ref<UniformBuffer> m_SSR_UBO;
		Ref<TextureView> m_ShadowMapSampler;

		DynamicGPUBuffer<StorageBuffer> m_BonesSBO;
		DynamicGPUBuffer<VertexBuffer> m_TransformsStorage;

		// Other
		Vector2u m_ViewportSize = { 1, 1 };
		Vector2u m_OriginalViewportSize = { 1, 1 };
		OnViewportResizeCallback m_ViewportResizeCallback;

		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Ref<GPUProfiler> m_Profiler;
		SceneRendererStatistics m_Statistics;
		SceneRendererSettings m_Settings;
	};
}
