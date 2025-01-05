#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"

/*
	Asset - polymorphic base class -> TextureAsset, MaterialAsset ...
	AssetHandle - UUID for Asset
	AssetRef - AssetHandle + cached Asset
	AssetDatabase - database that stores all asset handles and its metadata 

*/


namespace Athena
{
	using AssetHandle = UUID;

	enum class AssetType
	{
		None = 0,
		Scene,
		Texture2D,
		EnvironmentMap,
		MeshSource,
		StaticMesh,
		Material
	};

	struct AssetMetadata
	{
		AssetHandle Handle = 0;
		AssetType Type = AssetType::None;
		FilePath FilePath;
	};


	class ATHENA_API Asset
	{
	public:
		AssetHandle Handle;

		virtual AssetType GetType() const = 0;

		static std::string_view AssetTypeToString(AssetType type);
		static AssetType AssetTypeFromString(std::string_view assetType);
	};
	
	template <typename T>
	class AssetRef
	{
	public:
		AssetRef(AssetHandle handle)
			: m_Handle(handle)
		{

		}

		WeakRef<T> Get()
		{
			if (m_Asset == nullptr || m_Asset.Expired())
			{
				// Ask for Asset
			}

			return m_Asset;
		}

	private:
		WeakRef<T> m_Asset;
		AssetHandle m_Handle;
	};


	std::string_view Asset::AssetTypeToString(AssetType type)
	{
		switch (type)
		{
			case AssetType::None:			return "AssetType::None";
			case AssetType::Scene:			return "AssetType::Scene";
			case AssetType::Texture2D:		return "AssetType::Texture2D";
			case AssetType::EnvironmentMap: return "AssetType::EnvironmentMap";
			case AssetType::MeshSource:		return "AssetType::MeshSource";
			case AssetType::StaticMesh:		return "AssetType::StaticMesh";
			case AssetType::Material:		return "AssetType::Material";
		}

		return "AssetType::<Invalid>";
	}

	AssetType Asset::AssetTypeFromString(std::string_view assetType)
	{
		if (assetType == "AssetType::None")			  return AssetType::None;
		if (assetType == "AssetType::Scene")		  return AssetType::Scene;
		if (assetType == "AssetType::Texture2D")	  return AssetType::Texture2D;
		if (assetType == "AssetType::EnvironmentMap") return AssetType::EnvironmentMap;
		if (assetType == "AssetType::MeshSource")	  return AssetType::MeshSource;
		if (assetType == "AssetType::StaticMesh")	  return AssetType::StaticMesh;
		if (assetType == "AssetType::Material")		  return AssetType::Material;

		return AssetType::None;
	}
}
