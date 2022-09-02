#include "atnpch.h"
#include "Texture.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/OpenGLTexture2D.h"


namespace Athena
{
	Ref<Texture2D> Texture2D::Create(uint32 width, uint32 height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(width, height); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const String& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(path); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::DefaultTexture()
	{
		auto whiteTexture = Texture2D::Create(1, 1);
		uint32 whiteTextureData = 0xffffffff;
		whiteTexture->SetData(&whiteTextureData, sizeof(uint32));

		return whiteTexture;
	}


	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture)
	{
		SetTexture(texture);
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords)
	{
		SetTexture(texture);
		SetTexCoords(texCoords);
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
	{
		SetTexture(texture);
		SetTexCoords(min, max);
	}

	void Texture2DInstance::SetTexCoords(const Vector2& min, const Vector2& max) 
	{
		float width = (float)m_Texture->GetWidth();
		float height = (float)m_Texture->GetHeight();

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}
}
