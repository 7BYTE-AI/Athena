#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"

#include "Athena/Scene/Entity.h"

#include "Athena/Renderer/Material.h"

#include "Panels/Panel.h"

#include "Athena/UI/UI.h"

#include <string_view>


namespace Athena
{
	class Scene;


	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;

	private:
		void DrawEntitiesHierarchy();
		void DrawEntityNode(Entity entity);
		void DrawAllComponents(Entity entity);

		void DrawMaterialsEditor();
		void DrawMaterialProperty(Ref<Material> mat, const String& texName, const String& useTexName, const String& uniformName);

		template <typename Component, typename Func>
		void DrawComponent(Entity entity, std::string_view name, Func uiFunction);

		template <typename Component>
		void DrawAddComponentEntry(Entity entity, std::string_view name);

	private:
		String m_ActiveMaterial;
		bool m_EditTagComponent = false;
	};



	template <typename Component, typename Func>
	void SceneHierarchyPanel::DrawComponent(Entity entity, std::string_view name, Func uiFunction)
	{
		if (entity.HasComponent<Component>())
		{
			ImVec2 regionAvail = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });

			float lineHeight = ImGui::GetFrameHeight();

			bool open = UI::TreeNode(name.data());

			bool removeComponent = false;

			ImGui::SameLine(regionAvail.x - lineHeight);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
			UI::ShiftCursor(1.f, 1.f);
			if (ImGui::Button("...", { lineHeight - 2.f, lineHeight - 2.f }))
				ImGui::OpenPopup("ComponentSettings");
			ImGui::PopStyleVar();

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("RemoveComponent"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			ImGui::PopStyleVar(2);

			if (open)
			{
				if (UI::BeginPropertyTable())
				{
					if(uiFunction(entity.GetComponent<Component>()))
						UI::EndPropertyTable();
				}
				UI::TreePop();
				ImGui::Spacing();
			}

			if (removeComponent)
				entity.RemoveComponent<Component>();
		}
	}

	template <typename Component>
	void SceneHierarchyPanel::DrawAddComponentEntry(Entity entity, std::string_view name)
	{
		if (!entity.HasComponent<Component>() && ImGui::MenuItem(name.data()))
		{
			entity.AddComponent<Component>();
			ImGui::CloseCurrentPopup();
		}
	}
}
