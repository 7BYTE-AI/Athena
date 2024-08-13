#pragma once

#include "Athena/Core/Core.h"

#include <unordered_map>


namespace Athena
{
	enum class ScriptFieldType
	{
		None = 0,
		Bool, Char, Byte,
		Int16, Int32, Int64,
		UInt16, UInt32, UInt64,
		Float, Double,
		Vector2, Vector3, Vector4,
	};

	// Default case to trigger a compile-time error for unsupported types
	template <typename T>
	struct ScriptFieldTypeMapper
	{
		static_assert(!std::is_same_v<T, T>, "Not supported type for script field!");
	};

	template <>
	struct ScriptFieldTypeMapper<bool>    { static constexpr ScriptFieldType Value = ScriptFieldType::Bool; };

	template <>
	struct ScriptFieldTypeMapper<char>    { static constexpr ScriptFieldType Value = ScriptFieldType::Char; };

	template <>
	struct ScriptFieldTypeMapper<byte>    { static constexpr ScriptFieldType Value = ScriptFieldType::Byte; };

	template <>
	struct ScriptFieldTypeMapper<int16>   { static constexpr ScriptFieldType Value = ScriptFieldType::Int16; };

	template <>
	struct ScriptFieldTypeMapper<int32>   { static constexpr ScriptFieldType Value = ScriptFieldType::Int32; };

	template <>
	struct ScriptFieldTypeMapper<int64>   { static constexpr ScriptFieldType Value = ScriptFieldType::Int64; };

	template <>
	struct ScriptFieldTypeMapper<uint16>  { static constexpr ScriptFieldType Value = ScriptFieldType::UInt16; };

	template <>
	struct ScriptFieldTypeMapper<uint32>  { static constexpr ScriptFieldType Value = ScriptFieldType::UInt32; };

	template <>
	struct ScriptFieldTypeMapper<uint64>  { static constexpr ScriptFieldType Value = ScriptFieldType::UInt64; };

	template <>
	struct ScriptFieldTypeMapper<float>   { static constexpr ScriptFieldType Value = ScriptFieldType::Float; };

	template <>
	struct ScriptFieldTypeMapper<double>  { static constexpr ScriptFieldType Value = ScriptFieldType::Double; };

