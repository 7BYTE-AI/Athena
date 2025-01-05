#include "AssetRegistry.h"
#include "Athena/Core/YAMLTypes.h"

#include <yaml-cpp/yaml.h>


namespace Athena
{
	void AssetRegistry::AddAsset(const Ref<Asset>& asset)
	{

	}

	Ref<Asset> AssetRegistry::GetAsset(AssetHandle handle)
	{
		return nullptr;
	}

	void AssetRegistry::AddAssetMetadata(AssetHandle handle, const AssetMetadata& metadata)
	{

	}

	AssetMetadata& AssetRegistry::GetMetadata(AssetHandle handle)
	{
		auto x = AssetMetadata();
		return x;
	}

	void AssetRegistry::Serialize()
	{

	}

	void AssetRegistry::Deserialize()
	{

	}
}
