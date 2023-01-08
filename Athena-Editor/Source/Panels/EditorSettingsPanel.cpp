#include "EditorSettingsPanel.h"

#include "Athena/Scripting/PublicScriptEngine.h"
#include "UI/Widgets.h"

#include <ImGui/imgui.h>


namespace Athena
{
	EditorSettingsPanel::EditorSettingsPanel(std::string_view name)
		: Panel(name)
	{

	}

	void EditorSettingsPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Editor Settings");
		ImGui::PopStyleVar();

		if (UI::BeginTreeNode("Physics"))
		{
			UI::ShiftCursorY(2.f);
			UI::DrawImGuiWidget("Show Physics Colliders", [this]() { return ImGui::Checkbox("##Show Physics Colliders", &m_Settings.m_ShowPhysicsColliders); });
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Scripting"))
		{
			UI::ShiftCursorY(2.f);
			ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
			if (ImGui::Button("Reload Scripts"))
			{
				PublicScriptEngine::ReloadScripts();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			UI::EndTreeNode();
		}

		ImGui::End();
	}
}