	template <>
	struct ScriptFieldTypeMapper<Vector2> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector2; };

	template <>
	struct ScriptFieldTypeMapper<Vector3> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector3; };

	template <>
	struct ScriptFieldTypeMapper<Vector4> { static constexpr ScriptFieldType Value = ScriptFieldType::Vector4; };

	template <typename T>
	inline constexpr ScriptFieldType ScriptFieldTypeMapper_V = ScriptFieldTypeMapper<T>::Value;


	class ScriptFieldStorage
	{
	public:
		friend class ScriptInstance;
		friend class ScriptClass;

		ScriptFieldStorage()
		{
			memset(m_Buffer, 0, sizeof(m_Buffer));
		}

		template <typename T>
		ScriptFieldStorage(T* ref)
		{
			m_FieldReference = ref;

			m_Type = ScriptFieldTypeMapper_V<T>;
			m_Size = GetScriptFieldSize(m_Type);
			memcpy(m_Buffer, ref, m_Size);
		}

		ScriptFieldStorage(const ScriptFieldStorage& other)
		{
			m_Type = other.m_Type;
			m_FieldReference = other.m_FieldReference;
			m_Size = other.m_Size;
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
		}

		ScriptFieldStorage& operator=(const ScriptFieldStorage& other)
		{
			m_Type = other.m_Type;
			m_FieldReference = other.m_FieldReference;
			m_Size = other.m_Size;
			memcpy(m_Buffer, other.m_Buffer, sizeof(m_Buffer));
			return *this;
		}

		template<typename T>
		T GetValue() const
		{
			ScriptFieldTypeMapper_V<T>;
			return *(T*)m_Buffer;
		}

		template<typename T>
		T GetValue()
		{
			ScriptFieldTypeMapper_V<T>;

			if (m_FieldReference != nullptr)
				memcpy(m_Buffer, m_FieldReference, m_Size);

			return *(T*)m_Buffer;
		}

		template<typename T>
		void SetValue(T value)
		{
			ScriptFieldTypeMapper_V<T>;

			memcpy(m_Buffer, &value, sizeof(T));

			if (m_FieldReference != nullptr)
				memcpy(m_FieldReference, m_Buffer, m_Size);
		}

		ScriptFieldType GetType() const
		{
			return m_Type;
		}

		const void* GetBuffer() const
		{
			return m_Buffer;
		}

		void SetFieldReference(void* ref, bool write = true)
		{
			m_FieldReference = ref;

			if(write)
				memcpy(m_FieldReference, m_Buffer, m_Size);
			else
				memcpy(m_Buffer, m_FieldReference, m_Size);
		}

		void RemoveFieldReference()
		{
			m_FieldReference = nullptr;
		}

		void* GetFieldReference()
		{
			return m_FieldReference;
		}

	private:
		uint32 GetScriptFieldSize(ScriptFieldType type)
		{
			switch (type)
			{
			case ScriptFieldType::Bool:  return 1;
			case ScriptFieldType::Char:  return 1;
			case ScriptFieldType::Byte:  return 1;

			case ScriptFieldType::Int16: return 2;
			case ScriptFieldType::Int32: return 4;
			case ScriptFieldType::Int64: return 8;
			case ScriptFieldType::UInt16: return 2;
			case ScriptFieldType::UInt32: return 4;
			case ScriptFieldType::UInt64: return 8;

			case ScriptFieldType::Float: return 4;
			case ScriptFieldType::Double: return 8;

			case ScriptFieldType::Vector2: return 8;
			case ScriptFieldType::Vector3: return 12;
			case ScriptFieldType::Vector4: return 16;
			}

			ATN_CORE_ASSERT(false);
			return 0;
		}

	private:
		ScriptFieldType m_Type = ScriptFieldType::None;
		void* m_FieldReference = nullptr;
		uint32 m_Size = 0;
		byte m_Buffer[16];
	};

	using ScriptFieldMap = std::unordered_map<String, ScriptFieldStorage>;


	namespace Utils 
	{
		inline const char* ScriptFieldTypeToString(ScriptFieldType fieldType)
		{
			switch (fieldType)
			{
			case ScriptFieldType::None:    return "None";
			case ScriptFieldType::Bool:    return "Bool";
			case ScriptFieldType::Char:    return "Char";
			case ScriptFieldType::Byte:    return "Byte";
			case ScriptFieldType::Int16:   return "Int16";
			case ScriptFieldType::Int32:   return "Int32";
			case ScriptFieldType::Int64:   return "Int64";
			case ScriptFieldType::UInt16:  return "UInt16";
			case ScriptFieldType::UInt32:  return "UInt32";
			case ScriptFieldType::UInt64:  return "UInt64";
			case ScriptFieldType::Float:   return "Float";
			case ScriptFieldType::Double:  return "Double";
			case ScriptFieldType::Vector2: return "Vector2";
			case ScriptFieldType::Vector3: return "Vector3";
			case ScriptFieldType::Vector4: return "Vector4";
			}

			ATN_CORE_ASSERT(false, "Unknown ScriptFieldType");
			return "None";
		}

		inline ScriptFieldType ScriptFieldTypeFromString(std::string_view fieldType)
		{
			if (fieldType == "None")    return ScriptFieldType::None;
			if (fieldType == "Bool")    return ScriptFieldType::Bool;
			if (fieldType == "Char")    return ScriptFieldType::Char;
			if (fieldType == "Byte")    return ScriptFieldType::Byte;
			if (fieldType == "Int16")   return ScriptFieldType::Int16;
			if (fieldType == "Int32")   return ScriptFieldType::Int32;
			if (fieldType == "Int64")   return ScriptFieldType::Int64;
			if (fieldType == "UInt16")  return ScriptFieldType::UInt16;
			if (fieldType == "UInt32")  return ScriptFieldType::UInt32;
			if (fieldType == "UInt64")  return ScriptFieldType::UInt64;
			if (fieldType == "Float")   return ScriptFieldType::Float;
			if (fieldType == "Double")  return ScriptFieldType::Double;
			if (fieldType == "Vector2") return ScriptFieldType::Vector2;
			if (fieldType == "Vector3") return ScriptFieldType::Vector3;
			if (fieldType == "Vector4") return ScriptFieldType::Vector4;

			ATN_CORE_ASSERT(false, "Unknown ScriptFieldType");
			return ScriptFieldType::None;
		}
	}
}
