#include "Athena/Scene/SceneSerializer.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scene/Entity.h"

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
#define WRITE_SCRIPT_FIELD(nativeType, fieldType)   \
		case ScriptFieldType::fieldType:		    \
			output << field.GetValue<nativeType>(); \
			break								  
	
#define READ_SCRIPT_FIELD(nativeType, fieldType)						 \
		case ScriptFieldType::fieldType:								 \
		{																 \
			nativeType data = scriptFieldNode["Data"].as<nativeType>();  \
			field.SetValue(data);										 \
			break;														 \
		}


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

	Emitter& operator<<(Emitter& out, const Athena::Vector4& vec4)
	{
		out << Flow;
		out << BeginSeq << vec4.x << vec4.y << vec4.z << vec4.w << EndSeq;

		return out;
	}

	Emitter& operator<<(Emitter& out, const Athena::Quaternion& quat)
	{
		out << Flow;
		out << BeginSeq << quat.w << quat.x << quat.y << quat.z << EndSeq;

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

	void SceneSerializer::SerializeToFile(const FilePath& path)
	{
		ATN_PROFILE_FUNC();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetSceneName();
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
			{
				Entity entity = { entityID, m_Scene.Raw() };
				if (!entity)
					return;

				SerializeEntity(out, entity);
			});

		out << YAML::EndSeq;

		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
	}

	bool SceneSerializer::DeserializeFromFile(const FilePath& path)
	{
		ATN_PROFILE_FUNC();

		if (!FileSystem::Exists(path))
		{
			ATN_CORE_ERROR_TAG("SceneSerializer", "Invalid scene filepath {}", path);
			return false;
		}

		std::ifstream stream(path);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data;
		try
		{
			data = YAML::Load(strStream.str());

			if (!data["Scene"])
			{
				ATN_CORE_ERROR_TAG("SceneSerializer", "Failed to deserialize scene {0}", path);
				return false;
			}

			String sceneName = data["Scene"].as<String>();
			m_Scene->SetSceneName(sceneName);

			const auto& entities = data["Entities"];
			if (!entities)
				return false;

			for (const auto& entityNode : entities)
			{
				uint64 uuid = 0;
				{
					const auto& uuidComponentNode = entityNode["IDComponent"];
					if (uuidComponentNode)
						uuid = uuidComponentNode["ID"].as<uint64>();
				}

				String name;
				{
					const auto& tagComponentNode = entityNode["TagComponent"];
					if (tagComponentNode)
						name = tagComponentNode["Tag"].as<String>();
				}

				Entity deserializedEntity = m_Scene->CreateEntity(name, uuid);

				{
					const auto& transformComponentNode = entityNode["TransformComponent"];
					if (transformComponentNode)
					{
						auto& transform = deserializedEntity.GetComponent<TransformComponent>();
						transform.Translation = transformComponentNode["Translation"].as<Vector3>();
						transform.Rotation = transformComponentNode["Rotation"].as<Quaternion>();
						transform.Scale = transformComponentNode["Scale"].as<Vector3>();
					}
				}

				{
					const auto& scriptComponentNode = entityNode["ScriptComponent"];
					if (scriptComponentNode)
					{
						auto& script = deserializedEntity.AddComponent<ScriptComponent>();
						script.Name = scriptComponentNode["ScriptName"].as<String>();

						const YAML::Node& scriptFieldsNode = scriptComponentNode["ScriptFields"];
						if (scriptFieldsNode && ScriptEngine::IsScriptExists(script.Name))
						{
							ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(deserializedEntity);

							for (const YAML::Node& scriptFieldNode : scriptFieldsNode)
							{
								String name = scriptFieldNode["Name"].as<std::string>();
								std::string typeString = scriptFieldNode["Type"].as<std::string>();
								ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

								if (!fieldMap || !fieldMap->contains(name))
								{
									ATN_CORE_WARN_TAG("Deserializer", "Uknown script field name '{}' (field will be discarded, script name - {}, entity name - {})!", 
										name, script.Name, deserializedEntity.GetName());
									continue;
								}

								ScriptFieldStorage& field = fieldMap->at(name);

								if (type != field.GetType())
								{
									ATN_CORE_WARN_TAG("Deserializer", "Script field type '{}' does not match with original type '{}' (field will be discarded, script name - {}, entity name - {})!", 
										Utils::ScriptFieldTypeToString(type), Utils::ScriptFieldTypeToString(field.GetType()), script.Name, deserializedEntity.GetName());
									continue;
								}

								switch (type)
								{
								READ_SCRIPT_FIELD(bool,   Bool);
								READ_SCRIPT_FIELD(char,   Char);
								READ_SCRIPT_FIELD(byte,   Byte);
								READ_SCRIPT_FIELD(int16,  Int16);
								READ_SCRIPT_FIELD(int32,  Int32);
								READ_SCRIPT_FIELD(int64,  Int64);
								READ_SCRIPT_FIELD(uint16, UInt16);
								READ_SCRIPT_FIELD(uint32, UInt32);
								READ_SCRIPT_FIELD(uint64, UInt64);
								READ_SCRIPT_FIELD(float,  Float);
								READ_SCRIPT_FIELD(double, Double);
								}
							}
						}
						else if (!ScriptEngine::IsScriptExists(script.Name))
						{
							ATN_CORE_WARN_TAG("Deserializer", "Uknown script '{}' (script and all fields will be discarded)", script.Name);
						}
					}
				}

				{
					const auto& cameraComponentNode = entityNode["CameraComponent"];
					if (cameraComponentNode)
					{
						auto& cc = deserializedEntity.AddComponent<CameraComponent>();
						const auto& cameraPropsNode = cameraComponentNode["Camera"];

						cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraPropsNode["ProjectionType"].as<int>());

						SceneCamera::PerspectiveData perspectiveData;
						perspectiveData.VerticalFOV = cameraPropsNode["PerspectiveFOV"].as<float>();
						perspectiveData.NearClip = cameraPropsNode["PerspectiveNearClip"].as<float>();
						perspectiveData.FarClip = cameraPropsNode["PerspectiveFarClip"].as<float>();
						cc.Camera.SetPerspectiveData(perspectiveData);

						SceneCamera::OrthographicData orthoData;
						orthoData.Size = cameraPropsNode["OrthographicSize"].as<float>();
						orthoData.NearClip = cameraPropsNode["OrthographicNearClip"].as<float>();
						orthoData.FarClip = cameraPropsNode["OrthographicFarClip"].as<float>();
						cc.Camera.SetOrthographicData(orthoData);

						cc.Primary = cameraComponentNode["Primary"].as<bool>();
						cc.FixedAspectRatio = cameraComponentNode["FixedAspectRatio"].as<bool>();
					}
				}

				{
					const auto& spriteComponentNode = entityNode["SpriteComponent"];
					if (spriteComponentNode)
					{
						auto& sprite = deserializedEntity.AddComponent<SpriteComponent>();

						sprite.Space = (Renderer2DSpace)spriteComponentNode["Space"].as<int>();
						sprite.Color = spriteComponentNode["Color"].as<LinearColor>();

						std::array<Vector2, 4> texCoords;
						const auto& texCoordsNode = spriteComponentNode["TexCoords"];
						texCoords[0] = texCoordsNode["0"].as<Vector2>();
						texCoords[1] = texCoordsNode["1"].as<Vector2>();
						texCoords[2] = texCoordsNode["2"].as<Vector2>();
						texCoords[3] = texCoordsNode["3"].as<Vector2>();

						const auto& textureNode = spriteComponentNode["Texture"];
						const auto& path = FilePath(textureNode.as<String>());
						if (!path.empty())
						{
							Ref<Texture2D> texture = TextureImporter::Load(path, true);
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
					const auto& circleComponentNode = entityNode["CircleComponent"];
					if (circleComponentNode)
					{
						auto& circle = deserializedEntity.AddComponent<CircleComponent>();

						circle.Space = (Renderer2DSpace)circleComponentNode["Space"].as<int>();
						circle.Color = circleComponentNode["Color"].as<LinearColor>();
						circle.Thickness = circleComponentNode["Thickness"].as<float>();
						circle.Fade = circleComponentNode["Fade"].as<float>();
					}
				}

				{
					const auto& textComponentNode = entityNode["TextComponent"];
					if (textComponentNode)
					{
						auto& text = deserializedEntity.AddComponent<TextComponent>();

						text.Text = textComponentNode["Text"].as<String>();
						text.Font = Font::Create(textComponentNode["Font"].as<String>());
						text.Space = (Renderer2DSpace)textComponentNode["Space"].as<int>();
						text.Color = textComponentNode["Color"].as<LinearColor>();
						text.MaxWidth = textComponentNode["MaxWidth"].as<float>();
						text.Kerning = textComponentNode["Kerning"].as<float>();
						text.LineSpacing = textComponentNode["LineSpacing"].as<float>();
						text.Shadowing = textComponentNode["Shadowing"].as<bool>();
						text.ShadowDistance = textComponentNode["ShadowDistance"].as<float>();
						text.ShadowColor = textComponentNode["ShadowColor"].as<LinearColor>();
					}
				}

				{
					const auto& rigidbody2DComponentNode = entityNode["Rigidbody2DComponent"];
					if (rigidbody2DComponentNode)
					{
						auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();

						rb2d.Type = (Rigidbody2DComponent::BodyType)rigidbody2DComponentNode["BodyType"].as<int>();
						rb2d.FixedRotation = rigidbody2DComponentNode["FixedRotation"].as<bool>();
					}
				}

				{
					const auto& boxCollider2DComponentNode = entityNode["BoxCollider2DComponent"];
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
					const auto& circleCollider2DComponentNode = entityNode["CircleCollider2DComponent"];
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

				{
					const auto& staticMeshComponentNode = entityNode["StaticMeshComponent"];
					if (staticMeshComponentNode)
					{
						auto& meshComp = deserializedEntity.AddComponent<StaticMeshComponent>();

						FilePath path = staticMeshComponentNode["FilePath"].as<String>();

						meshComp.Mesh = StaticMesh::Create(path);
						meshComp.Visible = staticMeshComponentNode["Visible"].as<bool>();
					}
				}

				{
					const auto directionalLightComponent = entityNode["DirectionalLightComponent"];
					if (directionalLightComponent)
					{
						auto& lightComp = deserializedEntity.AddComponent<DirectionalLightComponent>();

						lightComp.Color = directionalLightComponent["Color"].as<LinearColor>();
						lightComp.Intensity = directionalLightComponent["Intensity"].as<float>();
						lightComp.CastShadows = directionalLightComponent["CastShadows"].as<bool>();
						lightComp.LightSize = directionalLightComponent["LightSize"].as<float>();
					}
				}

				{
					const auto pointLightComponent = entityNode["PointLightComponent"];
					if (pointLightComponent)
					{
						auto& lightComp = deserializedEntity.AddComponent<PointLightComponent>();
						lightComp.Color = pointLightComponent["Color"].as<LinearColor>();
						lightComp.Intensity = pointLightComponent["Intensity"].as<float>();
						lightComp.Radius = pointLightComponent["Radius"].as<float>();
						lightComp.FallOff = pointLightComponent["FallOff"].as<float>();
					}
				}

				{
					const auto pointLightComponent = entityNode["SpotLightComponent"];
					if (pointLightComponent)
					{
						auto& lightComp = deserializedEntity.AddComponent<SpotLightComponent>();
						lightComp.Color = pointLightComponent["Color"].as<LinearColor>();
						lightComp.Intensity = pointLightComponent["Intensity"].as<float>();
						lightComp.SpotAngle = pointLightComponent["SpotAngle"].as<float>();
						lightComp.InnerFallOff = pointLightComponent["InnerFallOff"].as<float>();
						lightComp.Range = pointLightComponent["Range"].as<float>();
						lightComp.RangeFallOff = pointLightComponent["RangeFallOff"].as<float>();
					}
				}

				{
					const auto skyLightComponent = entityNode["SkyLightComponent"];
					if (skyLightComponent)
					{
						auto& lightComp = deserializedEntity.AddComponent<SkyLightComponent>();
						const auto& envMap = lightComp.EnvironmentMap;

						envMap->SetResolution(skyLightComponent["Resolution"].as<uint32>());
						envMap->SetType((EnvironmentMapType)skyLightComponent["Type"].as<uint32>());
						envMap->SetFilePath(skyLightComponent["FilePath"].as<String>());

						float turbidity = skyLightComponent["Turbidity"].as<float>();
						float azimuth = skyLightComponent["Azimuth"].as<float>();
						float inclination = skyLightComponent["Inclination"].as<float>();
						envMap->SetPreethamParams(turbidity, azimuth, inclination);

						lightComp.Intensity = skyLightComponent["Intensity"].as<float>();
						lightComp.LOD = skyLightComponent["LOD"].as<float>();
					}
				}
			}

			// Build Entity Hierarchy
			for (const auto& entityNode : entities)
			{
				uint64 uuid = 0;
				{
					const auto& uuidComponentNode = entityNode["IDComponent"];
					if (uuidComponentNode)
						uuid = uuidComponentNode["ID"].as<uint64>();
				}

				Entity entity = m_Scene->GetEntityByUUID(uuid);
				
				{
					const auto& parentComponentNode = entityNode["ParentComponent"];
					if (parentComponentNode)
					{
						UUID parentID = parentComponentNode["Parent"].as<uint64>();
						Entity parent = m_Scene->GetEntityByUUID(parentID);

						if (parent)
							m_Scene->MakeRelationship(parent, entity);
					}
				}
			}
		}
		catch (const YAML::Exception& ex)
		{
			ATN_CORE_ERROR_TAG("Serializer", "Failed to deserialize scene '{0}'\n {1}", path, ex.what());
			return false;
		}

		return true;
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
			ATN_CORE_ERROR_TAG("Serializer", "Entity cannot been serialized(does not have UUIDComponent and TagComponent)");
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

		SerializeComponent<ParentComponent>(out, "ParentComponent", entity,
			[](YAML::Emitter& output, const ParentComponent& parentCmp)
			{
				uint64 parentID = (uint64)parentCmp.Parent.GetComponent<IDComponent>().ID;
				output << YAML::Key << "Parent" << YAML::Value << parentID;
			});

		SerializeComponent<ScriptComponent>(out, "ScriptComponent", entity,
			[entity](YAML::Emitter& output, const ScriptComponent& script)
			{
				output << YAML::Key << "ScriptName" << YAML::Value << script.Name;

				const ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(entity);
				if (!fieldMap)
					return;

				output << YAML::Key << "ScriptFields" << YAML::Value;
				output << YAML::BeginSeq;

				for (const auto& [name, field] : *fieldMap)
				{
					output << YAML::BeginMap;

					output << YAML::Key << "Name" << YAML::Value << name.data();
					output << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.GetType());
					output << YAML::Key << "Data" << YAML::Value;

					switch (field.GetType())
					{
					WRITE_SCRIPT_FIELD(bool,    Bool);
					WRITE_SCRIPT_FIELD(char,    Char);
					WRITE_SCRIPT_FIELD(byte,    Byte);
					WRITE_SCRIPT_FIELD(int16,   Int16);
					WRITE_SCRIPT_FIELD(int32,   Int32);
					WRITE_SCRIPT_FIELD(int64,   Int64);
					WRITE_SCRIPT_FIELD(uint16,  UInt16);
					WRITE_SCRIPT_FIELD(uint32,  UInt32);
					WRITE_SCRIPT_FIELD(uint64,  UInt64);
					WRITE_SCRIPT_FIELD(float,   Float);
					WRITE_SCRIPT_FIELD(double,  Double);
					WRITE_SCRIPT_FIELD(Vector2, Vector2);
					WRITE_SCRIPT_FIELD(Vector3, Vector3);
					WRITE_SCRIPT_FIELD(Vector4, Vector4);
					}

					output << YAML::EndMap;
				}

				output << YAML::EndSeq;
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
				output << YAML::Key << "Space" << YAML::Value << (int)sprite.Space;
				output << YAML::Key << "Color" << YAML::Value << sprite.Color;
				output << YAML::Key << "Texture" << YAML::Value << sprite.Texture.GetNativeTexture()->GetFilePath().string();

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
				output << YAML::Key << "Space" << YAML::Value << (int)circle.Space;
				output << YAML::Key << "Color" << YAML::Value << circle.Color;
				output << YAML::Key << "Thickness" << YAML::Value << circle.Thickness;
				output << YAML::Key << "Fade" << YAML::Value << circle.Fade;
			});

		SerializeComponent<TextComponent>(out, "TextComponent", entity, [](YAML::Emitter& output, const TextComponent& text)
			{
				output << YAML::Key << "Text" << YAML::Value << text.Text;
				output << YAML::Key << "Font" << YAML::Value << text.Font->GetFilePath().string();
				output << YAML::Key << "Space" << YAML::Value << (int)text.Space;
				output << YAML::Key << "Color" << YAML::Value << text.Color;
				output << YAML::Key << "MaxWidth" << YAML::Value << text.MaxWidth;
				output << YAML::Key << "Kerning" << YAML::Value << text.Kerning;
				output << YAML::Key << "LineSpacing" << YAML::Value << text.LineSpacing;
				output << YAML::Key << "Shadowing" << YAML::Value << text.Shadowing;
				output << YAML::Key << "ShadowDistance" << YAML::Value << text.ShadowDistance;
				output << YAML::Key << "ShadowColor" << YAML::Value << text.ShadowColor;
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

		SerializeComponent<StaticMeshComponent>(out, "StaticMeshComponent", entity,
			[](YAML::Emitter& output, const StaticMeshComponent& meshComponent)
			{
				Ref<StaticMesh> mesh = meshComponent.Mesh;
				output << YAML::Key << "FilePath" << YAML::Value << mesh->GetFilePath().string();
				output << YAML::Key << "Visible" << YAML::Value << meshComponent.Visible;
			});

		SerializeComponent<DirectionalLightComponent>(out, "DirectionalLightComponent", entity,
			[](YAML::Emitter& output, const DirectionalLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "CastShadows" << YAML::Value << lightComponent.CastShadows;
				output << YAML::Key << "LightSize" << YAML::Value << lightComponent.LightSize;
			});

		SerializeComponent<PointLightComponent>(out, "PointLightComponent", entity,
			[](YAML::Emitter& output, const PointLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "Radius" << YAML::Value << lightComponent.Radius;
				output << YAML::Key << "FallOff" << YAML::Value << lightComponent.FallOff;
			});

		SerializeComponent<SpotLightComponent>(out, "SpotLightComponent", entity,
			[](YAML::Emitter& output, const SpotLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "SpotAngle" << YAML::Value << lightComponent.SpotAngle;
				output << YAML::Key << "InnerFallOff" << YAML::Value << lightComponent.InnerFallOff;
				output << YAML::Key << "Range" << YAML::Value << lightComponent.Range;
				output << YAML::Key << "RangeFallOff" << YAML::Value << lightComponent.RangeFallOff;
			});


		SerializeComponent<SkyLightComponent>(out, "SkyLightComponent", entity,
			[](YAML::Emitter& output, const SkyLightComponent& lightComponent)
			{
				const auto& envMap = lightComponent.EnvironmentMap;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "LOD" << YAML::Value << lightComponent.LOD;
				output << YAML::Key << "Resolution" << envMap->GetResolution();
				output << YAML::Key << "Type" << (int)envMap->GetType();
				output << YAML::Key << "FilePath" << envMap->GetFilePath().string();
				output << YAML::Key << "Turbidity" << envMap->GetTurbidity();
				output << YAML::Key << "Azimuth" << envMap->GetAzimuth();
				output << YAML::Key << "Inclination" << envMap->GetInclination();
			});

		out << YAML::EndMap;
	}
}
