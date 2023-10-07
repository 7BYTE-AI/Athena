#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	m_SceneRenderer = SceneRenderer::Create();

	//m_Scene = CreateRef<Scene>();

	//SceneSerializer serializer(m_Scene);
	//if (serializer.DeserializeFromFile(m_ScenePath.string()))
	//{
	//	ATN_ERROR_TAG("SandBoxLayer", "Failed to load scene at '{}'", m_ScenePath);
	//}

	//m_Scene->OnRuntimeStart();

	//auto& settings = SceneRenderer::GetSettings();

	//settings.BloomSettings.EnableBloom = false;
	//settings.ShadowSettings.FarPlaneOffset = 150.f;
	//settings.BloomSettings.Intensity = 1.2f;
}

void SandBoxLayer::OnDetach()
{
	m_SceneRenderer->Shutdown();
}

void SandBoxLayer::OnUpdate(Time frameTime)
{
	Renderer::BeginFrame();

	//m_Scene->OnUpdateRuntime(frameTime, m_SceneRenderer);
	m_SceneRenderer->RenderTest();

	Renderer::EndFrame();
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	//m_Scene->OnViewportResize(event.GetWidth(), event.GetHeight());
	m_SceneRenderer->OnWindowResized(event.GetWidth(), event.GetHeight());

	return false;
}

void SandBoxLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
}
