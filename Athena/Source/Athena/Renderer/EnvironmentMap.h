#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	enum class EnvironmentMapType
	{
		STATIC = 1,
		PREETHAM,
	};

	class ATHENA_API EnvironmentMap
	{
	public:
		static Ref<EnvironmentMap> Create(uint32 resolution);
		EnvironmentMap(uint32 resolution);

		EnvironmentMapType GetType() const { return m_Type; }
		void SetType(EnvironmentMapType type);

		void SetFilePath(const FilePath& path);
		const FilePath& GetFilePath() { return m_FilePath; };

		Ref<TextureCube> GetEnvironmentTexture();
		Ref<TextureCube> GetIrradianceTexture();

		uint32 GetResolution() const { return m_Resolution; }
		void SetResolution(uint32 resolution);

		void SetPreethamParams(float turbidity, float azimuth, float inclination);

		float GetTurbidity() const { return m_Turbidity; }
		float GetAzimuth() const { return m_Azimuth; }
		float GetInclination() const { return m_Inclination; }

	private:
		void LoadFromFile(const Ref<RenderCommandBuffer>& commandBuffer);
		void LoadPreetham(const Ref<RenderCommandBuffer>& commandBuffer);
		void Load();

		bool IsEmpty();

	private:
		Ref<TextureCube> m_EnvironmentTexture;
		Ref<TextureCube> m_IrradianceTexture;

		EnvironmentMapType m_Type;
		bool m_Dirty = true;

		uint32 m_Resolution = 1024;
		uint32 m_IrradianceMapResolution = 64;

		float m_Turbidity = 2.f;
		float m_Azimuth = 0.f;
		float m_Inclination = 0.f;

		FilePath m_FilePath;

		Ref<ComputePass> m_PreethamPass;
		Ref<ComputePipeline> m_PreethamPipeline;
		Ref<Material> m_PreethamMaterial;

		Ref<ComputePass> m_PanoramaToCubePass;
		Ref<ComputePipeline> m_PanoramaToCubePipeline;

		Ref<ComputePass> m_IrradiancePass;
		Ref<ComputePipeline> m_IrradiancePipeline;

		Ref<ComputePass> m_MipFilterPass;
		Ref<ComputePipeline> m_MipFilterPipeline;
		std::array<Ref<Material>, ShaderDef::MAX_SKYBOX_MAP_LOD> m_MipFilterMaterials;
	};
}
