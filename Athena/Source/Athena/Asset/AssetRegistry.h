#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"

#include <unordered_map>


namespace Athena
{
	class ATHENA_API AssetRegistry
	{
	public:
		void AddAsset(const Ref<Asset>& asset);
		void AddAssetMetadata(AssetHandle handle, const AssetMetadata& metadata);

		Ref<Asset> GetAsset(AssetHandle handle);
		AssetMetadata& GetMetadata(AssetHandle handle);

		void Serialize();
		void Deserialize();

	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_Registry;
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryOnlyAssets;
	};
}
