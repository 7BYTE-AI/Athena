#pragma once

#include "Athena/Core/Core.h"
#include "EditorContext.h"


namespace Athena
{
	class Panel
	{
	public:
		Panel(std::string_view name, Ref<EditorContext> context)
			: m_Name(name), m_EditorCtx(*context)
		{
			
		}

		virtual ~Panel() = default;

		virtual void OnImGuiRender() = 0;

		std::string_view GetName() { return m_Name; }

	protected:
		std::string_view m_Name;
		EditorContext& m_EditorCtx;
	};
}
