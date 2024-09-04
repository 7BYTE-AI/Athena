#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"

#include "EditorContext.h"


namespace Athena
{
	class Titlebar : public RefCounted
	{
	public:
		Titlebar(const Ref<EditorContext>& editorCtx);

		void OnImGuiRender();
		void SetMenubarCallback(const std::function<void()>& callback) { m_MenubarCallback = callback; }

		bool IsHovered() { return m_Hovered; }
		float GetHeight() { return m_Height; }

	private:
		Ref<EditorContext> m_EditorCtx;
		bool m_Hovered;
		const float m_Height = 58.f;

		std::function<void()> m_MenubarCallback;
	};
}
