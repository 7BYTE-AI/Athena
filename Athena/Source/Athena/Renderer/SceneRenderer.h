#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/GPUProfiler.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Light.h"
#include "Pipeline.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	enum class Antialising
	{
		NONE = 0,
		MSAA_2X = 1,
		MSAA_4X = 2,
		MSAA_8X = 3,
	};

	enum class DebugView
	{
		NONE = 0,
		WIREFRAME = 1,
		SHADOW_CASCADES = 2
	};

	struct LightEnvironmentSettings
	{
		float Exposure = 1;
		float Gamma = 2.2f;
	};

	struct ShadowSettings
	{
		bool EnableShadows = true;
		bool SoftShadows = true;
		float LightSize = 0.5f;
		float MaxDistance = 200.f;
		float FadeOut = 15.f;
		float ExponentialSplitFactor = 0.91f;
		float NearPlaneOffset = -15.f;
		float FarPlaneOffset = 15.f;
	};

	struct BloomSettings
	{
		bool EnableBloom = true;
		float Intensity = 1;
		float Threshold = 1.5;
		float Knee = 0.1;
		float DirtIntensity = 2;
		Ref<Texture2D> DirtTexture;
	};

	struct SceneRendererSettings
	{
		LightEnvironmentSettings LightEnvironmentSettings;
		ShadowSettings ShadowSettings;
		BloomSettings BloomSettings;
		DebugView DebugView = DebugView::NONE;
		Antialising AntialisingMethod = Antialising::MSAA_2X;
	};


	struct CameraData
	{
		Matrix4 View;
		Matrix4 Projection;
	};

	struct SceneData
	{
		float Exposure = 1.f;
		float Gamma = 2.2f;
	};

	struct EnvironmentMapData
	{
		float LOD = 0.f;
		float Intensity = 1.f;
	};

	struct EntityData
	{
		Matrix4 TransformMatrix;
		int32 EntityID = -1;
		bool IsAnimated = false;
	};

	struct LightData
	{
		DirectionalLight DirectionalLightBuffer[ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLightBuffer[ShaderDef::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;
	};

	struct ShadowsData
	{
		struct CascadeSplitInfo
		{
			Vector2 LightFrustumPlanes;
			float SplitDepth;
			float __Padding;
		};

		Matrix4 LightViewProjMatrices[ShaderDef::SHADOW_CASCADES_COUNT];
		Matrix4 LightViewMatrices[ShaderDef::SHADOW_CASCADES_COUNT];
		CascadeSplitInfo CascadeSplits[ShaderDef::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float LightSize = 0.5f;
		bool SoftShadows = true;
	};

	struct BloomData
	{
		float Intensity;
		float Threshold;
		float Knee;
		float DirtIntensity;
		Vector2 TexelSize;
		bool EnableThreshold;
		int32 MipLevel;
	};

	struct SceneRendererStatistics
	{
		Time GeometryPass;
		PipelineStatistics PipelineStats;
	};

	struct DrawCall
	{
		Ref<VertexBuffer> VertexBuffer;
		Matrix4 Transform;
	};

	struct SceneRendererData
	{
		std::vector<DrawCall> StaticGeometryList;

		Ref<RenderPass> GeometryPass;
		Ref<Pipeline> StaticGeometryPipeline;

		CameraData CameraData;
		Ref<UniformBuffer> CameraUBO;

		Vector2u ViewportSize = { 1, 1 };

		Ref<GPUProfiler> Profiler;
		SceneRendererStatistics Statistics;
	};

	class ATHENA_API SceneRenderer : public RefCounted
	{
	public:
		static Ref<SceneRenderer> Create();

		void Render(const CameraInfo& cameraInfo);

		void Init();
		void Shutdown();

		const SceneRendererStatistics& GetStatistics() { return m_Data->Statistics; }
		Vector2u GetViewportSize() { return m_Data->ViewportSize; }

		Ref<Texture2D> GetFinalImage();

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const CameraInfo& cameraInfo);
		void EndScene();

		void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity());
		void SubmitLightEnvironment(const LightEnvironment& lightEnv);

	private:
		void GeometryPass();

	private:
		SceneRendererData* m_Data = nullptr;
	};
}
