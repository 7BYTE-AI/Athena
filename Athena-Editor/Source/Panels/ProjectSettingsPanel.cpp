#include "ProjectSettingsPanel.h"

#include "Athena/Core/FileDialogs.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"
#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>


namespace Athena
{
	ProjectSettingsPanel::ProjectSettingsPanel(const Ref<EditorContext>& context)
		: Panel(PROJECT_SETTINGS_PANEL_ID, context)
	{
		OnProjectUpdate();
	}

	void ProjectSettingsPanel::OnProjectUpdate() 
	{
		m_Config = Project::GetActive()->GetConfig();
	}

	void ProjectSettingsPanel::OnImGuiRender()
	{
		ImGui::Begin("Project Settings");

		if (UI::TreeNode("General") && UI::BeginPropertyTable())
		{
			UI::PropertyRow("Name", ImGui::GetFrameHeight());
			UI::TextInput("ProjectNameInputText", m_Config.Name);

			UI::PropertyRow("Start Scene", ImGui::GetFrameHeight());

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(" ... ").x - 2 * ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x);
			String startScene = m_Config.StartScene.string();
			if (UI::TextInput("StartSceneInputText", startScene))
				m_Config.StartScene = startScene;

			ImGui::SameLine();
			if (ImGui::Button(" ... "))
			{
				FilePath path = FileDialogs::OpenFile("Select Start Scene", { "Scene files", "*.atn" }, Project::GetAssetDirectory());
				if (!path.empty())
					m_Config.StartScene = Project::GetRelativeAssetPath(path);
			}

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Scripting") && UI::BeginPropertyTable())
		{
			UI::PropertyRow("Athena Source Directory", ImGui::GetFrameHeight());

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(" ... ").x - 2 * ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x);
			String sourceDir = m_Config.AthenaSourceDirectory.string();
			if (UI::TextInput("AthenSourceInputText", sourceDir))
				m_Config.AthenaSourceDirectory = sourceDir;

			ImGui::SameLine();
			if (ImGui::Button(" ... "))
			{
				FilePath path = FileDialogs::OpenDirectory("Select Athena Source Directory", Project::GetProjectDirectory());
				if (!path.empty())
					m_Config.AthenaSourceDirectory = std::filesystem::relative(path, Project::GetScriptsDirectory());
			}

			UI::PropertyRow("Athena Binary Path", ImGui::GetFrameHeight());

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(" ... ").x - 2 * ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x);
			String binaryPath = m_Config.AthenaBinaryDirectory.string();
			if (UI::TextInput("AthenBinaryInputText", binaryPath))
				m_Config.AthenaBinaryDirectory = binaryPath;

			ImGui::SameLine();
			ImGui::PushID("SelectAthenaBinary");
			if (ImGui::Button(" ... "))
			{
				FilePath path = FileDialogs::OpenDirectory("Select Athena Binary Directory", Project::GetProjectDirectory());
				if (!path.empty())
					m_Config.AthenaBinaryDirectory = std::filesystem::relative(path, Project::GetScriptsDirectory());
			}
			ImGui::PopID();

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Physics") && UI::BeginPropertyTable())
		{
			int velocityIter = m_Config.VelocityIterations;
			if (UI::PropertySlider("Velocity Iterations", &velocityIter, 1, 10))
				m_Config.VelocityIterations = velocityIter;

			int posIter = m_Config.PositionIterations;
			if (UI::PropertySlider("Position Iterations", &posIter, 1, 10))
				m_Config.PositionIterations = posIter;

			UI::PropertyDrag("Gravity", &m_Config.Gravity, 0.1f);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (ImGui::Button("Save"))
		{
			Project::GetActive()->GetConfig() = m_Config;
			Project::SaveActive();
		}

		ImGui::SameLine();

		if(ImGui::Button("Close"))
		{
			PanelManager::ClosePanel(m_Name);
		}

		ImGui::End();
	}
}
