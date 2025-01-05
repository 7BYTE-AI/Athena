#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Input/KeyEvent.h"

#include "Athena/Scene/Entity.h"

#include "EditorContext.h"

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
	class ViewportPanel;
	class EditorCamera;


	class ImGuizmoLayer
	{
	public:
		friend class ViewportPanel;

	public:
		ImGuizmoLayer(const Ref<EditorContext>& context, const Ref<EditorCamera>& camera);

		void OnImGuiRender();
		void OnEvent(Event& event);

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		ImGuizmo::OPERATION m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

		Ref<EditorContext> m_EditorCtx;
		Ref<EditorCamera> m_EditorCamera;

		ViewportPanel* m_ViewportPanel = nullptr;
	};
}
