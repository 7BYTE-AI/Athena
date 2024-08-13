#pragma once

#include "Athena/Core/Layer.h"

#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EditorCamera.h"

#include "Athena/Scene/Entity.h"

#include "EditorContext.h"
#include "ImGuizmoLayer.h"
#include "Titlebar.h"


namespace Athena
{
	class Scene;


	struct EditorConfig
	{
		FilePath EditorResources;
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer(const EditorConfig& config);

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Time frameTime) override;
		void OnImGuiRender() override;
		void OnEvent(Event& event) override;

	private:
		Entity DuplicateEntity(Entity entity);

		void InitUI();
		void OnRender2D();
		void DrawAboutModal();
		void DrawThemeEditor();
		void DrawCreateScriptModal();

		Entity GetEntityByCurrentMousePosition();

		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseReleased(MouseButtonReleasedEvent& event);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();

		void NewScene();
		void SaveSceneAs();
		void SaveSceneAs(const FilePath& path);
		void OpenScene();
		void OpenScene(const FilePath& path);

	private:
		EditorConfig m_Config;

		Ref<SceneRenderer> m_ViewportRenderer;
		Ref<SceneRenderer2D> m_Renderer2D;
		Ref<EditorContext> m_EditorCtx;
		Ref<EditorCamera> m_EditorCamera;
		Ref<ImGuizmoLayer> m_ImGuizmoLayer;

		Ref<Titlebar> m_Titlebar;

		Ref<Scene> m_EditorScene, m_RuntimeScene;
		FilePath m_CurrentScenePath;
	};
}
