#include "SceneHierarchyPanel.h"

#include "Athena/Core/FileDialogs.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Input/Input.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scripting/ScriptEngine.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"
#include "Panels/PanelManager.h"
#include "EditorResources.h"

#include <ImGui/imgui.h>


namespace Athena
{
#define SCRIPT_FIELD_SCALAR_INPUT(nativeType, fieldType, imguiType)	\
	case ScriptFieldType::fieldType:	\
	{	\
		nativeType data = field.GetValue<nativeType>();  \
		UI::PropertyRow(name.data(), ImGui::GetFrameHeight());  \
		ImGui::PushID(name.data());  \
		if (ImGui::DragScalar("##InputScalar", imguiType, &data))  \
			field.SetValue(data);  \
		ImGui::PopID();  \
		break;  \
	}

#define SCRIPT_FIELD_SCALAR_INPUT_N(type, components)	\
	case ScriptFieldType::type:	\
	{	\
		type data = field.GetValue<type>();  \
		UI::PropertyRow(name.data(), ImGui::GetFrameHeight());  \
		ImGui::PushID(name.data());  \
		if (ImGui::DragScalarN("##InputScalar", ImGuiDataType_Float, data.Data(), components))  \
			field.SetValue(data);  \
		ImGui::PopID();  \
		break;  \
	}


	static void DrawVec3Property(std::string_view label, Vector3& values, float defaultValues)
	{
		UI::PropertyRow(label, ImGui::GetFrameHeight());

		ImGui::PushID(label.data());

		float full_width = ImGui::GetContentRegionAvail().x - 15.f;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

		float buttonWidth = ImGui::GetFrameHeight();
		ImVec2 buttonSize = { buttonWidth, buttonWidth };

		float dragWidth = (full_width - 3 * buttonWidth) / 3.f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("X", buttonSize))
			values.x = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##X", &values.x, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("Y", buttonSize))
			values.y = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Y", &values.y, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("Z", buttonSize))
			values.z = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Z", &values.z, 0.7f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();

