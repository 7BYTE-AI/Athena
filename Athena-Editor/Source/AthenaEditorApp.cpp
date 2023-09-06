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
		appinfo.AppConfig.EnableConsole = true;
		appinfo.AppConfig.EnableImGui = true;
		appinfo.AppConfig.WorkingDirectory = "";
		appinfo.AppConfig.EngineResources = "../Athena/EngineResources";

		appinfo.RendererConfig.API = Renderer::API::OpenGL;

		appinfo.ScriptConfig.ScriptsFolder = "Assets/Scripts";

		appinfo.WindowInfo.Width = 1600;
		appinfo.WindowInfo.Height = 900;
		appinfo.WindowInfo.Title = "Athena Editor";
		appinfo.WindowInfo.VSync = false;
		appinfo.WindowInfo.Mode = WindowMode::Default;
		appinfo.WindowInfo.CustomTitlebar = true;
		appinfo.WindowInfo.WindowResizeable = true;
		appinfo.WindowInfo.Icon = "EditorResources/Icons/Logo/no-background.png";

		Application* application = new AthenaEditor(appinfo);
		
		EditorConfig editorConfig;
		editorConfig.EditorResources = "EditorResources/";

		application->PushLayer(CreateRef<EditorLayer>(editorConfig));

		return application;
	}
}
