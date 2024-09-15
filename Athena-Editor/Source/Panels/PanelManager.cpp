#include "PanelManager.h"

#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"

#include "Panels/Panel.h"

#include <ImGui/imgui.h>


namespace Athena
{
	static const char* KeyToString(Keyboard::Key key)
	{
		switch (key)
		{
		case Keyboard::Space: return "Ctrl+Space";
		case Keyboard::I: return "Ctrl+I";
		case Keyboard::K: return "Ctrl+K";
		case Keyboard::J: return "Ctrl+J";
		}

		return NULL;
	}

	std::unordered_map<std::string_view, PanelDescription> PanelManager::m_Panels;


	void PanelManager::Shutdown()
	{
		m_Panels.clear();
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto& [key, panel] : m_Panels)
		{
			if (panel.IsOpen)
			{
				panel.PanelRef->OnImGuiRender();
			}
		}
	}

	void PanelManager::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(PanelManager::OnKeyPressedEvent);
	}

	void PanelManager::ImGuiRenderAsMenuItems()
	{
		for (auto& [key, panel] : m_Panels)
		{
			if (panel.IsHideable)
			{
				if (ImGui::MenuItem(panel.PanelRef->GetName().data(), KeyToString(panel.HotKey), panel.IsOpen))
				{
					panel.IsOpen = !panel.IsOpen;
				}
			}
		}
	}

	void PanelManager::AddPanel(const Ref<Panel>& panel, bool isHideable, bool defaultOpen)
	{
		PanelDescription desc;
		desc.PanelRef = panel;
		desc.IsOpen = defaultOpen;
		desc.IsHideable = isHideable;
		m_Panels[panel->GetName()] = desc;
	}

	void PanelManager::AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey, bool defaultOpen)
	{
		PanelDescription desc;
		desc.PanelRef = panel;
		desc.IsOpen = defaultOpen;
		desc.IsHideable = true;
		desc.HotKey = hotkey;
		m_Panels[panel->GetName()] = desc;
	}

	void PanelManager::ClosePanel(std::string_view name)
	{
		ATN_CORE_ASSERT(m_Panels.contains(name));
		m_Panels.at(name).IsOpen = false;
	}

	void PanelManager::OpenPanel(std::string_view name)
	{
		ATN_CORE_ASSERT(m_Panels.contains(name));
		m_Panels.at(name).IsOpen = true;
	}

	bool PanelManager::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		bool ctrl = Input::IsKeyPressed(Keyboard::LCtrl) || Input::IsKeyPressed(Keyboard::RCtrl);

		if (ctrl)
		{
			auto keycode = event.GetKeyCode();
			for (auto& [key, panel] : m_Panels)
			{
				if (panel.HotKey != Keyboard::Escape && panel.HotKey == keycode)
				{
					panel.IsOpen = !panel.IsOpen;
				}
			}
		}

		return false;
	}
}
