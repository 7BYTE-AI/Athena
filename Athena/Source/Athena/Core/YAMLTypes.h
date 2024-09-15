#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/Quaternion.h"
#include "Athena/Renderer/Color.h"

#if defined(_MSC_VER)
#pragma warning (push, 0)
#endif

#include <yaml-cpp/yaml.h>

#if defined(_MSC_VER)
#pragma warning (pop)
#endif


namespace YAML
{
	template <>
	struct convert<Athena::FilePath>
	{
		static Node encode(const Athena::FilePath& path)
		{
			Node node;
			node.push_back(path.string());
			return node;
		}

		static bool decode(const Node& node, Athena::FilePath& path)
		{
			path = node.as<std::string>();
			return true;
		}
	};

	template <>
	struct convert<Athena::UUID>
	{
		static Node encode(const Athena::UUID& uuid)
		{
			Node node;
			node.push_back((Athena::uint64)uuid);
			return node;
		}

		static bool decode(const Node& node, Athena::UUID& uuid)
		{
			uuid = node.as<Athena::uint64>();
			return true;
		}
	};

	template <>
	struct convert<Athena::Vector2>
	{
		static Node encode(const Athena::Vector2& vec2)
		{
			Node node;
			node.push_back(vec2.x);
			node.push_back(vec2.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Vector2& vec2)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			vec2.x = node[0].as<float>();
			vec2.y = node[1].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::Vector3>
	{
		static Node encode(const Athena::Vector3& vec3)
		{
			Node node;
			node.push_back(vec3.x);
			node.push_back(vec3.y);
			node.push_back(vec3.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Vector3& vec3)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			vec3.x = node[0].as<float>();
			vec3.y = node[1].as<float>();
			vec3.z = node[2].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::Vector4>
	{
		static Node encode(const Athena::Vector4& vec4)
		{
			Node node;
			node.push_back(vec4.x);
			node.push_back(vec4.y);
			node.push_back(vec4.z);
			node.push_back(vec4.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Vector4& vec4)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			vec4.x = node[0].as<float>();
			vec4.y = node[1].as<float>();
			vec4.z = node[2].as<float>();
			vec4.w = node[3].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::Quaternion>
	{
		static Node encode(const Athena::Quaternion& quat)
		{
			Node node;
			node.push_back(quat.w);
			node.push_back(quat.x);
			node.push_back(quat.y);
			node.push_back(quat.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Quaternion& quat)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			quat.w = node[0].as<float>();
			quat.x = node[1].as<float>();
			quat.y = node[2].as<float>();
			quat.z = node[3].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::LinearColor>
	{
		static Node encode(const Athena::LinearColor& color)
		{
			Node node;
			node.push_back(color.r);
			node.push_back(color.g);
			node.push_back(color.b);
			node.push_back(color.a);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::LinearColor& color)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			color.r = node[0].as<float>();
			color.g = node[1].as<float>();
			color.b = node[2].as<float>();
			color.a = node[3].as<float>();
			return true;
		}
	};

	inline Emitter& operator<<(Emitter& out, const Athena::UUID& uuid)
	{
		out << (Athena::uint64)uuid;
		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::FilePath& path)
	{
		out << path.string();
		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::Vector2& vec2)
	{
		out << Flow;
		out << BeginSeq << vec2.x << vec2.y << EndSeq;

		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::Vector3& vec3)
	{
		out << Flow;
		out << BeginSeq << vec3.x << vec3.y << vec3.z << EndSeq;

		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::Vector4& vec4)
	{
		out << Flow;
		out << BeginSeq << vec4.x << vec4.y << vec4.z << vec4.w << EndSeq;

		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::Quaternion& quat)
	{
		out << Flow;
		out << BeginSeq << quat.w << quat.x << quat.y << quat.z << EndSeq;

		return out;
	}

	inline Emitter& operator<<(Emitter& out, const Athena::LinearColor& color)
	{
		out << Flow;
		out << BeginSeq << color.r << color.g << color.b << color.a << EndSeq;

		return out;
	}
}
