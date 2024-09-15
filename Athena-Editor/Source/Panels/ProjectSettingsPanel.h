#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Project/Project.h"
#include "Panels/Panel.h"


namespace Athena
{
	class ProjectSettingsPanel : public Panel
	{
	public:
		ProjectSettingsPanel(const Ref<EditorContext>& context);
		virtual void OnImGuiRender() override;

		void OnProjectUpdate();

	private:
		ProjectConfig m_Config;
	};
}
