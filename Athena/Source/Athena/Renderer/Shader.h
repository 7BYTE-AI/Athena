#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Texture.h"

#include <map>


namespace Athena
{
	enum ShaderStage
	{
		UNDEFINED	   = BIT(0),
		VERTEX_STAGE   = BIT(1),
		FRAGMENT_STAGE = BIT(2),
		GEOMETRY_STAGE = BIT(3),
		COMPUTE_STAGE  = BIT(4)
	};

	enum class ShaderResourceType
	{
		Unknown = 0,
		Texture2D,
		TextureCube,
		StorageTexture2D,
		StorageTextureCube,
		UniformBuffer,
		StorageBuffer,
	};

	struct ShaderResourceDescription
	{
		ShaderResourceType Type;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
	};

	struct StructMemberShaderMetaData
	{
		ShaderDataType Type;	// Unknown if custom struct
		uint32 Size;
		uint32 Offset;
	};

	struct PushConstantShaderMetaData
	{
		bool Enabled;
		uint32 Size;
		std::unordered_map<String, StructMemberShaderMetaData> Members;
		ShaderStage StageFlags;
	};

	struct BufferShaderMetaData
	{
		uint64 Size;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
		ShaderStage StageFlags;
	};

	struct TextureShaderMetaData
	{
		TextureType TextureType;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
		ShaderStage StageFlags;
	};

	struct ShaderMetaData
	{
		std::unordered_map<String, TextureShaderMetaData> SampledTextures;
		std::unordered_map<String, TextureShaderMetaData> StorageTextures;
		std::unordered_map<String, BufferShaderMetaData> UniformBuffers;
		std::unordered_map<String, BufferShaderMetaData> StorageBuffers;

		PushConstantShaderMetaData PushConstant;
		Vector3u WorkGroupSize;
	};

	class ATHENA_API Shader
	{
	public:
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const FilePath& path, const String& name);

		virtual ~Shader() = default;

		virtual void Reload() = 0;
		virtual bool IsCompute() = 0;

		void AddOnReloadCallback(uint64 hash, const std::function<void()>& callback);
		void RemoveOnReloadCallback(uint64 hash);

		const ShaderMetaData& GetMetaData() { return m_MetaData; }
		const auto& GetResourcesDescription() const { return m_ResourcesDescriptionTable; }

		bool IsCompiled() const { return m_IsCompiled; }
		const String& GetName() const { return m_Name; }

	protected:
		String m_Name;
		FilePath m_FilePath;
		bool m_IsCompiled;
		ShaderMetaData m_MetaData;
		std::unordered_map<String, ShaderResourceDescription> m_ResourcesDescriptionTable;
		std::unordered_map<uint64, std::function<void()>> m_OnReloadCallbacks;
	};

	class ATHENA_API ShaderPack
	{
	public:
		static Ref<ShaderPack> Create(const FilePath& path);
		void Add(const String& name, const Ref<Shader>& shader);

		Ref<Shader> Load(const FilePath& path);
		Ref<Shader> Load(const FilePath& path, const String& name);
		Ref<Shader> Get(const String& name);

		bool Exists(const String& name) const;
		void Reload();
		bool IsCompiled() const;

		const FilePath GetDirectory() const { return m_Directory; };

		auto begin() { return m_Shaders.begin(); }
		auto end() { return m_Shaders.end(); }

	private:
		void LoadDirectory(const FilePath& path);

	private:
		std::map<String, Ref<Shader>> m_Shaders;
		FilePath m_Directory;
	};
}
