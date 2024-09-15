#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Input/Event.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/Keyboard.h"
#include "Panels/Panel.h"

#include <unordered_map>


#define SCENE_HIERARCHY_PANEL_ID  "SceneHierarchy"
#define CONTENT_BROWSER_PANEL_ID  "ContentBrowser"
#define PROFILING_PANEL_ID		  "Profiling"
#define MAIN_VIEWPORT_PANEL_ID	  "MainViewport"
#define SETTINGS_PANEL_ID		  "Settings"
#define PROJECT_SETTINGS_PANEL_ID "ProjectSettings"


namespace Athena
{
	struct PanelDescription
	{
		Ref<Panel> PanelRef;
		bool IsOpen = true;
		bool IsHideable = true;
		Keyboard::Key HotKey = Keyboard::Escape;
	};

	class PanelManager
	{
	public:
		static void Shutdown();

		static void OnImGuiRender();
		static void OnEvent(Event& event);

		static void ImGuiRenderAsMenuItems();

		static void AddPanel(const Ref<Panel>& panel, bool isHideable = true, bool defaultOpen = true);
		static void AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey, bool defaultOpen = true);

		static void ClosePanel(std::string_view name);
		static void OpenPanel(std::string_view name);

		template<typename T>
		static Ref<T> GetPanel(std::string_view name)
		{
			return m_Panels.at(name).PanelRef.As<T>();
		}

	private:
		static bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		static std::unordered_map<std::string_view, PanelDescription> m_Panels;
	};
}
