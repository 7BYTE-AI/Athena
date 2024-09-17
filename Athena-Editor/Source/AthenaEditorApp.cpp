#include <EntryPoint.h>
#include "Athena/Core/Application.h"

#include "EditorLayer.h"


namespace Athena
{
	class AthenaEditor : public Application
	{
	public:
		AthenaEditor(const ApplicationCreateInfo& appinfo)
			: Application(appinfo)
		{

		}

		~AthenaEditor()
		{
			
		}
	};


	Application* CreateApplication()
	{
		ApplicationCreateInfo appinfo;

		appinfo.AppConfig.Name = "Athena Editor";
		appinfo.AppConfig.EnableImGui = true;
		appinfo.AppConfig.WorkingDirectory = "";
		appinfo.AppConfig.EngineResourcesPath = "../Athena/EngineResources";
		appinfo.AppConfig.CleanCacheOnLoad = false;

		appinfo.LogConfig.EnableConsole = true;
		appinfo.LogConfig.FileLocation = "Sandbox.log";

		appinfo.RendererConfig.API = Renderer::API::Vulkan;
		appinfo.RendererConfig.MaxFramesInFlight = 3;

		appinfo.ScriptConfig.EnableDebug = true;

		appinfo.WindowInfo.Width = 1600;
		appinfo.WindowInfo.Height = 900;
		appinfo.WindowInfo.Title = "Athena Editor";
		appinfo.WindowInfo.VSync = true;
		appinfo.WindowInfo.StartMode = WindowMode::Maximized;
		appinfo.WindowInfo.CustomTitlebar = true;
		appinfo.WindowInfo.WindowResizeable = true;
		appinfo.WindowInfo.Icon = "EditorResources/Icons/Logo/LogoBlack.png";

		Application* application = new AthenaEditor(appinfo);
		
		EditorConfig editorConfig;
		editorConfig.EditorResources = "EditorResources";
		editorConfig.SelectProjectManually = false;
		editorConfig.StartProject = "SandBoxProject/SandBox.atproj";

		application->PushLayer(Ref<EditorLayer>::Create(editorConfig));

		return application;
	}
}
