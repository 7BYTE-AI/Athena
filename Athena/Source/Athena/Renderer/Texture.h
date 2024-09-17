#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/RenderResource.h"

#include <array>


namespace Athena
{
	enum class TextureFormat
	{
		NONE = 0,
		// Color
		R8,
		R8_SRGB,
		RG8,
		RG8_SRGB,
		RGB8,
		RGB8_SRGB,
		RGBA8,
		RGBA8_SRGB,

		R32F,
		RG16F,
		RGB16F,
		R11G11B10F,
		RGB32F,
		RGBA16F,
		RGBA32F,

		//Depth/Stencil
		DEPTH16,
		DEPTH24STENCIL8,
		DEPTH32F
	};

	enum TextureUsage
	{
		NONE = BIT(0),
		SAMPLED = BIT(1),
		STORAGE = BIT(2),
		ATTACHMENT = BIT(3),

		DEFAULT = SAMPLED
	};

	enum class TextureType
	{
		TEXTURE_2D,
		TEXTURE_CUBE
	};

	enum class TextureFilter
	{
		LINEAR = 1,
		NEAREST = 2,
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	enum class TextureCompareOperator
	{
		NONE = 0,
		LESS_OR_EQUAL,
		GREATER_OR_EQUAL
	};

	struct TextureSamplerCreateInfo
	{
		bool operator==(const TextureSamplerCreateInfo& other) const = default;

		TextureFilter Filter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;
		TextureCompareOperator Compare = TextureCompareOperator::NONE;
	};

	struct TextureViewCreateInfo
	{
		bool operator==(const TextureViewCreateInfo& other) const = default;

		String Name; // Optional (if empty, it will be generated)

		uint32 BaseMipLevel = 0;
		uint32 MipLevelCount = 1;
		uint32 BaseLayer = 0;
		uint32 LayerCount = 1;

		bool EnableAlphaBlending = true;
		bool GrayScale = false;

		bool OverrideSampler = false;
		TextureSamplerCreateInfo Sampler;
	};
}

// Hash functions
namespace std
{
	using namespace Athena;

	template<>
	struct hash<TextureSamplerCreateInfo>
	{
		size_t operator()(const TextureSamplerCreateInfo& value) const
		{
			string str = std::format("{}{}{}", (uint32)value.Filter, (uint32)value.Wrap, (uint32)value.Compare);
			return hash<string>()(str);
		}
	};

	template<>
	struct hash<TextureViewCreateInfo>
	{
		size_t operator()(const TextureViewCreateInfo& value) const
		{
			size_t hash = value.BaseMipLevel ^ value.MipLevelCount + value.BaseLayer ^ value.LayerCount;
			hash = (hash << 1) + value.EnableAlphaBlending;
			hash = (hash << 1) + value.GrayScale;
			hash = (hash << 1) + value.OverrideSampler;
			return hash;
		}
	};
}

namespace Athena
{
	class Texture;

	class ATHENA_API TextureView : public RenderResource
	{
	public:
		static Ref<TextureView> Create(const Ref<Texture>& texture, const TextureViewCreateInfo& info);
		virtual ~TextureView() = default;

		virtual void Invalidate() = 0;

		virtual const String& GetName() const override { return m_Info.Name; }

		WeakRef<Texture> GetTexture() const { return m_Texture; }
		const TextureViewCreateInfo& GetInfo() const { return m_Info; }

	protected:
		WeakRef<Texture> m_Texture;
		TextureViewCreateInfo m_Info;
	};

	struct TextureCreateInfo
	{
		String Name;
		TextureFormat Format = TextureFormat::RGBA8;
		TextureUsage Usage = TextureUsage::DEFAULT;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		bool GenerateMipMap = false;
		TextureSamplerCreateInfo Sampler;
	};

	class ATHENA_API Texture : public RenderResource
	{
	public:
		virtual ~Texture();

		virtual TextureType GetType() const = 0;

		virtual void Resize(uint32 width, uint32 height) = 0;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) = 0;

		virtual void WriteContentToBuffer(Buffer* dstBuffer) = 0;
		virtual uint32 GetImageLayerCount() const = 0;

		Vector2u GetMipSize(uint32 mip) const;
		uint32 GetMipLevelsCount() const;

		Ref<TextureView> GetMipView(uint32 mip);
		Ref<TextureView> GetLayerView(uint32 layer);
		Ref<TextureView> GetView(const TextureViewCreateInfo& info);
		void InvalidateViews();
		void ClearViews();

		uint32 GetWidth() const { return m_Info.Width; }
		uint32 GetHeight() const { return m_Info.Height; }
		Vector2u GetSize() const { return { m_Info.Width, m_Info.Height }; }
		TextureFormat GetFormat() const { return m_Info.Format; }