		ImGui::PopID();
	}


	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<EditorContext>& context)
		: Panel(SCENE_HIERARCHY_PANEL_ID, context)
	{
		UI::RegisterEnum("Renderer2DSpace");
		UI::EnumAdd("Renderer2DSpace", 1, "World Space");
		UI::EnumAdd("Renderer2DSpace", 2, "Screen Space");
		UI::EnumAdd("Renderer2DSpace", 3, "Billboard");
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Materials Editor");
		if (m_EditorCtx.SelectedEntity)
		{
			DrawMaterialsEditor();
		}
		ImGui::PopStyleVar();
		ImGui::End();

		DrawEntitiesHierarchy();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Properties");
		if (m_EditorCtx.SelectedEntity)
		{
			DrawAllComponents(m_EditorCtx.SelectedEntity);
		}
		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntitiesHierarchy()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetTheme().BackgroundDark);
		ImGui::Begin("Scene Hierarchy");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
		ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
		ImGui::PushStyleColor(ImGuiCol_Header, color);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

		const auto& view = m_EditorCtx.ActiveScene->GetAllEntitiesWith<IDComponent>();

		// When scene state changes to Play or Simulate, scene is copied, which leads
		// to reversed order of entities, to "prevent" this we iterate entities in reversed order
		if (m_EditorCtx.SceneState == SceneState::Edit)
		{
			for (auto entt : view)
			{
				Entity entity = Entity(entt, m_EditorCtx.ActiveScene.Raw());
				if (!entity.HasComponent<ParentComponent>())
				{
					DrawEntityNode(entity);
				}
			}
		}
		else
		{
			for (auto it = view.rbegin(); it != view.rend(); ++it)
			{
				Entity entity = Entity(*it, m_EditorCtx.ActiveScene.Raw());
				if (!entity.HasComponent<ParentComponent>())
				{
					DrawEntityNode(entity);
				}
			}
		}

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		UI::InvisibleItem("DragDropRegion", ImGui::GetContentRegionAvail());
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
			{
				Entity payloadEntity = *(Entity*)payload->Data;
				m_EditorCtx.ActiveScene->MakeOrphan(payloadEntity);
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonLeft) && ImGui::IsWindowHovered())
			m_EditorCtx.SelectedEntity = {};

		if (ImGui::BeginPopupContextItem("Entity Hierarchy Settings", ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Entity"))
			{
				Entity created = m_EditorCtx.ActiveScene->CreateEntity();
				m_EditorCtx.SelectedEntity = created;
			}

			ImGui::EndPopup();
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;
		bool selected = m_EditorCtx.SelectedEntity == entity;
		bool hasChildren = entity.HasComponent<ChildComponent>();
		bool hasParent = entity.HasComponent<ParentComponent>();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_FramePadding |
			ImGuiTreeNodeFlags_DefaultOpen;

		if (selected)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		if(selected)
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive]);

		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (selected)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked())
		{
			m_EditorCtx.SelectedEntity = entity;
		}

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("SCENE_HIERARCHY_ENTITY", &entity, sizeof(entity));
			ImGui::Text(tag.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				Entity payloadEntity = *(Entity*)payload->Data;
				if (payloadEntity != entity)
				{
					m_EditorCtx.ActiveScene->MakeRelationship(entity, payloadEntity);
				}
			}

			ImGui::EndDragDropTarget();
		}

		Entity selectedEntity = m_EditorCtx.SelectedEntity;
		bool entityDeleted = false;

		static char idString[20];
		sprintf(idString, "%llu", (uint64)entity.GetComponent<IDComponent>().ID);

		if (ImGui::BeginPopupContextItem(idString, ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			if (hasParent && ImGui::MenuItem("Make Orphan"))
				m_EditorCtx.ActiveScene->MakeOrphan(entity);

			if (selectedEntity && entity != selectedEntity && ImGui::MenuItem("Add to children"))
			{
				Entity newParent = entity;
				Entity newChild = selectedEntity;
				m_EditorCtx.ActiveScene->MakeRelationship(newParent, newChild);
			}

			ImGui::EndPopup();
		}

		if (opened && hasChildren)
		{
			const std::vector<Entity>& children = entity.GetComponent<ChildComponent>().Children;
			for (auto entity : children)
			{
				DrawEntityNode(entity);
			}
		}

		if (opened && hasChildren)
			ImGui::TreePop();

		if (entityDeleted)
		{
			m_EditorCtx.ActiveScene->DestroyEntity(entity);
			if (selectedEntity == entity)
				m_EditorCtx.SelectedEntity = {};
		}
	}

	void SceneHierarchyPanel::DrawMaterialsEditor()
	{
		Entity selectedEntity = m_EditorCtx.SelectedEntity;
		std::vector<String> materials;
		if(selectedEntity.HasComponent<StaticMeshComponent>())
		{
			Ref<StaticMesh> mesh = selectedEntity.GetComponent<StaticMeshComponent>().Mesh;
			const auto& subMeshes = mesh->GetAllSubMeshes();
			materials.reserve(subMeshes.size());
			for (uint32 i = 0; i < subMeshes.size(); ++i)
			{
				const String& material = subMeshes[i].MaterialName;
				if (std::find(materials.begin(), materials.end(), material) == materials.end())
				{
					materials.push_back(material);
				}
			}

			if (!materials.empty() && std::find(materials.begin(), materials.end(), m_ActiveMaterial) == materials.end())
				m_ActiveMaterial = materials[0];
		}

		if (!materials.empty())
		{
			if (m_ActiveMaterial.empty())
				m_ActiveMaterial = materials[0];

			if (UI::BeginPropertyTable())
			{
				UI::PropertyCombo("Material List", materials.data(), materials.size(), &m_ActiveMaterial);

				UI::EndPropertyTable();
			}
			
			Ref<MaterialTable> materialTable = selectedEntity.GetComponent<StaticMeshComponent>().Mesh->GetMaterialTable();
			Ref<Material> material = materialTable->Get(m_ActiveMaterial);

			if (UI::TreeNode("Material") && UI::BeginPropertyTable())
			{
				DrawMaterialProperty(material, "u_AlbedoMap", "u_UseAlbedoMap", "u_Albedo");
				DrawMaterialProperty(material, "u_NormalMap", "u_UseNormalMap", "");
				DrawMaterialProperty(material, "u_RoughnessMap", "u_UseRoughnessMap", "u_Roughness");
				DrawMaterialProperty(material, "u_MetalnessMap", "u_UseMetalnessMap", "u_Metalness");

				bool castShadows = material->GetFlag(MaterialFlag::CAST_SHADOWS);
				if(UI::PropertyCheckbox("Cast Shadows", &castShadows))
					material->SetFlag(MaterialFlag::CAST_SHADOWS, castShadows);

				UI::EndPropertyTable();
				UI::TreePop();
				ImGui::Spacing();
			}
		}
	}

	void SceneHierarchyPanel::DrawMaterialProperty(Ref<Material> mat, const String& texName, const String& useTexName, const String& uniformName)
	{
		ImGui::PushID(texName.data());
		{
			float imageSize = 45.f;
			Ref<Texture2D> texture = mat->Get<Ref<Texture2D>>(texName);
			Ref<Texture2D> displayTexture = texture;

			if (!texture || texture == TextureGenerator::GetWhiteTexture())
				displayTexture = EditorResources::GetIcon("Empty Texture");

			if (UI::PropertyImage(texName.data(), displayTexture, { imageSize, imageSize, }))
			{
				FilePath path = FileDialogs::OpenFile("Select Texture", { "Texture files", "*.png *.jpg" }, Project::GetAssetDirectory());
				if (!path.empty())
				{
					texture = TextureImporter::Load(path, texName == "u_Albedo" ? true : false);
					mat->Set(texName, texture);
					mat->Set(useTexName, (uint32)true);
				}
			}
			ImGui::SameLine();
			                                                                          
			bool useTexture = mat->Get<uint32>(useTexName);
			if(ImGui::Checkbox("Use", &useTexture))
				mat->Set(useTexName, (uint32)useTexture);
			
			if (!uniformName.empty())
			{
				ImGui::SameLine();
				if (uniformName == "u_Albedo")
				{
					Vector4 albedo = mat->Get<Vector4>(uniformName);
					if(ImGui::ColorEdit4(uniformName.data(), albedo.Data(), ImGuiColorEditFlags_NoInputs))
						mat->Set(uniformName, albedo);

					float emission = mat->Get<float>("u_Emission");
					if(ImGui::DragFloat("u_Emission", &emission, 1.f, 0.f, 500.f))
						mat->Set("u_Emission", emission);
				}
				else
				{
					float uniform = mat->Get<float>(uniformName);
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
					if(ImGui::SliderFloat(uniformName.data(), &uniform, 0.f, 1.f))
						mat->Set(uniformName, uniform);
				}
			}
		}
		ImGui::PopID();
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		float fullWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 3.f, 3.f });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5.f, 5.f });
		UI::ShiftCursorX(5.f);
		if (ImGui::Button("Edit"))
			m_EditTagComponent = !m_EditTagComponent;

		ImGui::SameLine();

		auto& tag = entity.GetComponent<TagComponent>().Tag;
		if (m_EditTagComponent)
		{
			if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
			UI::TextInput("TagInputText", tag);
			if (ImGui::IsItemDeactivatedAfterEdit())
				m_EditTagComponent = false;
		}
		else 
		{
			UI::PushFont(UI::Fonts::Bold);
			ImGui::Text(tag.data());
			ImGui::PopFont();
		}

		ImGui::SameLine();

		auto& style = ImGui::GetStyle();
		float buttonWidth = ImGui::CalcTextSize("Add Component").x + 2 * style.ItemInnerSpacing.x;
		ImGui::SetCursorPosX(fullWidth - buttonWidth - 5.f);
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("Add Component");

		if (ImGui::BeginPopup("Add Component"))
		{
			DrawAddComponentEntry<TransformComponent>(entity, "Transform");
			DrawAddComponentEntry<CameraComponent>(entity, "Camera");
			DrawAddComponentEntry<ScriptComponent>(entity, "Script");
			DrawAddComponentEntry<SpriteComponent>(entity, "Sprite");
			DrawAddComponentEntry<CircleComponent>(entity, "Circle");
			DrawAddComponentEntry<TextComponent>(entity, "Text");
			DrawAddComponentEntry<Rigidbody2DComponent>(entity, "Rigidbody2D");
			DrawAddComponentEntry<BoxCollider2DComponent>(entity, "BoxCollider2D");
			DrawAddComponentEntry<CircleCollider2DComponent>(entity, "CircleCollider2D");
			DrawAddComponentEntry<StaticMeshComponent>(entity, "StaticMesh");
			DrawAddComponentEntry<DirectionalLightComponent>(entity, "DirectionalLight");
			DrawAddComponentEntry<PointLightComponent>(entity, "PointLight");
			DrawAddComponentEntry<SpotLightComponent>(entity, "SpotLight");
			DrawAddComponentEntry<SkyLightComponent>(entity, "SkyLight");

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(2);

		ImGui::Spacing();

		DrawComponent<TransformComponent>(entity, "Transform", [](TransformComponent& transform)
		{
			DrawVec3Property("Translation", transform.Translation, 0.0f);

			Vector3 degrees = Math::Degrees(transform.Rotation.AsEulerAngles());
			DrawVec3Property("Rotation", degrees, 0.0f);
			transform.Rotation = Math::Radians(degrees);

			DrawVec3Property("Scale", transform.Scale, 1.0f);

			return true;
		});

		DrawComponent<ScriptComponent>(entity, "Script", [entity](ScriptComponent& script)
		{
 			const std::vector<String>& scripts = ScriptEngine::GetAvailableScripts();
			UI::PropertyCombo("Script Name", scripts.data(), scripts.size(), &script.Name);

			ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(entity);

			if (fieldMap == nullptr)
				return true;

			for (auto& [name, field] : *fieldMap)
			{
				switch (field.GetType())
				{
				SCRIPT_FIELD_SCALAR_INPUT(byte,   Byte,   ImGuiDataType_U8);
				SCRIPT_FIELD_SCALAR_INPUT(int16,  Int16,  ImGuiDataType_S16);
				SCRIPT_FIELD_SCALAR_INPUT(uint16, UInt16, ImGuiDataType_U16);
				SCRIPT_FIELD_SCALAR_INPUT(int32,  Int32,  ImGuiDataType_S32);
				SCRIPT_FIELD_SCALAR_INPUT(uint32, UInt32, ImGuiDataType_U32);
				SCRIPT_FIELD_SCALAR_INPUT(int64,  Int64,  ImGuiDataType_S64);
				SCRIPT_FIELD_SCALAR_INPUT(uint64, UInt64, ImGuiDataType_U64);
				SCRIPT_FIELD_SCALAR_INPUT(float,  Float,  ImGuiDataType_Float);
				SCRIPT_FIELD_SCALAR_INPUT(double, Double, ImGuiDataType_Double);
				
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector2, 2);
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector3, 3);
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector4, 4);

				case ScriptFieldType::Bool:
				{
					bool data = field.GetValue<bool>();
					if (UI::PropertyCheckbox(name.data(), &data))
						field.SetValue(data);
					break;
				}
				case ScriptFieldType::Char:
				{
					char data = field.GetValue<char>();  
					UI::PropertyRow(name.data(), ImGui::GetFrameHeight());

					static char buffer[2];
					buffer[0] = data;
					buffer[1] = '\0';

					ImGui::PushID(name.data());
					if (ImGui::InputText("##InputText", buffer, sizeof(buffer)))
						field.SetValue(buffer[0]);
					ImGui::PopID();  
					break;
				}
				}
			}

			return true;
		});

		DrawComponent<CameraComponent>(entity, "Camera", [](CameraComponent& cameraComponent)
		{
			auto& camera = cameraComponent.Camera;

			static auto typeToStr = [](SceneCamera::ProjectionType type) -> std::string_view
			{
				switch (type)
				{
				case SceneCamera::ProjectionType::Orthographic: return "Orthographic";
				case SceneCamera::ProjectionType::Perspective: return "Perspective";
				}

				return "Invalid";
			};

			static auto strToType = [](std::string_view str) -> SceneCamera::ProjectionType
			{
				if (str == "Orthographic")
					return SceneCamera::ProjectionType::Orthographic;

				if (str == "Perspective")
					return SceneCamera::ProjectionType::Perspective;

				return (SceneCamera::ProjectionType)0;
			};

			std::string_view projTypesStrings[] = { "Orthographic", "Perspective" };
			std::string_view currentTypeString = typeToStr(camera.GetProjectionType());

			if(UI::PropertyCombo("Projection", projTypesStrings, std::size(projTypesStrings), &currentTypeString))
			{
				camera.SetProjectionType(strToType(currentTypeString));
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				auto perspectiveDesc = camera.GetPerspectiveData();
				bool used = false;

				float degreesFOV = Math::Degrees(perspectiveDesc.VerticalFOV);
				used = UI::PropertySlider("FOV", &degreesFOV, 0.f, 360.f) || used;
				used = UI::PropertySlider("Near Clip", &perspectiveDesc.NearClip, 0.f, 10.f) || used;
				used = UI::PropertySlider("Far Clip", &perspectiveDesc.FarClip, 1000.f, 30000.f) || used;

				if (used)
				{
					perspectiveDesc.VerticalFOV = Math::Radians(degreesFOV);
					camera.SetPerspectiveData(perspectiveDesc);
				}
			}

			else if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				auto orthoDesc = camera.GetOrthographicData();
				bool used = false;

				used = UI::PropertyDrag("Size", &orthoDesc.Size, 0.1f) || used;
				used = UI::PropertySlider("Near Clip", &orthoDesc.NearClip, -10.f, 0.f) || used;
				used = UI::PropertySlider("Far Clip", &orthoDesc.FarClip, 0.f, 10.f) || used;

				if (used)
				{
					camera.SetOrthographicData(orthoDesc);
				}
			}

			UI::PropertyCheckbox("Primary", &cameraComponent.Primary);
			UI::PropertyCheckbox("Fixed Aspect Ratio", &cameraComponent.FixedAspectRatio);

			return true;
		});

		DrawComponent<SpriteComponent>(entity, "Sprite", [&entity](SpriteComponent& sprite)
		{
			UI::PropertyColor4("Color", sprite.Color.Data());

			float imageSize = 45.f;
			UI::PropertyImage("Texture", sprite.Texture.GetNativeTexture(), { imageSize, imageSize });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					FilePath path = (const char*)payload->Data;
					if (path.extension() == ".png\0")
					{
						sprite.Texture = TextureImporter::Load(FilePath(path), true);
						sprite.Color = LinearColor::White;
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			ImVec2 cursor = ImGui::GetCursorPos();
			if (ImGui::Button("Browse"))
			{
				FilePath path = FileDialogs::OpenFile("Select Texture", { "Texture files", "*.png *.jpg" }, Project::GetAssetDirectory());
				if (!path.empty())
				{
					sprite.Texture = TextureImporter::Load(path, true);
					sprite.Color = LinearColor::White;
				}
			}

			ImGui::SetCursorPos({ cursor.x, cursor.y + imageSize / 1.8f });
			if (ImGui::Button("Reset"))
				sprite.Texture = TextureGenerator::GetWhiteTexture();

			if(UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&sprite.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertySlider("Tiling", &sprite.TilingFactor, 1.f, 20.f);

			return true;
		});

		DrawComponent<CircleComponent>(entity, "Circle", [&entity](CircleComponent& circle)
		{
			UI::PropertyColor4("Color", circle.Color.Data());

			if (UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&circle.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertySlider("Thickness", &circle.Thickness, 0.f, 1.f);
			UI::PropertySlider("Fade", &circle.Fade, 0.f, 1.f);

			return true;
		});

		DrawComponent<TextComponent>(entity, "Text", [&entity](TextComponent& text)
		{
			UI::PropertyRow("Text", ImGui::GetFrameHeight());
			UI::InputTextMultiline("##TextInput", text.Text, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 6), ImGuiInputTextFlags_AllowTabInput);

			if (!FileSystem::Exists(text.Font->GetFilePath()))
				text.Font = Font::GetDefault();

			bool isDefault = std::filesystem::equivalent(Font::GetDefault()->GetFilePath(), text.Font->GetFilePath());
			String fontName = isDefault ? "Default" : text.Font->GetFilePath().filename().string();

			UI::PropertyRow("Font", ImGui::GetFrameHeight() + 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 4 });
			if (ImGui::Button(fontName.c_str()))
			{
				FilePath filepath = FileDialogs::OpenFile("Select Font", { "Font files", "*.ttf *.TTF" }, Project::GetAssetDirectory());
				String ext = filepath.extension().string();
				if (ext == ".ttf" || ext == ".TTF")
					text.Font = Font::Create(filepath);
			}

			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					FilePath filepath = (const char*)payload->Data;
					String ext = filepath.extension().string();
					if (ext == ".ttf" || ext == ".TTF")
						text.Font = Font::Create(filepath);
				}
				ImGui::EndDragDropTarget();
			}

			if (!isDefault)
			{
				ImGui::SameLine();

				auto& style = ImGui::GetStyle();
				ImVec2 labelSize = ImGui::CalcTextSize("  ");
				ImVec2 size = ImGui::CalcItemSize({ 0, 0 }, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 0.5f);
				UI::ShiftCursorY(style.FramePadding.y);
				if (ImGui::InvisibleButton("ResetFont", size))
					text.Font = Font::GetDefault();
				UI::ButtonImage((EditorResources::GetIcon("ContentBrowser_Refresh")));
			}

			if (UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&text.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertyColor4("Color", text.Color.Data());
			UI::PropertyDrag("Max Width", &text.MaxWidth, 0.2f);
			UI::PropertyDrag("Kerning", &text.Kerning, 0.025f);
			UI::PropertyDrag("Line Spacing", &text.LineSpacing, 0.025f);
			UI::PropertyCheckbox("Shadowing", &text.Shadowing);

			if (text.Shadowing)
			{
				UI::PropertyDrag("Shadow Distance", &text.ShadowDistance, 0.1f);
				UI::PropertyColor4("Shadow Color", text.ShadowColor.Data());
			}

			return true;
		});

		DrawComponent<Rigidbody2DComponent>(entity, "Rigidbody2D", [](Rigidbody2DComponent& rb2d)
		{
			static auto typeToStr = [](Rigidbody2DComponent::BodyType type) -> std::string_view
			{
				switch (type)
				{
				case Rigidbody2DComponent::BodyType::STATIC: return "Static";
				case Rigidbody2DComponent::BodyType::DYNAMIC: return "Dynamic";
				case Rigidbody2DComponent::BodyType::KINEMATIC: return "Kinematic";
				}

				return "Invalid";
			};

			static auto strToType = [](std::string_view str) -> Rigidbody2DComponent::BodyType
			{
				if (str == "Static")
					return Rigidbody2DComponent::BodyType::STATIC;

				if (str == "Dynamic")
					return Rigidbody2DComponent::BodyType::DYNAMIC;

				if (str == "Kinematic")
					return Rigidbody2DComponent::BodyType::KINEMATIC;

				return (Rigidbody2DComponent::BodyType)0;
			};


			std::string_view bodyTypesStrings[] = { "Static", "Dynamic", "Kinematic"};
			std::string_view currentBodyType = typeToStr(rb2d.Type);

			if (UI::PropertyCombo("BodyType", bodyTypesStrings, std::size(bodyTypesStrings), &currentBodyType))
			{
				rb2d.Type = strToType(currentBodyType);
			}

			UI::PropertyCheckbox("Fixed Rotation", &rb2d.FixedRotation);
					
			return true;
		});


		DrawComponent<BoxCollider2DComponent>(entity, "BoxCollider2D", [](BoxCollider2DComponent& bc2d)
		{
			UI::PropertyDrag("Offset", bc2d.Offset.Data(), 0.1f);
			UI::PropertyDrag("Size", bc2d.Size.Data(), 0.1f);
			UI::PropertySlider("Density", &bc2d.Density, 0.f, 1.f);
			UI::PropertySlider("Friction", &bc2d.Friction, 0.f, 1.f);
			UI::PropertySlider("Restitution", &bc2d.Restitution, 0.f, 1.f);
			UI::PropertySlider("Restitution Threshold", &bc2d.RestitutionThreshold, 0.f, 1.f);

			return true;
		});

		DrawComponent<CircleCollider2DComponent>(entity, "CircleCollider2D", [](CircleCollider2DComponent& cc2d)
		{
			UI::PropertyDrag("Offset", &cc2d.Offset, 0.1f);
			UI::PropertyDrag("Radius", &cc2d.Radius, 0.1f);
			UI::PropertySlider("Density", &cc2d.Density, 0.f, 1.f);
			UI::PropertySlider("Friction", &cc2d.Friction, 0.f, 1.f);
			UI::PropertySlider("Restitution", &cc2d.Restitution, 0.f, 1.f);
			UI::PropertySlider("Restitution Threshold", &cc2d.RestitutionThreshold, 0.f, 1.f);

			return true;
		});

		DrawComponent<StaticMeshComponent>(entity, "StaticMesh", [this, entity](StaticMeshComponent& meshComponent)
		{
			String meshFilename = meshComponent.Mesh->GetFilePath().filename().string();

			String name = meshComponent.Mesh->GetFilePath().filename().string();

			UI::PropertyRow("Mesh", ImGui::GetFrameHeight() + 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 4 });
			if (ImGui::Button(name.c_str()))
			{
				FilePath filepath = FileDialogs::OpenFile("Select Mesh", { "Mesh files", "*.fbx *.gltf *.obj *.blend *.x3d" }, Project::GetAssetDirectory());
				String ext = filepath.extension().string();
				if (!filepath.empty())
					meshComponent.Mesh = StaticMesh::Create(filepath);
			}

			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					FilePath filepath = (const char*)payload->Data;
					String ext = filepath.extension().string();
					if (ext == ".obj\0" || ext == ".fbx" || ext == ".x3d" || ext == ".gltf" || ext == ".blend")
						meshComponent.Mesh = StaticMesh::Create(filepath);
				}
				ImGui::EndDragDropTarget();
			}

			UI::PropertyCheckbox("Visible", &meshComponent.Visible);
			UI::EndPropertyTable();

			Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
			if (animator)
			{
				if (UI::TreeNode("Animations", true, true) && UI::BeginPropertyTable())
				{
					Ref<Animation> active = animator->GetCurrentAnimation();
					active = active ? active : (animator->GetAllAnimations().size() > 0 ? animator->GetAllAnimations()[0] : nullptr);

					std::string_view selectedAnim = active ? active->GetName().c_str() : "";

					const auto& animations = animator->GetAllAnimations();
					std::vector<std::string_view> animNames(animations.size());
					for (uint32 i = 0; i < animations.size(); ++i)
						animNames[i] = animations[i]->GetName();

					if (UI::PropertyCombo("Animation List", animNames.data(), animNames.size(), &selectedAnim))
					{
						uint32 index = std::distance(animNames.begin(), std::find(animNames.begin(), animNames.end(), selectedAnim));
						Ref<Animation> anim = animations[index];
						animator->PlayAnimation(anim);
					}

					{ 
						bool playNow = active == animator->GetCurrentAnimation();
						bool check = playNow;
						UI::PropertyCheckbox("Play", &check);

						if (check && !playNow)
							animator->PlayAnimation(active);
						else if (!check && playNow)
							animator->StopAnimation();

						if (check)
						{
							Ref<Animation> anim = animator->GetCurrentAnimation();
							uint32 ticks = anim->GetTicksPerSecond();
							float animTime = animator->GetAnimationTime() / (float)ticks;
							ImGui::SameLine();
							ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
							ImGui::SliderFloat("##Duration", &animTime, 0, (anim->GetDuration() - 1) / (float)ticks, nullptr, ImGuiSliderFlags_NoInput);
							animator->SetAnimationTime(animTime * (float)ticks);
						}
					}

					UI::EndPropertyTable();

					UI::TreePop();
					ImGui::Spacing();
				}
			}

			return false;
		});

		DrawComponent<DirectionalLightComponent>(entity, "DirectionalLight", [](DirectionalLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 100.f);
			UI::PropertyCheckbox("Cast Shadows", &lightComponent.CastShadows);
			UI::PropertyDrag("Light Size", &lightComponent.LightSize, 0.025f, 0.f, 100.f);

			return true;
		});

		DrawComponent<PointLightComponent>(entity, "PointLight", [](PointLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);
			UI::PropertyDrag("Radius", &lightComponent.Radius, 2.5f, 0.f, 10000.f);
			UI::PropertyDrag("Fall Off", &lightComponent.FallOff, 0.1f, 0.f, 100.f);

			return true;
		});

		DrawComponent<SpotLightComponent>(entity, "SpotLight", [](SpotLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);
			UI::PropertySlider("Spot Angle", &lightComponent.SpotAngle, 0.f, 180.f);
			UI::PropertyDrag("Inner Fall Off", &lightComponent.InnerFallOff, 0.1f, 0.f, 100.f);
			UI::PropertyDrag("Range", &lightComponent.Range, 2.5f, 0.f, 10000.f);
			UI::PropertyDrag("Range Fall Off", &lightComponent.RangeFallOff, 0.1f, 0.f, 100.f);

			return true;
		});

		DrawComponent<SkyLightComponent>(entity, "SkyLight", [](SkyLightComponent& lightComponent)
		{
			static auto typeToStr = [](EnvironmentMapType type) -> std::string_view
				{
					switch (type)
					{
					case EnvironmentMapType::STATIC: return "Static";
					case EnvironmentMapType::PREETHAM: return "Preetham";
					}

					return "Invalid";
				};

			static auto strToType = [](std::string_view str) -> EnvironmentMapType
				{
					if (str == "Static")
						return EnvironmentMapType::STATIC;

					if (str == "Preetham")
						return EnvironmentMapType::PREETHAM;

					return (EnvironmentMapType)0;
				};


			UI::PropertySlider("Intensity", &lightComponent.Intensity, 0.f, 10.f);
			UI::PropertySlider("LOD", &lightComponent.LOD, 0, ShaderDef::MAX_SKYBOX_MAP_LOD - 1);

			const auto& envMap = lightComponent.EnvironmentMap;

			const std::string_view resolutions[] = { "128", "256", "512", "1024", "2048", "4096" };
			String selectedStr = std::to_string(envMap->GetResolution());
			std::string_view selected = selectedStr.data();

			if (UI::PropertyCombo("Resolution", resolutions, std::size(resolutions), &selected))
			{
				uint32 resolution = std::atoi(selected.data());
				envMap->SetResolution(resolution);
			}

			std::string_view typesStrings[] = { "Static", "Preetham"};
			std::string_view type = typeToStr(envMap->GetType());

			if (UI::PropertyCombo("Type", typesStrings, std::size(typesStrings), &type))
			{
				envMap->SetType(strToType(type));
			}
			if (envMap->GetType() == EnvironmentMapType::STATIC)
			{
				UI::PropertyRow("FilePath", ImGui::GetFrameHeight());

				const FilePath& envPath = envMap->GetFilePath();
				String label = envPath.empty() ? "Load Environment Map" : envPath.stem().string();

				if (ImGui::Button(label.data()))
				{
					FilePath filepath = FileDialogs::OpenFile("Select Environment map", { "HDR files", "*.hdr" }, Project::GetAssetDirectory());
					if (!filepath.empty())
						envMap->SetFilePath(filepath);
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						FilePath filepath = (const char*)payload->Data;
						String ext = filepath.extension().string();
						if (ext == ".hdr")
							envMap->SetFilePath(filepath);
					}
					ImGui::EndDragDropTarget();
				}

			}
			else if (envMap->GetType() == EnvironmentMapType::PREETHAM)
			{
				float turbidity = envMap->GetTurbidity();
				UI::PropertySlider("Turbidity", &turbidity, 1.8f, 10.f);

				float azimuth = envMap->GetAzimuth();
				UI::PropertySlider("Azimuth", &azimuth, 0, 2 * Math::PI<float>());

				float inclination = envMap->GetInclination();
				UI::PropertySlider("Inclination", &inclination, 0, 2 * Math::PI<float>());

				envMap->SetPreethamParams(turbidity, azimuth, inclination);
			}

			return true;
		});
	}
}
