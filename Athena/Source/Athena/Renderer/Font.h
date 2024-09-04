#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class FontGeometry;

	class ATHENA_API Font: public RefCounted
	{
	public:
		static bool Init();
		static void Shutdown();

		static Ref<Font> Create(const FilePath& path);
		~Font();

		static Ref<Font> GetDefault();

		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		const FilePath& GetFilePath() const { return m_FilePath; }
		FontGeometry* GetFontGeometry() { return m_FontGeometry; }

	private:
		Buffer GenerateAtlasOrReadFromCache(uint32 width, uint32 height);

	private:
		FilePath m_FilePath;
		FontGeometry* m_FontGeometry = nullptr;
		Ref<Texture2D> m_AtlasTexture;
	};
}
