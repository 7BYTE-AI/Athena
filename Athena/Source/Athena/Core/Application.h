#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/LayerStack.h"
#include "Athena/Core/Window.h"

#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Input/WindowEvent.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Scripting/ScriptEngine.h"

#include <queue>


namespace Athena
{
	struct AppConfig
	{
		String Name = "";
		bool EnableImGui = true;
		FilePath WorkingDirectory = FilePath();
		FilePath EngineResourcesPath = FilePath();
		bool CleanCacheOnLoad = false;
	};

	struct ApplicationCreateInfo
	{
		AppConfig AppConfig;
		LogConfig LogConfig;
		RendererConfig RendererConfig;
		ScriptConfig ScriptConfig;
		WindowCreateInfo WindowInfo;
	};

	struct ApplicationStatistics
	{
		Time FrameTime;
		Time CPUWait;
		Time GPUWait;
		Time Application_ProcessEvents;
		Time Application_OnUpdate;
		Time Application_RenderImGui;
		Time SwapChain_Present;
		Time SwapChain_AcquireImage;
		Time Renderer_QueueSubmit;
	};

	class ATHENA_API Application
	{
	public:
		Application(const ApplicationCreateInfo& appinfo);
		virtual ~Application();

		void Run();

		void PushLayer(const Ref<Layer>& layer);
		void PushOverlay(const Ref<Layer>& layer);

		void SubmitToMainThread(const std::function<void()>& func);

		const AppConfig GetConfig() const { return m_Config; }

		const Ref<ImGuiLayer>& GetImGuiLayer() { return m_ImGuiLayer; }
		Window& GetWindow() { return *m_Window; }
		ApplicationStatistics& GetStats() { return m_Statistics; }

		void Close();

		inline static Application& Get() { return *s_Instance; }

	private:
		void ProcessEvents();
		void ExecuteMainThreadQueue();
		void RenderImGui();

		void QueueEvent(const Ref<Event>& event);
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		void CreateMainWindow(WindowCreateInfo info);
		void InitImGui();

		void ResetStats();

	private:
		AppConfig m_Config;
		Scope<Window> m_Window;
		Ref<ImGuiLayer> m_ImGuiLayer;
		bool m_Running;
		bool m_Minimized;
		LayerStack m_LayerStack;

		std::queue<Ref<Event>> m_EventQueue;
		std::queue<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;

		ApplicationStatistics m_Statistics;

	private:
		static Application* s_Instance;
	};

	// Defined by user
	Application* CreateApplication();
}
