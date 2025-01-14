#include <EntryPoint.h>
#include "Athena/Core/Application.h"

#include "SandBoxLayer.h"

using namespace Athena;


class SandBox : public Application
{
public:
	SandBox(const ApplicationCreateInfo& appdesc)
		: Application(appdesc)
	{

	}

	~SandBox()
	{

	}
};


namespace Athena
{
	Application* CreateApplication()
	{
		ApplicationCreateInfo appinfo;

		appinfo.AppConfig.Name = "SandBox";
		appinfo.AppConfig.EnableImGui = false;
		appinfo.AppConfig.EnableConsole = true;
		appinfo.AppConfig.WorkingDirectory = "../Athena-Editor";
		appinfo.AppConfig.EngineResourcesPath = "../Athena/EngineResources";
		appinfo.AppConfig.CleanCacheOnLoad = false;

		appinfo.RendererConfig.API = Renderer::API::Vulkan;
		appinfo.RendererConfig.MaxFramesInFlight = 3;

		appinfo.ScriptConfig.ScriptsFolder = "Assets/Scripts";

		appinfo.WindowInfo.Width = 1600;
		appinfo.WindowInfo.Height = 900;
		appinfo.WindowInfo.Title = "SandBox";
		appinfo.WindowInfo.VSync = false;
		appinfo.WindowInfo.StartMode = WindowMode::Maximized;
		appinfo.WindowInfo.CustomTitlebar = false;
		appinfo.WindowInfo.WindowResizeable = true;
		appinfo.WindowInfo.Icon = "EditorResources/Icons/Logo/LogoBlack.png";

		Application* app = new SandBox(appinfo);

		FilePath scene = "Assets/Scenes/Sponza.atn";

		app->PushLayer(Ref<SandBoxLayer>::Create(scene));

		return app;
	}
}
