#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/PlatformUtils.h"

#include "Athena/Renderer/SceneRenderer.h"

#include "Panels/Panel.h"

#include <array>


namespace Athena
{
	class ProfilingPanel : public Panel
	{
	public:
		ProfilingPanel(const Ref<EditorContext>& context);

		void SetContext(Ref<SceneRenderer> sceneRenderer) { m_SceneRenderer = sceneRenderer; }
		virtual void OnImGuiRender() override;

	private:
		Ref<SceneRenderer> m_SceneRenderer;
	};
}
