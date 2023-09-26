#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/RenderList.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	struct CameraData
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		Matrix4 RotationViewMatrix;
		Vector4 Position;
		float NearClip;
		float FarClip;
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

	struct SceneRendererData
	{
		Ref<Framebuffer> HDRFramebuffer;
		Ref<Framebuffer> FinalFramebuffer;
		Ref<Framebuffer> ShadowMap;
		Ref<Framebuffer> EntityIDFramebuffer;

		Ref<TextureSampler> PCF_Sampler;

		RenderList MeshList;

		Ref<EnvironmentMap> EnvironmentMap;

		CameraData CameraDataBuffer;
		SceneData SceneDataBuffer;
		EnvironmentMapData EnvMapDataBuffer;
		EntityData EntityDataBuffer;
		LightData LightDataBuffer;
		ShadowsData ShadowsDataBuffer;
		BloomData BloomDataBuffer;

		Ref<ConstantBuffer> CameraConstantBuffer;
		Ref<ConstantBuffer> SceneConstantBuffer;
		Ref<ConstantBuffer> EnvMapConstantBuffer;
		Ref<ConstantBuffer> EntityConstantBuffer;
		Ref<ConstantBuffer> MaterialConstantBuffer;
		Ref<ConstantBuffer> ShadowsConstantBuffer;
		Ref<ConstantBuffer> BloomConstantBuffer;

		Ref<ShaderStorageBuffer> LightShaderStorageBuffer;
		Ref<ShaderStorageBuffer> BoneTransformsShaderStorageBuffer;

		SceneRendererSettings Settings;

		const uint32 ShadowMapResolution = 2048;
	};

	static SceneRendererData s_Data;


	void SceneRenderer::Init()
	{
		FramebufferCreateInfo fbInfo;
		fbInfo.Attachments = { { TextureFormat::RGBA16F, true }, TextureFormat::DEPTH24STENCIL8 };
		fbInfo.Width = 1280;
		fbInfo.Height = 720;
		fbInfo.Layers = 1;
		fbInfo.Samples = 1;
		 
		s_Data.HDRFramebuffer = Framebuffer::Create(fbInfo);

		fbInfo.Attachments = { TextureFormat::RGBA8, TextureFormat::DEPTH24STENCIL8 };
		fbInfo.Width = 1280;
		fbInfo.Height = 720;
		fbInfo.Layers = 1;
		fbInfo.Samples = 1;

		s_Data.FinalFramebuffer = Framebuffer::Create(fbInfo);

		fbInfo.Attachments = { TextureFormat::DEPTH32F };
		fbInfo.Width = s_Data.ShadowMapResolution;
		fbInfo.Height = s_Data.ShadowMapResolution;
		fbInfo.Layers = ShaderDef::SHADOW_CASCADES_COUNT;
		fbInfo.Samples = 1;

		s_Data.ShadowMap = Framebuffer::Create(fbInfo);

		fbInfo.Attachments = { TextureFormat::RED_INTEGER, TextureFormat::DEPTH24STENCIL8 };
		fbInfo.Width = 1280;
		fbInfo.Height = 720;
		fbInfo.Layers = 1;
		fbInfo.Samples = 1;

		s_Data.EntityIDFramebuffer = Framebuffer::Create(fbInfo);

		TextureSamplerCreateInfo samplerInfo;
		samplerInfo.MinFilter = TextureFilter::LINEAR;
		samplerInfo.MagFilter = TextureFilter::LINEAR;
		samplerInfo.Wrap = TextureWrap::CLAMP_TO_BORDER;
		samplerInfo.BorderColor = LinearColor::White;
		samplerInfo.CompareMode = TextureCompareMode::REF;
		samplerInfo.CompareFunc = TextureCompareFunc::LEQUAL;

		s_Data.PCF_Sampler = TextureSampler::Create(samplerInfo);

		s_Data.CameraConstantBuffer = ConstantBuffer::Create(sizeof(CameraData), BufferBinder::CAMERA_DATA);
		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(SceneData), BufferBinder::SCENE_DATA);
		s_Data.EnvMapConstantBuffer = ConstantBuffer::Create(sizeof(EnvironmentMapData), BufferBinder::ENVIRONMENT_MAP_DATA);
		s_Data.EntityConstantBuffer = ConstantBuffer::Create(sizeof(EntityData), BufferBinder::ENTITY_DATA);
		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), BufferBinder::MATERIAL_DATA);
		s_Data.ShadowsConstantBuffer = ConstantBuffer::Create(sizeof(ShadowsData), BufferBinder::SHADOWS_DATA);
		s_Data.BloomConstantBuffer = ConstantBuffer::Create(sizeof(BloomData), BufferBinder::BLOOM_DATA);

		s_Data.LightShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(LightData), BufferBinder::LIGHT_DATA);
		s_Data.BoneTransformsShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(Matrix4) * ShaderDef::MAX_NUM_BONES, BufferBinder::BONES_DATA);
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	void SceneRenderer::OnWindowResized(uint32 width, uint32 height)
	{
		s_Data.HDRFramebuffer->Resize(width, height);
		s_Data.FinalFramebuffer->Resize(width, height);
		s_Data.EntityIDFramebuffer->Resize(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		s_Data.MeshList.Clear();

		s_Data.CameraDataBuffer.ViewMatrix = cameraInfo.ViewMatrix;
		s_Data.CameraDataBuffer.ProjectionMatrix = cameraInfo.ProjectionMatrix;
		s_Data.CameraDataBuffer.RotationViewMatrix = cameraInfo.ViewMatrix.AsMatrix3();
		s_Data.CameraDataBuffer.Position = Math::AffineInverse(cameraInfo.ViewMatrix)[3];
		s_Data.CameraDataBuffer.NearClip = cameraInfo.NearClip;
		s_Data.CameraDataBuffer.FarClip = cameraInfo.FarClip;

		s_Data.SceneDataBuffer.Exposure = s_Data.Settings.LightEnvironmentSettings.Exposure;
		s_Data.SceneDataBuffer.Gamma = s_Data.Settings.LightEnvironmentSettings.Gamma;

		s_Data.ShadowsDataBuffer.SoftShadows = s_Data.Settings.ShadowSettings.SoftShadows;
		s_Data.ShadowsDataBuffer.LightSize = s_Data.Settings.ShadowSettings.LightSize;
		s_Data.ShadowsDataBuffer.MaxDistance = s_Data.Settings.ShadowSettings.MaxDistance;
		s_Data.ShadowsDataBuffer.FadeOut = s_Data.Settings.ShadowSettings.FadeOut;

		s_Data.BloomDataBuffer.Intensity = s_Data.Settings.BloomSettings.Intensity;
		s_Data.BloomDataBuffer.Threshold = s_Data.Settings.BloomSettings.Threshold;
		s_Data.BloomDataBuffer.Knee = s_Data.Settings.BloomSettings.Knee;
		s_Data.BloomDataBuffer.DirtIntensity = s_Data.Settings.BloomSettings.DirtIntensity;

		if (s_Data.Settings.BloomSettings.DirtTexture == nullptr)
			s_Data.Settings.BloomSettings.DirtTexture = Renderer::GetBlackTexture();

		ComputeCascadeSplits();

		// Per frame buffers
		s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));
		s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));
		s_Data.EnvMapConstantBuffer->SetData(&s_Data.EnvMapDataBuffer, sizeof(EnvironmentMapData));

		FramebufferCreateInfo finalFBInfo = s_Data.FinalFramebuffer->GetCreateInfo();

		// TODO: remove hack
		uint32 samples = Math::Pow(2u, (uint32)s_Data.Settings.AntialisingMethod);
		if (samples != finalFBInfo.Samples)
		{
			finalFBInfo.Samples = samples;
			s_Data.FinalFramebuffer = Framebuffer::Create(finalFBInfo);
		}
	}

	void SceneRenderer::EndScene()
	{
		ShadowMapPass();
		GeometryPass();
		BloomPass();
		SceneCompositePass();
		DebugViewPass();

		s_Data.EnvironmentMap = nullptr;

		s_Data.LightDataBuffer.DirectionalLightCount = 0;
		s_Data.LightDataBuffer.PointLightCount = 0;
	}

	void SceneRenderer::FlushEntityIDs()
	{
		RenderGeometry("EntityID", false);
	}

	void SceneRenderer::BeginFrame() {}
	void SceneRenderer::EndFrame()
	{
		s_Data.FinalFramebuffer->ResolveMutlisampling();
		s_Data.FinalFramebuffer->UnBind();
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			DrawCallInfo info;
			info.VertexBuffer = vertexBuffer;
			info.Material = material;
			info.Animator = animator;
			info.Transform = transform;
			info.EntityID = entityID;

			s_Data.MeshList.Push(info);
		}
		else
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit nullptr vertexBuffer!");
		}
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		if (lightEnv.DirectionalLights.size() > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);

		if (lightEnv.PointLights.size() > ShaderDef::MAX_POINT_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);

		s_Data.LightDataBuffer.DirectionalLightCount = Math::Min(lightEnv.DirectionalLights.size(), (uint64)ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
		for (uint32 i = 0; i < s_Data.LightDataBuffer.DirectionalLightCount; ++i)
		{
			s_Data.LightDataBuffer.DirectionalLightBuffer[i] = lightEnv.DirectionalLights[i];
		}

		s_Data.LightDataBuffer.PointLightCount = Math::Min(lightEnv.PointLights.size(), (uint64)ShaderDef::MAX_POINT_LIGHT_COUNT);
		for (uint32 i = 0; i < s_Data.LightDataBuffer.PointLightCount; ++i)
		{
			s_Data.LightDataBuffer.PointLightBuffer[i] = lightEnv.PointLights[i];
		}

		s_Data.EnvironmentMap = lightEnv.EnvironmentMap;

		s_Data.EnvMapDataBuffer.LOD = lightEnv.EnvironmentMapLOD;
		s_Data.EnvMapDataBuffer.Intensity = lightEnv.EnvironmentMapIntensity;
	}

	void SceneRenderer::RenderGeometry(std::string_view shader, bool useMaterials)
	{
		if (s_Data.MeshList.Empty())
			return;

		Renderer::BindShader(shader);

		// Render Static Meshes
		s_Data.EntityDataBuffer.IsAnimated = false;

		while (s_Data.MeshList.HasStaticMeshes())
		{
			auto& info = s_Data.MeshList.Next();

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (useMaterials && s_Data.MeshList.UpdateMaterial())
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));

			Renderer::DrawTriangles(info.VertexBuffer);
		}

		// Render Animated Meshes
		s_Data.EntityDataBuffer.IsAnimated = true;

		while (s_Data.MeshList.HasAnimMeshes())
		{
			auto& info = s_Data.MeshList.Next();

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (s_Data.MeshList.UpdateAnimator())
			{
				const auto& boneTransforms = info.Animator->GetBoneTransforms();
				s_Data.BoneTransformsShaderStorageBuffer->SetData(boneTransforms.data(), sizeof(Matrix4) * boneTransforms.size());
			}

			if (useMaterials && s_Data.MeshList.UpdateMaterial())
			{
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));
			}

			Renderer::DrawTriangles(info.VertexBuffer);
		}

		s_Data.MeshList.Reset();
	}

	void SceneRenderer::ShadowMapPass()
	{
		Pipeline pipeline;
		pipeline.CullFace = CullFace::FRONT;
		pipeline.CullDirection = CullDirection::COUNTER_CLOCKWISE;
		pipeline.BlendFunc = BlendFunc::NONE;
		pipeline.DepthFunc = DepthFunc::LEQUAL;

		Renderer::BindPipeline(pipeline);

		RenderPass renderPass;
		renderPass.TargetFramebuffer = s_Data.ShadowMap;
		renderPass.ClearBit = CLEAR_DEPTH_BIT;
		renderPass.Name = "ShadowMapPass";

		Renderer::BeginRenderPass(renderPass);
		{
			if (s_Data.Settings.ShadowSettings.EnableShadows && s_Data.LightDataBuffer.DirectionalLightCount > 0) // For now only 1 directional light 
			{
				ComputeCascadeSpaceMatrices(s_Data.LightDataBuffer.DirectionalLightBuffer[0]);
				s_Data.ShadowsConstantBuffer->SetData(&s_Data.ShadowsDataBuffer, sizeof(ShadowsData));

				s_Data.MeshList.Sort();
				RenderGeometry("DirShadowMap", false);
			}
		}
		Renderer::EndRenderPass();
	}

	void SceneRenderer::GeometryPass()
	{
		Pipeline pipeline;
		pipeline.CullFace = CullFace::BACK;
		pipeline.CullDirection = CullDirection::COUNTER_CLOCKWISE;
		pipeline.BlendFunc = BlendFunc::ONE_MINUS_SRC_ALPHA;
		pipeline.DepthFunc = DepthFunc::LEQUAL;

		Renderer::BindPipeline(pipeline);

		RenderPass renderPass;
		renderPass.TargetFramebuffer = s_Data.HDRFramebuffer;
		renderPass.ClearBit = CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT;
		renderPass.ClearColor = LinearColor::Black;
		renderPass.Name = "GeometryPass";

		Renderer::BeginRenderPass(renderPass);
		{
			s_Data.LightShaderStorageBuffer->SetData(&s_Data.LightDataBuffer, sizeof(LightData));

			if (s_Data.EnvironmentMap)
			{
				s_Data.EnvironmentMap->Bind();
				Renderer::GetBRDF_LUT()->Bind(TextureBinder::BRDF_LUT);
			}

			s_Data.ShadowMap->BindDepthAttachment(TextureBinder::SHADOW_MAP);
			s_Data.ShadowMap->BindDepthAttachment(TextureBinder::PCF_SAMPLER);
			s_Data.PCF_Sampler->Bind(TextureBinder::PCF_SAMPLER);

			RenderGeometry("PBR", true);

			s_Data.PCF_Sampler->UnBind(TextureBinder::PCF_SAMPLER);

			if (s_Data.EnvironmentMap)
			{
				s_Data.CameraDataBuffer.ProjectionMatrix;
				s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));

				Renderer::BindShader("Skybox");
				Renderer::DrawTriangles(Renderer::GetCubeVertexBuffer());
			}
		}
		Renderer::EndRenderPass();
	}

	void SceneRenderer::BloomPass()
	{
		if (!s_Data.Settings.BloomSettings.EnableBloom)
			return;

		const FramebufferCreateInfo& hdrfbInfo = s_Data.HDRFramebuffer->GetCreateInfo();

		uint32 mipLevels = 1;
		Vector2u mipSize = { hdrfbInfo.Width / 2, hdrfbInfo.Height / 2 };

		// Compute mip levels
		{
			const uint32 maxIterations = 16;
			const uint32 downSampleLimit = 10;

			uint32 width = hdrfbInfo.Width;
			uint32 height = hdrfbInfo.Height;

			for (uint8 i = 0; i < maxIterations; ++i)
			{
				width = width / 2;
				height = height / 2;

				if (width < downSampleLimit || height < downSampleLimit) 
					break;

				++mipLevels;
			}

			mipLevels += 1;
		}

		ComputePass computePass;
		computePass.BarrierBit = SHADER_IMAGE_BARRIER_BIT | FRAMEBUFFER_BARRIER_BIT | BUFFER_UPDATE_BARRIER_BIT;
		computePass.Name = "Bloom";

		Renderer::BeginComputePass(computePass);
		// Downsample
		{
			Renderer::BindShader("BloomDownsample");
			s_Data.HDRFramebuffer->BindColorAttachment(0, 0);

			for (uint8 i = 0; i < mipLevels - 1; ++i)
			{
				s_Data.BloomDataBuffer.TexelSize = Vector2(1.f, 1.f) / Vector2(mipSize);
				s_Data.BloomDataBuffer.MipLevel = i;
				s_Data.BloomDataBuffer.EnableThreshold = i == 0;

				s_Data.HDRFramebuffer->BindColorAttachmentAsImage(0, 1, i + 1);
				s_Data.BloomConstantBuffer->SetData(&s_Data.BloomDataBuffer, sizeof(s_Data.BloomDataBuffer));

				Renderer::Dispatch(mipSize.x, mipSize.y, 1, { 8, 8, 1 });
				mipSize = mipSize / 2u;
			}
		}

		// Upsample
		{
			Renderer::BindShader("BloomUpsample");
			s_Data.HDRFramebuffer->BindColorAttachment(0, 0);

			if (s_Data.Settings.BloomSettings.DirtTexture)
				s_Data.Settings.BloomSettings.DirtTexture->Bind(2);

			for (uint8 i = mipLevels - 1; i >= 1; --i)
			{
				mipSize.x = Math::Max(1.f, Math::Floor(float(hdrfbInfo.Width) / Math::Pow<float>(2.f, i - 1)));
				mipSize.y = Math::Max(1.f, Math::Floor(float(hdrfbInfo.Height) / Math::Pow<float>(2.f, i - 1)));

				s_Data.BloomDataBuffer.TexelSize = Vector2(1.f, 1.f) / Vector2(mipSize);
				s_Data.BloomDataBuffer.MipLevel = i;

				s_Data.HDRFramebuffer->BindColorAttachmentAsImage(0, 1, i - 1);
				s_Data.BloomConstantBuffer->SetData(&s_Data.BloomDataBuffer, sizeof(s_Data.BloomDataBuffer));

				Renderer::Dispatch(mipSize.x, mipSize.y, 1, { 8, 8, 1 });
			}
		}
		Renderer::EndComputePass();
	}

	void SceneRenderer::SceneCompositePass()
	{
		RenderPass renderPass;
		renderPass.TargetFramebuffer = s_Data.FinalFramebuffer;
		renderPass.ClearBit = CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT;
		renderPass.Name = "SceneComposite";

		Renderer::BeginRenderPass(renderPass);
		{
			Renderer::BindShader("SceneComposite");

			s_Data.HDRFramebuffer->BindColorAttachment(0, 0);
			s_Data.HDRFramebuffer->BindDepthAttachment(1);

			Renderer::DrawTriangles(Renderer::GetQuadVertexBuffer());
		}
		Renderer::EndRenderPass();
	}

	void SceneRenderer::DebugViewPass()
	{
		if (s_Data.Settings.DebugView == DebugView::NONE)
			return;

		RenderPass renderPass;
		renderPass.TargetFramebuffer = s_Data.FinalFramebuffer;
		renderPass.ClearBit = CLEAR_NONE_BIT;
		renderPass.Name = "DebugView";
		
		Renderer::BeginRenderPass(renderPass);
		{
			if (s_Data.Settings.DebugView == DebugView::WIREFRAME)
				RenderGeometry("Debug_Wireframe", false);

			else if (s_Data.Settings.DebugView == DebugView::SHADOW_CASCADES)
				RenderGeometry("Debug_ShadowCascades", false);
		}
		Renderer::EndRenderPass();
	}

	void SceneRenderer::ComputeCascadeSplits()
	{
		float cameraNear = s_Data.CameraDataBuffer.NearClip;
		float cameraFar = s_Data.CameraDataBuffer.FarClip;

		const float splitWeight = s_Data.Settings.ShadowSettings.ExponentialSplitFactor;

		for (uint32 i = 0; i < ShaderDef::SHADOW_CASCADES_COUNT; ++i)
		{
			float percent = (i + 1) / float(ShaderDef::SHADOW_CASCADES_COUNT);
			float log = cameraNear * Math::Pow(cameraFar / cameraNear, percent);
			float uniform = Math::Lerp(cameraNear, cameraFar, percent); 
			float split = Math::Lerp(uniform, log, splitWeight);

			s_Data.ShadowsDataBuffer.CascadeSplits[i].SplitDepth = split;
		}
	}

	void SceneRenderer::ComputeCascadeSpaceMatrices(const DirectionalLight& light)
	{
		float cameraNear = s_Data.CameraDataBuffer.NearClip;
		float cameraFar = s_Data.CameraDataBuffer.FarClip;

		Matrix4 invCamera = Math::Inverse(s_Data.CameraDataBuffer.ViewMatrix * s_Data.CameraDataBuffer.ProjectionMatrix);

		float lastSplit = 0.f;
		float averageFrustumSize = 0.f;

		for (uint32 layer = 0; layer < ShaderDef::SHADOW_CASCADES_COUNT; ++layer)
		{
			float split = (s_Data.ShadowsDataBuffer.CascadeSplits[layer].SplitDepth - cameraNear) / (cameraFar - cameraNear); // range (0, 1)

			std::array<Vector3, 8> frustumCorners = {
				//Near face
				Vector3{  1.0f,  1.0f, -1.0f },
				Vector3{ -1.0f,  1.0f, -1.0f },
				Vector3{  1.0f, -1.0f, -1.0f },
				Vector3{ -1.0f, -1.0f, -1.0f },

				//Far face
				Vector3{  1.0f,  1.0f, 1.0f },
				Vector3{ -1.0f,  1.0f, 1.0f },
				Vector3{  1.0f, -1.0f, 1.0f },
				Vector3{ -1.0f, -1.0f, 1.0f },
			};

			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				Vector4 cornerWorldSpace = Vector4(frustumCorners[j], 1.f) * invCamera;
				frustumCorners[j] = cornerWorldSpace / cornerWorldSpace.w;
			}

			for (uint32_t j = 0; j < 4; ++j)
			{
				Vector3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * split);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplit);
			}

			Vector3 frustumCenter = Vector3(0.f);
			for (const auto& corner : frustumCorners)
				frustumCenter += corner;

			frustumCenter /= frustumCorners.size();

			float radius = 0.0f;
			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				float distance = (frustumCorners[j] - frustumCenter).Length();
				radius = Math::Max(radius, distance);
			}
			radius = Math::Ceil(radius * 16.0f) / 16.0f;

			Vector3 maxExtents = Vector3(radius);
			Vector3 minExtents = -maxExtents;

			minExtents.z += s_Data.Settings.ShadowSettings.NearPlaneOffset;
			maxExtents.z += s_Data.Settings.ShadowSettings.FarPlaneOffset;

			Matrix4 lightView = Math::LookAt(frustumCenter - light.Direction.GetNormalized() * minExtents.z, frustumCenter, Vector3::Up());
			Matrix4 lightProjection = Math::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, maxExtents.z - minExtents.z);

			Matrix4 lightSpace = lightView * lightProjection;
			
			Vector4 shadowOrigin = Vector4(0.f, 0.f, 0.f, 1.f);
			shadowOrigin = shadowOrigin * lightSpace;
			shadowOrigin = shadowOrigin * (s_Data.ShadowMapResolution / 2.f);

			Vector4 roundedOrigin = Math::Round(shadowOrigin);
			Vector4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * (2.f / s_Data.ShadowMapResolution);
			roundOffset.z = 0.f;
			roundOffset.w = 0.f;

			lightProjection[3] += roundOffset;

			s_Data.ShadowsDataBuffer.LightViewProjMatrices[layer] = lightView * lightProjection;
			s_Data.ShadowsDataBuffer.LightViewMatrices[layer] = lightView;

			s_Data.ShadowsDataBuffer.CascadeSplits[layer].LightFrustumPlanes = { minExtents.z, maxExtents.z };

			averageFrustumSize = Math::Max(averageFrustumSize, maxExtents.x - minExtents.x);

			lastSplit = split;
		}

		s_Data.ShadowsDataBuffer.LightSize /= averageFrustumSize;
	}

	void SceneRenderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, EnvironmentMap* envMap)
	{
		ComputePass computePass;
		computePass.BarrierBit = SHADER_IMAGE_BARRIER_BIT | BUFFER_UPDATE_BARRIER_BIT;
		computePass.Name = "PreProcessEnvironmentMap";

		Renderer::BeginComputePass(computePass);
		// Convert EquirectangularHDRMap to Cubemap
		Ref<TextureCube> skybox;
		{
			uint32 width = envMap->m_Resolution;
			uint32 height = envMap->m_Resolution;

			TextureSamplerCreateInfo samplerInfo;
			samplerInfo.MinFilter = TextureFilter::LINEAR;
			samplerInfo.MagFilter = TextureFilter::LINEAR;
			samplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			skybox = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, samplerInfo);

			equirectangularHDRMap->Bind();
			skybox->BindAsImage(1);

			Renderer::BindShader("EquirectangularToCubemap");
			Renderer::Dispatch(width, height, 6);

			skybox->SetFilters(TextureFilter::LINEAR_MIPMAP_LINEAR, TextureFilter::LINEAR);
			skybox->GenerateMipMap(ShaderDef::MAX_SKYBOX_MAP_LOD);
		}

		// Compute Irradiance Map
		{
			uint32 width = envMap->m_IrradianceResolution;
			uint32 height = envMap->m_IrradianceResolution;

			TextureSamplerCreateInfo samplerInfo;
			samplerInfo.MinFilter = TextureFilter::LINEAR;
			samplerInfo.MagFilter = TextureFilter::LINEAR;
			samplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			envMap->m_IrradianceMap = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, samplerInfo);

			skybox->Bind();
			envMap->m_IrradianceMap->BindAsImage(1);

			Renderer::BindShader("IrradianceMapConvolution");
			Renderer::Dispatch(width, height, 6);
		}

		// Compute Prefiltered Skybox Map
		{
			uint32 width = envMap->m_Resolution;
			uint32 height = envMap->m_Resolution;
		
			TextureSamplerCreateInfo samplerInfo;
			samplerInfo.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
			samplerInfo.MagFilter = TextureFilter::LINEAR;
			samplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;
		
			envMap->m_PrefilteredMap = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, samplerInfo);
			envMap->m_PrefilteredMap->GenerateMipMap(ShaderDef::MAX_SKYBOX_MAP_LOD);
		
			Renderer::BindShader("EnvironmentMipFilter");
			skybox->Bind();
		
			for (uint32 mip = 0; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				envMap->m_PrefilteredMap->BindAsImage(1, mip);
		
				uint32 mipWidth = width * Math::Pow(0.5f, (float)mip);
				uint32 mipHeight = height * Math::Pow(0.5f, (float)mip);
		
				float roughness = (float)mip / (float)(ShaderDef::MAX_SKYBOX_MAP_LOD - 1);
				s_Data.EnvMapDataBuffer.LOD = roughness;
				s_Data.EnvMapConstantBuffer->SetData(&s_Data.EnvMapDataBuffer, sizeof(EnvironmentMapData));
		
				Renderer::Dispatch(mipWidth, mipHeight, 6);
			}
		}

		Renderer::EndComputePass();
	}

	Ref<Framebuffer> SceneRenderer::GetEntityIDFramebuffer()
	{
		return s_Data.EntityIDFramebuffer;
	}

	Ref<Framebuffer> SceneRenderer::GetFinalFramebuffer()
	{
		return s_Data.FinalFramebuffer;
	}

	SceneRendererSettings& SceneRenderer::GetSettings()
	{
		return s_Data.Settings;
	}
}
