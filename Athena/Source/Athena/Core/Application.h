#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/LayerStack.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/ApplicationEvent.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	class ATHENA_API ImGuiLayer;


	struct ApplicationDescription
	{
		WindowDescription WindowDesc;
		Renderer::API API = Renderer::OpenGL;
		bool EnableImGui = true;
		bool EnableConsole = true;
		FilePath WorkingDirectory = FilePath();
		FilePath ScriptsFolder = FilePath();
	};


	class ATHENA_API Application
	{
	public:
		struct Statistics;

	public:
		Application(const ApplicationDescription& appdesc);
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Ref<Layer> layer);
		void PushOverlay(Ref<Layer> layer);

		inline Ref<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }
		inline Window& GetWindow() { return *m_Window; }
		inline const Statistics& GetStatistics() const { return m_Statistics; }

		void Close();

		inline static Application& Get() { return *s_Instance; }

	public:
		struct Statistics
		{
			Time FrameTime;
			Time Application_OnUpdate;
			Time Application_OnEvent;
			Time Application_OnImGuiRender;
			Time Window_OnUpdate;
		};

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		Scope<Window> m_Window;
		Ref<ImGuiLayer> m_ImGuiLayer;
		bool m_Running;
		bool m_Minimized;
		LayerStack m_LayerStack;

		Statistics m_Statistics;

	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}

