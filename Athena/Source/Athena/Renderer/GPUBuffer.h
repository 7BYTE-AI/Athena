#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Utils/StringUtils.h"
#include "Athena/Renderer/RenderResource.h"


namespace Athena
{
	enum class ShaderDataType : uint8
	{
		Unknown = 0, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, UInt, Mat3, Mat4
	};

	constexpr uint32 ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return 4;
			case ShaderDataType::Float2: return 4 * 2;
			case ShaderDataType::Float3: return 4 * 3;
			case ShaderDataType::Float4: return 4 * 4;
			case ShaderDataType::Int:    return 4;
			case ShaderDataType::Int2:   return 4 * 2;
			case ShaderDataType::Int3:   return 4 * 3;
			case ShaderDataType::Int4:   return 4 * 4;
			case ShaderDataType::UInt:   return 4;
			case ShaderDataType::Mat3:   return 4 * 9;
			case ShaderDataType::Mat4:   return 4 * 16;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	constexpr std::string_view ShaderDataTypeToString(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:  return "float";
		case ShaderDataType::Float2: return "float2";
		case ShaderDataType::Float3: return "float3";
		case ShaderDataType::Float4: return "float4";
		case ShaderDataType::Int:    return "int";
		case ShaderDataType::Int2:   return "int2";
		case ShaderDataType::Int3:   return "int3";
		case ShaderDataType::Int4:   return "int4";
		case ShaderDataType::UInt:   return "uint";
		case ShaderDataType::Mat3:   return "mat3";
		case ShaderDataType::Mat4:   return "mat4";
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return "";
	}


	struct VertexElement
	{
		String Name;
		ShaderDataType Type;
		uint32 Size;
		uint32 Offset;

		VertexElement() = default;
		VertexElement(ShaderDataType type, const String& name)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0) {}

		uint32 GetComponentCount() const
		{
			switch (Type)
			{
				case ShaderDataType::Float: return 1;
				case ShaderDataType::Float2: return 2;
				case ShaderDataType::Float3: return 3;
				case ShaderDataType::Float4: return 4;
				case ShaderDataType::Mat3: return 9;
				case ShaderDataType::Mat4: return 16;
				case ShaderDataType::Int: return 1;
				case ShaderDataType::Int2: return 2;
				case ShaderDataType::Int3: return 3;
				case ShaderDataType::Int4: return 4;
				case ShaderDataType::UInt: return 4;
			}

			ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};


	class ATHENA_API VertexMemoryLayout
	{
	public:
		using iterator = std::vector<VertexElement>::iterator;
		using const_iterator = std::vector<VertexElement>::const_iterator;

	public:
		VertexMemoryLayout() = default;

		VertexMemoryLayout(const std::initializer_list<VertexElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		VertexMemoryLayout(const std::vector<VertexElement>& elements)
			: m_Elements(elements) 
		{
			CalculateOffsetsAndStride();
		}

		uint32 GetStride() const { return m_Stride; }

		const std::vector<VertexElement>& GetElements() const { return m_Elements; }
		uint32 GetElementsNum() const { return m_Elements.size(); }

		iterator	   begin()	     { return m_Elements.begin(); }
		const_iterator begin() const { return m_Elements.begin(); }
		iterator	   end()		 { return m_Elements.end(); }
		const_iterator end()   const { return m_Elements.end(); }

	private:
		void CalculateOffsetsAndStride()
		{
			uint32 offset = 0;
			for (auto& elem: m_Elements)
			{
				elem.Offset = offset;
				offset += elem.Size;
				m_Stride += elem.Size;
			}
		}

	private:
		std::vector<VertexElement> m_Elements;
		uint32 m_Stride = 0;
	};

	enum class BufferMemoryFlags
	{
		GPU_ONLY = 0,
		CPU_WRITEABLE,
		// TODO: add CPU_READABLE 
	};

	struct IndexBufferCreateInfo
	{
		String Name;
		const void* Data = nullptr;
		uint32 Count = 0;
		BufferMemoryFlags Flags = BufferMemoryFlags::GPU_ONLY;
	};

