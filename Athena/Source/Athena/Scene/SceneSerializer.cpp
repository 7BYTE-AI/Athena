#include "atnpch.h"
#include "SceneSerializer.h"
#include "Components.h"
#include "Entity.h"

#if defined(_MSC_VER)
	#pragma warning (push, 0)
#endif

#include <yaml-cpp/yaml.h>

#if defined(_MSC_VER)
	#pragma warning (pop)
#endif

#include <fstream>
#include <sstream>


namespace YAML
{
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

	Emitter& operator<<(Emitter& out, const Athena::Vector2& vec2)
	{
		out << Flow;
		out << BeginSeq << vec2.x << vec2.y << EndSeq;

		return out;
	}

	Emitter& operator<<(Emitter& out, const Athena::Vector3& vec3)
	{
		out << Flow;
		out << BeginSeq << vec3.x << vec3.y << vec3.z << EndSeq;

		return out;
	}

	Emitter& operator<<(Emitter& out, const Athena::LinearColor& color)
	{
		out << Flow;
		out << BeginSeq << color.r << color.g << color.b << color.a << EndSeq;

		return out;
	}
}


namespace Athena
{
	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{

	}

	void SceneSerializer::SerializeToFile(const String& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Scene Name";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
			{
				Entity entity = { entityID, m_Scene.get() };
				if (!entity)
					return;

				SerializeEntity(out, entity);
			});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const String& filepath)
	{
		ATN_CORE_ASSERT(false, "No Implemented");
	}

	bool SceneSerializer::DeserializeFromFile(const String& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data;
		try
		{
			data = YAML::Load(strStream.str());
		}
		catch (const YAML::ParserException& ex)
		{
			ATN_CORE_ERROR("Failed to deserialize scene '{0}'\n {1}", filepath, ex.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		String sceneName = data["Scene"].as<String>();

		auto& entities = data["Entities"];
		if (entities)
		{
			for (auto entityNode : entities)
			{
				uint64 uuid = 0;
				{
					auto& uuidComponentNode = entityNode["IDComponent"];
					if (uuidComponentNode)
						uuid = uuidComponentNode["ID"].as<uint64>();
				}

				String name;
				{
					auto& tagComponentNode = entityNode["TagComponent"];
					if (tagComponentNode)
						name = tagComponentNode["Tag"].as<String>();
				}

				Entity deserializedEntity = m_Scene->CreateEntity(name, uuid);

				{
					auto& transformComponentNode = entityNode["TransformComponent"];
					if (transformComponentNode)
					{
						auto& transform = deserializedEntity.GetComponent<TransformComponent>();
						transform.Translation = transformComponentNode["Translation"].as<Vector3>();
						transform.Rotation = transformComponentNode["Rotation"].as<Vector3>();
						transform.Scale = transformComponentNode["Scale"].as<Vector3>();
					}
				}

				{
					auto& cameraComponentNode = entityNode["CameraComponent"];
					if (cameraComponentNode)
					{
						auto& cc = deserializedEntity.AddComponent<CameraComponent>();
						auto& cameraPropsNode = cameraComponentNode["Camera"];

						cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraPropsNode["ProjectionType"].as<int>());

						SceneCamera::PerspectiveDESC perspectiveDesc;
						perspectiveDesc.VerticalFOV = cameraPropsNode["PerspectiveFOV"].as<float>();
						perspectiveDesc.NearClip = cameraPropsNode["PerspectiveNearClip"].as<float>();
						perspectiveDesc.FarClip = cameraPropsNode["PerspectiveFarClip"].as<float>();
						cc.Camera.SetPerspectiveData(perspectiveDesc);

						SceneCamera::OrthographicDESC orthoDesc;
						orthoDesc.Size = cameraPropsNode["OrthographicSize"].as<float>();
						orthoDesc.NearClip = cameraPropsNode["OrthographicNearClip"].as<float>();
						orthoDesc.FarClip = cameraPropsNode["OrthographicFarClip"].as<float>();
						cc.Camera.SetOrthographicData(orthoDesc);

						cc.Primary = cameraComponentNode["Primary"].as<bool>();
						cc.FixedAspectRatio = cameraComponentNode["FixedAspectRatio"].as<bool>();
					}
				}

				{
					auto& spriteComponentNode = entityNode["SpriteComponent"];
					if (spriteComponentNode)
					{
						auto& sprite = deserializedEntity.AddComponent<SpriteComponent>();

						sprite.Color = spriteComponentNode["Color"].as<LinearColor>();

						std::array<Vector2, 4> texCoords;
						auto& texCoordsNode = spriteComponentNode["TexCoords"];
						texCoords[0] = texCoordsNode["0"].as<Vector2>();
						texCoords[1] = texCoordsNode["1"].as<Vector2>();
						texCoords[2] = texCoordsNode["2"].as<Vector2>();
						texCoords[3] = texCoordsNode["3"].as<Vector2>();

						auto& textureNode = spriteComponentNode["Texture"];
						const auto& path = textureNode.as<String>();
						if (!path.empty())
						{
							Ref<Texture2D> texture = Texture2D::Create(path);
							sprite.Texture = Texture2DInstance(texture, texCoords);
						}
						else
						{
							sprite.Texture.SetTexCoords(texCoords);
						}

						sprite.TilingFactor = spriteComponentNode["TilingFactor"].as<float>();
					}
				}

				{
					auto& circleComponentNode = entityNode["CircleComponent"];
					if (circleComponentNode)
					{
						auto& circle = deserializedEntity.AddComponent<CircleComponent>();

						circle.Color = circleComponentNode["Color"].as<LinearColor>();
						circle.Thickness = circleComponentNode["Thickness"].as<float>();
						circle.Fade = circleComponentNode["Fade"].as<float>();
					}
				}

				{
					auto& rigidbody2DComponentNode = entityNode["Rigidbody2DComponent"];
					if (rigidbody2DComponentNode)
					{
						auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();

						rb2d.Type = (Rigidbody2DComponent::BodyType)rigidbody2DComponentNode["BodyType"].as<int>();
						rb2d.FixedRotation = rigidbody2DComponentNode["FixedRotation"].as<bool>();
					}
				}

				{
					auto& boxCollider2DComponentNode = entityNode["BoxCollider2DComponent"];
					if (boxCollider2DComponentNode)
					{
						auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();

						bc2d.Offset = boxCollider2DComponentNode["Offset"].as<Vector2>();
						bc2d.Size = boxCollider2DComponentNode["Size"].as<Vector2>();

						bc2d.Density = boxCollider2DComponentNode["Density"].as<float>();
						bc2d.Friction = boxCollider2DComponentNode["Friction"].as<float>();
						bc2d.Restitution = boxCollider2DComponentNode["Restitution"].as<float>();
						bc2d.RestitutionThreshold = boxCollider2DComponentNode["RestitutionThreshold"].as<float>();
					}
				}

				{
					auto& circleCollider2DComponentNode = entityNode["CircleCollider2DComponent"];
					if (circleCollider2DComponentNode)
					{
						auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();

						cc2d.Offset = circleCollider2DComponentNode["Offset"].as<Vector2>();
						cc2d.Radius = circleCollider2DComponentNode["Radius"].as<float>();

						cc2d.Density = circleCollider2DComponentNode["Density"].as<float>();
						cc2d.Friction = circleCollider2DComponentNode["Friction"].as<float>();
						cc2d.Restitution = circleCollider2DComponentNode["Restitution"].as<float>();
						cc2d.RestitutionThreshold = circleCollider2DComponentNode["RestitutionThreshold"].as<float>();
					}
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const String& filepath)
	{
		ATN_CORE_ASSERT(false, "No Implemented");
		return false;
	}

	template <typename Component, typename Func>
	static void SerializeComponent(YAML::Emitter& out, const String& name, Entity entity, Func serialize)
	{
		if (entity.HasComponent<Component>())
		{
			out << YAML::Key << name;
			out << YAML::BeginMap;

			serialize(out, entity.GetComponent<Component>());

			out << YAML::EndMap;
		}
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		if (!entity.HasComponent<IDComponent>() && !entity.HasComponent<TagComponent>())
		{
			ATN_CORE_ERROR("Entity cannot been serialized(does not have UUIDComponent and TagComponent)");
			return;
		}

		out << YAML::BeginMap; // Entity

		SerializeComponent<IDComponent>(out, "IDComponent", entity, [](YAML::Emitter& output, const IDComponent& id) 
			{
				output << YAML::Key << "ID" << YAML::Value << (uint64)id.ID;
			});

		SerializeComponent<TagComponent>(out, "TagComponent", entity, 
			[](YAML::Emitter& output, const TagComponent& tag) 
			{
				output << YAML::Key << "Tag" << YAML::Value << tag.Tag;
			});

		SerializeComponent<TransformComponent>(out, "TransformComponent", entity,
			[](YAML::Emitter& output, const TransformComponent& transform)
			{
				output << YAML::Key << "Translation" << YAML::Value << transform.Translation;
				output << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
				output << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			});

		SerializeComponent<CameraComponent>(out, "CameraComponent", entity,
			[](YAML::Emitter& output, const CameraComponent& cameraComponent)
			{
				const auto& camera = cameraComponent.Camera;
				
				const auto& perspectiveData = camera.GetPerspectiveData();
				const auto& orthoData = camera.GetOrthographicData();

				output << YAML::Key << "Camera" << YAML::Value;
				output << YAML::BeginMap;
				output << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
				output << YAML::Key << "PerspectiveFOV" << YAML::Value << perspectiveData.VerticalFOV;
				output << YAML::Key << "PerspectiveNearClip" << YAML::Value << perspectiveData.NearClip;
				output << YAML::Key << "PerspectiveFarClip" << YAML::Value << perspectiveData.FarClip;
				output << YAML::Key << "OrthographicSize" << YAML::Value << orthoData.Size;
				output << YAML::Key << "OrthographicNearClip" << YAML::Value << orthoData.NearClip;
				output << YAML::Key << "OrthographicFarClip" << YAML::Value << orthoData.FarClip;
				output << YAML::EndMap;

				output << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
				output << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;
			});

		SerializeComponent<SpriteComponent>(out, "SpriteComponent", entity,
			[](YAML::Emitter& output, const SpriteComponent& sprite)
			{
				output << YAML::Key << "Color" << YAML::Value << sprite.Color;
				output << YAML::Key << "Texture" << YAML::Value << sprite.Texture.GetNativeTexture()->GetFilepath();

				const auto& texCoords = sprite.Texture.GetTexCoords();
				output << YAML::Key << "TexCoords" << YAML::Value;
				output << YAML::BeginMap;
				output << YAML::Key << "0" << YAML::Value << texCoords[0];
				output << YAML::Key << "1" << YAML::Value << texCoords[1];
				output << YAML::Key << "2" << YAML::Value << texCoords[2];
				output << YAML::Key << "3" << YAML::Value << texCoords[3];
				output << YAML::EndMap;

				output << YAML::Key << "TilingFactor" << YAML::Value << sprite.TilingFactor;
			});

		SerializeComponent<CircleComponent>(out, "CircleComponent", entity, [](YAML::Emitter& output, const CircleComponent& circle)
			{
				output << YAML::Key << "Color" << YAML::Value << circle.Color;
				output << YAML::Key << "Thickness" << YAML::Value << circle.Thickness;
				output << YAML::Key << "Fade" << YAML::Value << circle.Fade;
			});

		SerializeComponent<Rigidbody2DComponent>(out, "Rigidbody2DComponent", entity,
			[](YAML::Emitter& output, const Rigidbody2DComponent& rb2d)
			{
				output << YAML::Key << "BodyType" << YAML::Value << (int)rb2d.Type;
				output << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;
			});

		SerializeComponent<BoxCollider2DComponent>(out, "BoxCollider2DComponent", entity,
			[](YAML::Emitter& output, const BoxCollider2DComponent& bc2d)
			{
				output << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
				output << YAML::Key << "Size" << YAML::Value << bc2d.Size;

				output << YAML::Key << "Density" << YAML::Value << bc2d.Density;
				output << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
				output << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
				output << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;
			});

		SerializeComponent<CircleCollider2DComponent>(out, "CircleCollider2DComponent", entity,
			[](YAML::Emitter& output, const CircleCollider2DComponent& cc2d)
			{
				output << YAML::Key << "Offset" << YAML::Value << cc2d.Offset;
				output << YAML::Key << "Radius" << YAML::Value << cc2d.Radius;

				output << YAML::Key << "Density" << YAML::Value << cc2d.Density;
				output << YAML::Key << "Friction" << YAML::Value << cc2d.Friction;
				output << YAML::Key << "Restitution" << YAML::Value << cc2d.Restitution;
				output << YAML::Key << "RestitutionThreshold" << YAML::Value << cc2d.RestitutionThreshold;
			});

		out << YAML::EndMap; // Entity
	}
}