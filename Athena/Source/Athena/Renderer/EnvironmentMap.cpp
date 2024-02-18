#include "EnvironmentMap.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(uint32 resolution)
	{
		Ref<EnvironmentMap> result = Ref<EnvironmentMap>::Create(resolution);

		return result;
	}

	EnvironmentMap::EnvironmentMap(uint32 resolution)
	{
		m_Dirty = true;
		m_Type = EnvironmentMapType::PREETHAM;
		m_Resolution = resolution;

		TextureCubeCreateInfo cubemapInfo;
		cubemapInfo.Name = "EnvironmentMap";
		cubemapInfo.Format = ImageFormat::R11G11B10F;
		cubemapInfo.Usage = ImageUsage(ImageUsage::STORAGE | ImageUsage::SAMPLED);
		cubemapInfo.InitialData = nullptr;
		cubemapInfo.Width = m_Resolution;
		cubemapInfo.Height = m_Resolution;
		cubemapInfo.MipLevels = ShaderDef::MAX_SKYBOX_MAP_LOD;
		cubemapInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

		m_EnvironmentTexture = TextureCube::Create(cubemapInfo);

		cubemapInfo.Name = "EnvIrradianceMap";
		cubemapInfo.Width = m_IrradianceMapResolution;
		cubemapInfo.Height = m_IrradianceMapResolution;
		cubemapInfo.MipLevels = 1;

		m_IrradianceTexture = TextureCube::Create(cubemapInfo);

		// Panorama To Cubemap
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "PanoramaToCubePass";
			passInfo.DebugColor = { 0.2f, 0.4f, 0.6f };

			m_PanoramaToCubePass = ComputePass::Create(passInfo);
			m_PanoramaToCubePass->SetOutput(m_EnvironmentTexture->GetImage());
		}

		// Preetham
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "PreethamPass";
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_PreethamPass = ComputePass::Create(passInfo);
			m_PreethamPass->SetOutput(m_EnvironmentTexture->GetImage());

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "PreethamPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PreethamSky");

			m_PreethamPipeline = ComputePipeline::Create(pipelineInfo);
			m_PreethamPipeline->SetInput("u_EnvironmentMap", m_EnvironmentTexture);
			m_PreethamPipeline->Bake();

			m_PreethamMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);
		}

		// Irradiance Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "IrradiancePass";
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_IrradiancePass = ComputePass::Create(passInfo);
			m_IrradiancePass->SetOutput(m_IrradianceTexture->GetImage());

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "IrradianceMapPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("IrradianceMapConvolution");

			m_IrradiancePipeline = ComputePipeline::Create(pipelineInfo);
			m_IrradiancePipeline->SetInput("u_Cubemap", m_EnvironmentTexture);
			m_IrradiancePipeline->SetInput("u_IrradianceMap", m_IrradianceTexture);
			m_IrradiancePipeline->Bake();
		}

		// Mip Filter Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "MipFilterPass";
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_MipFilterPass = ComputePass::Create(passInfo);
			m_MipFilterPass->SetOutput(m_EnvironmentTexture->GetImage());

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "MipFilterPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("EnvironmentMipFilter");

			m_MipFilterPipeline = ComputePipeline::Create(pipelineInfo);
			m_MipFilterPipeline->SetInput("u_EnvironmentMap", m_EnvironmentTexture);
			m_MipFilterPipeline->Bake();

			for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				Ref<Material> mipMaterial = Material::Create(pipelineInfo.Shader, std::format("{}_{}", pipelineInfo.Name, mip));
				mipMaterial->Set("u_EnvironmentMipImage", m_EnvironmentTexture, 0, mip);
				mipMaterial->Set("u_MipLevel", mip);

				m_MipFilterMaterials[mip] = mipMaterial;
			}
		}
	}

	void EnvironmentMap::SetType(EnvironmentMapType type)
	{
		m_Type = type;
		m_Dirty = true;
	}

	void EnvironmentMap::SetResolution(uint32 resolution)
	{
		if (m_Resolution == resolution)
			return;

		m_Resolution = resolution;
		m_EnvironmentTexture->Resize(resolution, resolution);
		m_Dirty = true;
	}

	void EnvironmentMap::SetFilePath(const FilePath& path)
	{
		if (m_FilePath == path)
			return;

		m_FilePath = path;
		m_Dirty = true;
	}

	void EnvironmentMap::SetPreethamParams(float turbidity, float azimuth, float inclination)
	{
		if (m_Turbidity == turbidity && m_Azimuth == azimuth && m_Inclination == inclination)
			return;

		m_Turbidity = turbidity;
		m_Azimuth = azimuth;
		m_Inclination = inclination;
		m_Dirty = true;
	}

	Ref<TextureCube> EnvironmentMap::GetEnvironmentTexture()
	{
		if (m_Dirty)
			Load();

		return m_EnvironmentTexture;
	}

	Ref<TextureCube> EnvironmentMap::GetIrradianceTexture()
	{
		if (m_Dirty)
			Load();

		return m_IrradianceTexture;
	}

	void EnvironmentMap::LoadFromFile(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Ref<Texture2D> panorama = Texture2D::Create(m_FilePath);

		ComputePipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "PanoramaToCubemapPipeline";
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PanoramaToCubemap");

		Ref<ComputePipeline> panoramaToCubePipeline = ComputePipeline::Create(pipelineInfo);
		panoramaToCubePipeline->SetInput("u_PanoramaTex", panorama);
		panoramaToCubePipeline->SetInput("u_Cubemap", m_EnvironmentTexture);
		panoramaToCubePipeline->Bake();

		m_PanoramaToCubePass->Begin(commandBuffer);
		if (panoramaToCubePipeline->Bind(commandBuffer))
		{
			Renderer::Dispatch(commandBuffer, panoramaToCubePipeline, { m_Resolution, m_Resolution, 6 });
		}
		m_PanoramaToCubePass->End(commandBuffer);
	}

	void EnvironmentMap::LoadPreetham(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		m_PreethamPass->Begin(commandBuffer);
		if (m_PreethamPipeline->Bind(commandBuffer))
		{
			m_PreethamMaterial->Set("u_Turbidity", m_Turbidity);
			m_PreethamMaterial->Set("u_Azimuth", m_Azimuth);
			m_PreethamMaterial->Set("u_Inclination", m_Inclination);
			m_PreethamMaterial->Bind(commandBuffer);

			Renderer::Dispatch(commandBuffer, m_PreethamPipeline, { m_Resolution, m_Resolution, 6 }, m_PreethamMaterial);
		}
		m_PreethamPass->End(commandBuffer);
	}

	void EnvironmentMap::Load()
	{
		if (m_Type == EnvironmentMapType::STATIC && m_FilePath.empty())
			return;

		Ref<RenderCommandBuffer> commandBuffer = Renderer::GetRenderCommandBuffer();

		if (m_Type == EnvironmentMapType::STATIC)
			LoadFromFile(commandBuffer);
		else if (m_Type == EnvironmentMapType::PREETHAM)
			LoadPreetham(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "EnvironmentBlitMipMap", { 0.6f, 0.4f, 0.2f, 1.f });
		{
			m_EnvironmentTexture->BlitMipMap(commandBuffer, ShaderDef::MAX_SKYBOX_MAP_LOD);
		}
		Renderer::EndDebugRegion(commandBuffer);


		m_IrradiancePass->Begin(commandBuffer);
		if (m_IrradiancePipeline->Bind(commandBuffer))
		{
			Renderer::Dispatch(commandBuffer, m_IrradiancePipeline, { m_IrradianceMapResolution, m_IrradianceMapResolution, 6 });
		}
		m_IrradiancePass->End(commandBuffer);


		m_MipFilterPass->Begin(commandBuffer);
		if (m_MipFilterPipeline->Bind(commandBuffer))
		{
			for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				uint32 mipResolution = m_Resolution * Math::Pow(0.5f, (float)mip);

				m_MipFilterMaterials[mip]->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, m_MipFilterPipeline, { mipResolution, mipResolution, 6 }, m_MipFilterMaterials[mip]);
			}
		}
		m_MipFilterPass->End(commandBuffer);

		m_Dirty = false;
	}
}