		const TextureCreateInfo& GetInfo() const { return m_Info; };

	public:
		static bool IsDepthFormat(TextureFormat format);
		static bool IsStencilFormat(TextureFormat format);
		static bool IsColorFormat(TextureFormat format);
		static bool IsHDRFormat(TextureFormat format);
		static uint32 BytesPerPixel(TextureFormat format);
		static uint32 ChannelsNum(TextureFormat format);

	protected:
		TextureCreateInfo m_Info;
		std::unordered_map<TextureViewCreateInfo, Ref<TextureView>> m_TextureViews;
	};


	class ATHENA_API Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureCreateInfo& info, Buffer data = Buffer());

		virtual TextureType GetType() const override { return TextureType::TEXTURE_2D; }

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::Texture2D; }
		virtual const String& GetName() const override { return m_Info.Name; }

		virtual uint32 GetImageLayerCount() const override { return m_Info.Layers; }
		const FilePath& GetFilePath() const { return m_FilePath; }

	private:
		friend class TextureImporter;

	private:
		FilePath m_FilePath;
	};

	class ATHENA_API TextureCube: public Texture
	{
	public:
		static Ref<TextureCube> Create(const TextureCreateInfo& info, Buffer data = Buffer());

		virtual TextureType GetType() const override { return TextureType::TEXTURE_CUBE; }

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::TextureCube; }
		virtual const String& GetName() const override { return m_Info.Name; }

		virtual uint32 GetImageLayerCount() const override { return m_Info.Layers * 6; }
	};

	class ATHENA_API Texture2DInstance
	{
	public:
		Texture2DInstance();
		Texture2DInstance(const Ref<Texture2D>& texture);
		Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		inline const Ref<Texture2D>& GetNativeTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		inline void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
		inline void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};


	inline bool Texture::IsDepthFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH16:		 return true;
		case TextureFormat::DEPTH24STENCIL8: return true;
		case TextureFormat::DEPTH32F:		 return true;
		}

		return false;
	}

	inline bool Texture::IsStencilFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	inline bool Texture::IsColorFormat(TextureFormat format)
	{
		return !IsDepthFormat(format) && !IsStencilFormat(format);
	}

	inline bool Texture::IsHDRFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R32F:		return true;
		case TextureFormat::RG16F:		return true;
		case TextureFormat::R11G11B10F: return true;
		case TextureFormat::RGB16F:		return true;
		case TextureFormat::RGB32F:		return true;
		case TextureFormat::RGBA16F:	return true;
		case TextureFormat::RGBA32F:	return true;
		case TextureFormat::DEPTH32F:	return true;
		}

		return false;
	}

	inline uint32 Texture::BytesPerPixel(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8:			   return 1;
		case TextureFormat::R8_SRGB:	   return 1;
		case TextureFormat::RG8:		   return 2;
		case TextureFormat::RG8_SRGB:	   return 2;
		case TextureFormat::RGB8:		   return 3 * 1;
		case TextureFormat::RGB8_SRGB:	   return 3 * 1;
		case TextureFormat::RGBA8:		   return 4 * 1;
		case TextureFormat::RGBA8_SRGB:	   return 4 * 1;

		case TextureFormat::R32F:		   return 1 * 4;
		case TextureFormat::RG16F:		   return 2 * 2;
		case TextureFormat::R11G11B10F:	   return 4;
		case TextureFormat::RGB16F:		   return 3 * 2;
		case TextureFormat::RGB32F:		   return 3 * 4;
		case TextureFormat::RGBA16F:	   return 4 * 2;
		case TextureFormat::RGBA32F:	   return 4 * 4;

		case TextureFormat::DEPTH16:    	 return 2;
		case TextureFormat::DEPTH24STENCIL8: return 4;
		case TextureFormat::DEPTH32F:		 return 4;
		}

		ATN_CORE_ASSERT(false);
		return false;
	}

	inline uint32 Texture::ChannelsNum(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8:			   
		case TextureFormat::R8_SRGB:
		case TextureFormat::R32F:
		case TextureFormat::DEPTH16:
		case TextureFormat::DEPTH32F: 
			return 1;
		case TextureFormat::RG8:
		case TextureFormat::RG8_SRGB:
		case TextureFormat::RG16F:
		case TextureFormat::DEPTH24STENCIL8: 
			return 2;
		case TextureFormat::RGB8:
		case TextureFormat::RGB8_SRGB:	
		case TextureFormat::R11G11B10F:	 
		case TextureFormat::RGB16F:	
		case TextureFormat::RGB32F:
			return 3;
		case TextureFormat::RGBA8:	
		case TextureFormat::RGBA8_SRGB:
		case TextureFormat::RGBA16F:
		case TextureFormat::RGBA32F:
			return 4;
		}

		ATN_CORE_ASSERT(false);
		return false;
	}
}
