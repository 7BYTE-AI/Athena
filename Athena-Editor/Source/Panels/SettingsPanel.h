#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Panels/Panel.h"


namespace Athena
{
	class SettingsPanel: public Panel
	{
	public:
		SettingsPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
		void SetContext(const Ref<SceneRenderer>& renderer) { m_ViewportRenderer = renderer; }

	private:
		Ref<SceneRenderer> m_ViewportRenderer;
	};
}
