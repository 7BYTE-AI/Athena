#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/Entity.h"


namespace Athena
{
	enum class SceneState
	{
		Edit = 0, Play = 1, Simulation = 2
	};

	struct EditorSettings
	{
		bool GizmosLocalTransform = true;
		float CameraSpeedLevel = 0.3f;
		bool ShowRendererIcons = false;
		float RendererIconsScale = 1.f;
		Vector2 NearFarClips = { 1.f, 200.f };
		bool ShowPhysicsColliders = false;
	};

	struct EditorContext
	{
		Entity SelectedEntity;
		EditorSettings EditorSettings;
		Ref<Scene> ActiveScene;
		SceneState SceneState;
	};
}