	class ATHENA_API IndexBuffer : public RefCounted
	{
	public:
		static Ref<IndexBuffer> Create(const IndexBufferCreateInfo& info);
		virtual ~IndexBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;
		virtual void Resize(uint64 size) = 0;

		uint32 GetCount() const { return m_Info.Count; }
		uint32 GetSize() const { return m_Info.Count * sizeof(uint32); };
		const String& GetName() const { return m_Info.Name; }
		const IndexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		IndexBufferCreateInfo m_Info;
	};

	struct VertexBufferCreateInfo
	{
		String Name;
		const void* Data = nullptr;
		uint64 Size = 0;
		Ref<IndexBuffer> IndexBuffer;
		BufferMemoryFlags Flags = BufferMemoryFlags::GPU_ONLY;
	};

	class ATHENA_API VertexBuffer : public RefCounted
	{
	public:
		static Ref<VertexBuffer> Create(const VertexBufferCreateInfo& info);
		virtual ~VertexBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;
		virtual void Resize(uint64 size) = 0;

		uint64 GetSize() const { return m_Info.Size; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_Info.IndexBuffer; }
		const String& GetName() const { return m_Info.Name; }
		const VertexBufferCreateInfo& GetInfo() const { return m_Info; }

	protected:
		VertexBufferCreateInfo m_Info;
	};


	class ATHENA_API UniformBuffer : public RenderResource
	{
	public:
		static Ref<UniformBuffer> Create(const String& name, uint64 size);
		virtual ~UniformBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::UniformBuffer; }
		virtual const String& GetName() const override { return m_Name; }

		uint64 GetSize() const { return m_Size; }

	protected:
		String m_Name;
		uint64 m_Size;
	};


	class ATHENA_API StorageBuffer : public RenderResource
	{
	public:
		static Ref<StorageBuffer> Create(const String& name, uint64 size, BufferMemoryFlags flags);
		virtual ~StorageBuffer() = default;

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) = 0;
		virtual void Resize(uint64 size) = 0;

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::StorageBuffer; }
		virtual const String& GetName() const override { return m_Name; }

		uint64 GetSize() const { return m_Size; };

	protected:
		String m_Name;
		uint64 m_Size;
	};

	template <typename T>
	class DynamicGPUBuffer
	{
	public:
		DynamicGPUBuffer() = default;
		DynamicGPUBuffer(const Ref<T>& buffer)
			: m_GPUBuffer(buffer) {}

		void Push(const void* data, uint64 size)
		{
			if (size == 0)
				return;

			uint64 newOffset = m_DataOffset + size;

			if (m_CPUBuffer.size() < newOffset)
			{
				uint64 newSize = newOffset * 2.f;
				m_CPUBuffer.resize(newSize);
			}

			auto begin = m_CPUBuffer.begin() + m_DataOffset;
			memcpy(&(*begin), data, size);

			m_DataOffset = newOffset;
		}

		void Flush()
		{
			if (m_DataOffset == 0)
				return;

			// Increase size if not enough
			if (m_GPUBuffer->GetSize() < m_DataOffset)
			{
				uint64 newSize = m_DataOffset * 2.f;

				ATN_CORE_WARN_TAG("Renderer", "{} allocating from {} to {}", m_GPUBuffer->GetName(), 
					Utils::MemoryBytesToString(m_GPUBuffer->GetSize()), Utils::MemoryBytesToString(newSize));

				m_GPUBuffer->Resize(newSize);
			}

			// TODO: Decrease size when too much allocated

			m_GPUBuffer->UploadData(m_CPUBuffer.data(), m_DataOffset);
			m_DataOffset = 0;
		}

		// Only for StorageBuffer and UniformBuffer
		operator Ref<RenderResource>() const 
		{ 
			return m_GPUBuffer.As<RenderResource>(); 
		}

		Ref<T> Get() const
		{
			return m_GPUBuffer;
		}

	private:
		Ref<T> m_GPUBuffer;
		std::vector<byte> m_CPUBuffer;
		uint64 m_DataOffset = 0;
	};
}
