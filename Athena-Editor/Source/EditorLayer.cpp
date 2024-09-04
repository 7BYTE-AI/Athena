#include "EditorLayer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Input/Input.h"
#include "Athena/Project/Project.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/FontGeometry.h"

#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/ProfilingPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ViewportPanel.h"

#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    EditorLayer::EditorLayer(const EditorConfig& config)
        : Layer("EditorLayer"), m_Config(config)
    {

    }

    void EditorLayer::OnAttach()
    {
        m_EditorCtx = Ref<EditorContext>::Create();
        m_EditorCtx->ActiveScene = nullptr;
        m_EditorCtx->SceneState = SceneState::Edit;
        m_EditorCtx->SelectedEntity = {};

        if (m_Config.SelectProjectManually)
        {
            if (!OpenProject())
            {
                // TODO: Editor Launcher
#if 1
                NewProject("SandBox", "SandBoxProject/");
#else
                Application::Get().Close();
                return;
#endif
            }
        }
        else
        {
            OpenProject(m_Config.StartProject);
        }

        if (m_EditorCtx->ActiveScene == nullptr)
            NewScene();

        m_ViewportRenderer = SceneRenderer::Create();
        m_Renderer2D = SceneRenderer2D::Create(m_ViewportRenderer->GetRender2DPass());

        m_ViewportRenderer->SetOnRender2DCallback(
            [this]() { OnRender2D(); });

        m_ViewportRenderer->SetOnViewportResizeCallback(
            [this](uint32 width, uint32 height) { m_Renderer2D->OnViewportResize(width, height); });
        
        m_EditorCamera = Ref<FirstPersonCamera>::Create(Math::Radians(45.f), 16.f / 9.f, 1.f, 200.f);

        InitUI();

#if 0
        OpenScene("Assets/Scenes/Default.atn");
#endif
    }

    void EditorLayer::OnDetach()
    {
        m_ViewportRenderer.Release();
        PanelManager::Shutdown();
        EditorResources::Shutdown();
        Application::Get().GetWindow().SetTitlebarHitTestCallback(nullptr);
    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        ATN_PROFILE_FUNC();

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);

        const auto& vpDesc = viewportPanel->GetDescription();
        const auto& rendererSize = m_EditorCtx->ActiveScene->GetViewportSize();

        if (vpDesc.Size.x > 0 && vpDesc.Size.y > 0 &&
            (rendererSize.x != vpDesc.Size.x || rendererSize.y != vpDesc.Size.y))
        {
            ATN_PROFILE_SCOPE("EditorLayer::OnViewportResize");

            m_ViewportRenderer->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCamera->SetViewportSize(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCtx->ActiveScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        }

        if ((Input::GetCursorMode() == CursorMode::Disabled || vpDesc.IsHovered) && !ImGuizmo::IsUsing() && (m_EditorCtx->SceneState != SceneState::Play))
            m_EditorCamera->OnUpdate(frameTime);

        switch (m_EditorCtx->SceneState)
        {
        case SceneState::Edit:
        {
            m_EditorCtx->ActiveScene->OnUpdateEditor(frameTime);
            m_EditorCtx->ActiveScene->OnRender(m_ViewportRenderer, *m_EditorCamera);
            break;
        }
        case SceneState::Play:
        {
            m_EditorCtx->ActiveScene->OnUpdateRuntime(frameTime);
            m_EditorCtx->ActiveScene->OnRender(m_ViewportRenderer);
            break;
        }
        case SceneState::Simulation:
        {
            m_EditorCtx->ActiveScene->OnUpdateSimulation(frameTime);
            m_EditorCtx->ActiveScene->OnRender(m_ViewportRenderer, *m_EditorCamera);
            break;
        }
        }
    }

    void EditorLayer::OnImGuiRender()
    {
        ATN_PROFILE_FUNC();

        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetTheme().Titlebar);
        //ImGui::PushStyleColor(ImGuiCol_Border, UI::GetTheme().Accent);

        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);

        //ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        m_Titlebar->OnImGuiRender();
        ImGui::SetCursorPosY(m_Titlebar->GetHeight());

        // Dockspace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;
        ImGui::DockSpace(ImGui::GetID("MyDockspace"));
        style.WindowMinSize.x = minWinSizeX;

        PanelManager::OnImGuiRender();

        if (UI::BeginPopupModal("About", ImGuiWindowFlags_AlwaysAutoResize))
            DrawAboutModal();

        if (UI::BeginPopupModal("Theme Editor"))
            DrawThemeEditor();

        if (UI::BeginPopupModal("New Project"))
            DrawNewProjectModal();

        if (UI::BeginPopupModal("New Script"))
            DrawNewScriptModal();

        ImGui::End();

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        m_EditorCamera->SetMoveSpeedLevel(m_EditorCtx->EditorSettings.CameraSpeedLevel);
        m_EditorCamera->SetNearClip(m_EditorCtx->EditorSettings.NearFarClips.x);
        m_EditorCamera->SetFarClip(m_EditorCtx->EditorSettings.NearFarClips.y);

        if (m_EditorCtx->SelectedEntity)
        {
            Entity entity = m_EditorCtx->SelectedEntity;
            if (entity.HasComponent<StaticMeshComponent>())
            {
                Ref<StaticMesh> mesh = entity.GetComponent<StaticMeshComponent>().Mesh;
                WorldTransformComponent& transform = entity.GetComponent<WorldTransformComponent>();
                m_ViewportRenderer->SubmitSelectionContext(mesh, transform.AsMatrix());
            }
        }
    }

    Entity EditorLayer::DuplicateEntity(Entity entity)
    {
        if (entity && m_EditorCtx->SceneState == SceneState::Edit)
        {
			Entity newEntity = m_EditorScene->DuplicateEntity(entity);
            m_EditorCtx->SelectedEntity = newEntity;
			return newEntity;
        }

        return entity;
    }

    void EditorLayer::InitUI()
    {
        EditorResources::Init(m_Config.EditorResources);

        Application::Get().GetImGuiLayer()->BlockEvents(false);
        Application::Get().GetWindow().SetTitlebarHitTestCallback([this]() { return m_Titlebar->IsHovered(); });

        m_ImGuizmoLayer = Ref<ImGuizmoLayer>::Create(m_EditorCtx, m_EditorCamera);

        UI::RegisterPopup("About", true);
        UI::RegisterPopup("Theme Editor");
        UI::RegisterPopup("New Project", true);
        UI::RegisterPopup("New Script", true);

        m_Titlebar = Ref<Titlebar>::Create(m_EditorCtx);

        m_Titlebar->SetMenubarCallback([this]()
            {
                if (ImGui::BeginMenu("File"))
                {
                    bool isEdit = m_EditorCtx->SceneState == SceneState::Edit;

                    if (ImGui::MenuItem("New Project", NULL, false))
                        UI::OpenPopup("New Project");

                    if (ImGui::MenuItem("Open Project...", NULL, false))
                        OpenProject();

                    if (ImGui::MenuItem("Save Project", NULL, false))
                        SaveProject();

                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::MenuItem("New Scene", NULL, false, isEdit))
                        NewScene();

                    if (ImGui::MenuItem("Open Scene...", NULL, false, isEdit))
                        OpenScene();

                    if (ImGui::MenuItem("Save Scene As...", NULL, false, isEdit))
                        SaveSceneAs();

                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::MenuItem("Save All", "Ctrl+S", false, isEdit))
                        SaveAll();

                    if (ImGui::MenuItem("Exit", NULL, false))
                        Application::Get().Close();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    PanelManager::ImGuiRenderAsMenuItems();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Theme Editor"))
                    {
                        UI::OpenPopup("Theme Editor");
                    }

                    if (ImGui::MenuItem("Take Screenshot"))
                    {
                        Ref<Texture2D> image = m_ViewportRenderer->GetFinalImage();

                        std::ostringstream oss;
                        auto t = std::time(nullptr);
                        auto tm = *std::localtime(&t);
                        oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");

                        String dateTime = oss.str();

                        if (!FileSystem::Exists("Screenshots"))
                            FileSystem::CreateDirectory("Screenshots");

                        FilePath path = std::format("Screenshots/Viewport_{}.png", dateTime);
                        TextureExporter::ExportPNG(path, image);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Scripting"))
                {
                    EditorSettings& settings = m_EditorCtx->EditorSettings;
                    bool isRuntime = m_EditorCtx->SceneState == SceneState::Play;

                    if (ImGui::MenuItem("Reload Scripts", NULL, false, !isRuntime))
                    {
                        ScriptEngine::ReloadScripts();
                    }

                    if (ImGui::MenuItem("New Script"))
                    {
                        UI::OpenPopup("New Script");
                    }
#ifdef ATN_PLATFORM_WINDOWS
                    if (ImGui::MenuItem("Open In Visual Studio"))
                    {
                        ScriptEngine::OpenInVisualStudio();
                    }
#endif

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("About", NULL, false))
                    {
                        UI::OpenPopup("About");
                    }

                    if (ImGui::MenuItem("Github", NULL, false))
                    {
                        Platform::OpenInBrowser(TEXT("https://github.com/Algor1tm/Athena"));
                    }
                    
                    ImGui::EndMenu();
                }
            });

        auto viewportPanel = Ref<ViewportPanel>::Create(VIEWPORT_PANEL_ID, m_EditorCtx);

        viewportPanel->SetImGuizmoLayer(m_ImGuizmoLayer);
        viewportPanel->SetViewportRenderer(m_ViewportRenderer);
        viewportPanel->SetDragDropCallback([this]()
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    std::string_view path = (const char*)payload->Data;
                    FilePath ext = FilePath(path).extension();
                    //Scene Drag/Drop
                    if (m_EditorCtx->SceneState == SceneState::Edit && ext == ".atn\0")
                    {
                        OpenScene(path.data());
                    }
                    //Texture Drag/Drop on Entity
                    else if (m_EditorCtx->SceneState == SceneState::Edit && ext == ".png\0")
                    {
                        Entity target = GetEntityByCurrentMousePosition();
                        if (target != Entity{})
                        {
                            if (target.HasComponent<SpriteComponent>())
                            {
                                auto& sprite = target.GetComponent<SpriteComponent>();
                                sprite.Texture = TextureImporter::Load(path, true);
                                sprite.Color = LinearColor::White;
                                m_EditorCtx->SelectedEntity = target;
                            }
                        }
                    }
                    // Mesh Drag/Drop
                    else if (m_EditorCtx->SceneState == SceneState::Edit && (ext == ".obj\0" || ext == ".fbx" || ext == ".x3d" || ext == ".gltf" || ext == ".blend"))
                    {
                        Ref<StaticMesh> mesh = StaticMesh::Create(path);
                        if (mesh)
                        {
                            Entity entity = m_EditorCtx->ActiveScene->CreateEntity();
                            entity.GetComponent<TagComponent>().Tag = mesh->GetName();
                            entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
                            m_EditorCtx->SelectedEntity = entity;
                        }
                    }
                }
            });

        viewportPanel->SetUIOverlayCallback([this]()
            {
                float windowPaddingY = 3.f;

                ImColor rectColor = IM_COL32(0, 0, 0, 70);
                ImVec2 rectPadding = { -1.f, 0.f };

                float buttonPaddingX = -1.f;
                ImVec2 buttonSize = { 28.f, 26.f };
                int32 buttonCount = 2;

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                float avail = ImGui::GetContentRegionAvail().x;

                float fullSize = 2 * rectPadding.x + buttonCount * buttonSize.x + (buttonCount - 1) * buttonPaddingX;
                ImVec2 rectMin = { (avail - fullSize) * 0.5f, windowPaddingY };
                rectMin.x = avail * 0.5f;
                rectMin.x += ImGui::GetCursorScreenPos().x;
                rectMin.y += ImGui::GetCursorScreenPos().y;

                drawList->AddRectFilled(rectMin, { rectMin.x + fullSize, rectMin.y + 2 * rectPadding.y + buttonSize.y }, rectColor, 4.f);

                ImColor tintNormal = IM_COL32(255, 255, 255, 255);

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { buttonPaddingX, 0.f });
                ImGui::SetCursorScreenPos({ rectMin.x + rectPadding.x, rectMin.y + rectPadding.y });

                Ref<Texture2D> playIcon = EditorResources::GetIcon("Viewport_Play");
                Ref<Texture2D> simulateIcon = EditorResources::GetIcon("Viewport_Simulate");
                Ref<Texture2D> stopIcon = EditorResources::GetIcon("Viewport_Stop");

                {
                    Ref<Texture2D> icon = m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Simulation ? playIcon : stopIcon;

                    if (ImGui::InvisibleButton("Play", buttonSize))
                    {
                        if (m_EditorCtx->SceneState == SceneState::Edit)
                            OnScenePlay();
                        else if (m_EditorCtx->SceneState == SceneState::Play)
                            OnSceneStop();
                    }

                    UI::ButtonImage(icon, tintNormal, tintNormal, tintNormal);
                }

                ImGui::SameLine();

                {
                    Ref<Texture2D> icon = m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Play ? simulateIcon : stopIcon;

                    if (ImGui::InvisibleButton("Simulate", buttonSize))
                    {
                        if (m_EditorCtx->SceneState == SceneState::Edit)
                            OnSceneSimulate();
                        else if (m_EditorCtx->SceneState == SceneState::Simulation)
                            OnSceneStop();
                    }

                    UI::ButtonImage(icon, tintNormal, tintNormal, tintNormal);
                }

                ImGui::PopStyleVar();
            });

        PanelManager::AddPanel(viewportPanel, false);

        auto settingsPanel = Ref<SettingsPanel>::Create(SETTINGS_PANEL_ID, m_EditorCtx);
        settingsPanel->SetContext(m_ViewportRenderer);
        PanelManager::AddPanel(settingsPanel, Keyboard::I);

        auto sceneHierarchyPanel = Ref<SceneHierarchyPanel>::Create(SCENE_HIERARCHY_PANEL_ID, m_EditorCtx);
        PanelManager::AddPanel(sceneHierarchyPanel, Keyboard::J);

        auto contentBrowserPanel = Ref<ContentBrowserPanel>::Create(CONTENT_BORWSER_PANEL_ID, m_EditorCtx);
        PanelManager::AddPanel(contentBrowserPanel, Keyboard::Space);

        auto profilingPanel = Ref<ProfilingPanel>::Create(PROFILING_PANEL_ID, m_EditorCtx);
        profilingPanel->SetContext(m_ViewportRenderer);
        PanelManager::AddPanel(profilingPanel, Keyboard::K);

        m_IsUIInitialized = true;
    }

    Entity EditorLayer::GetEntityByCurrentMousePosition()
    {
        return Entity{};    // TODO: Not implemented
    }

    void EditorLayer::OnRender2D()
    {
        Ref<SceneRenderer2D> renderer2D = m_Renderer2D;

        renderer2D->SetLineWidth(3.f);

        if (m_EditorCtx->SceneState == SceneState::Play)
        {
            Entity camera = m_EditorCtx->ActiveScene->GetPrimaryCameraEntity();
            if (camera)
            {
				auto& runtimeCamera = camera.GetComponent<CameraComponent>().Camera;

				Matrix4 view = Math::AffineInverse(camera.GetComponent<WorldTransformComponent>().AsMatrix());
                renderer2D->BeginScene(view, runtimeCamera.GetProjectionMatrix());
            }
            else
            {
                renderer2D->BeginScene(Matrix4::Identity(), m_EditorCamera->GetProjectionMatrix());
            }
        }
        else
        {
            renderer2D->BeginScene(m_EditorCamera->GetViewMatrix(), m_EditorCamera->GetProjectionMatrix());
        }

        m_EditorCtx->ActiveScene->OnRender2D(renderer2D);

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        if (m_EditorCtx->EditorSettings.ShowPhysicsColliders)
        {
            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                for (auto entity : view)
                {
                    const auto& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation);
                    Vector2 scale = tc.Scale * Vector3(bc2d.Size * 2.f, 1.f);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(bc2d.Offset.y, bc2d.Offset.x, 0.f)).Rotate(tc.Rotation.z, Vector3::Back()).Translate(Vector3(translation, 0.001f));

                    renderer2D->DrawRect(transform, LinearColor::Green);
                }
            }

            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
                for (auto entity : view)
                {
                    const auto& [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation) + cc2d.Offset;
                    Vector2 scale = Vector2(tc.Scale) * Vector2(cc2d.Radius * 2);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(translation, 0.001f));

                    renderer2D->DrawCircle(transform, Renderer2DSpace::WorldSpace, LinearColor::Green, 0.05f * 0.5f / cc2d.Radius);
                }
            }
        }
        
        const Vector2 iconScale = Vector2( 0.075f, 0.075f ) * m_EditorCtx->EditorSettings.RendererIconsScale;
        if (m_EditorCtx->EditorSettings.ShowRendererIcons && m_EditorCtx->SceneState == SceneState::Edit)
        {
            auto camerasView = m_EditorScene->GetAllEntitiesWith<CameraComponent, WorldTransformComponent>();
            for (auto entity : camerasView)
            { 
                const auto& transform = camerasView.get<WorldTransformComponent>(entity);
                renderer2D->DrawBillboardFixedSize(transform.Translation, iconScale * 4.f / 3.f, EditorResources::GetIcon("Viewport_Camera"));
            }

            auto dirLightsView = m_EditorScene->GetAllEntitiesWith<DirectionalLightComponent, WorldTransformComponent>();
            for (auto entity : dirLightsView)
            {
                const auto& transform = dirLightsView.get<WorldTransformComponent>(entity);
                renderer2D->DrawBillboardFixedSize(transform.Translation, iconScale * 1.6f, EditorResources::GetIcon("Viewport_DirLight"));
            }

            auto pointLightsView = m_EditorScene->GetAllEntitiesWith<PointLightComponent, WorldTransformComponent>();
            for (auto entity : pointLightsView)
            {
                const auto& transform = pointLightsView.get<WorldTransformComponent>(entity);
                renderer2D->DrawBillboardFixedSize(transform.Translation, iconScale, EditorResources::GetIcon("Viewport_PointLight"));
            }

            auto spotLightsView = m_EditorScene->GetAllEntitiesWith<SpotLightComponent, WorldTransformComponent>();
            for (auto entity : spotLightsView)
            {
                const auto& transform = spotLightsView.get<WorldTransformComponent>(entity);
                renderer2D->DrawBillboardFixedSize(transform.Translation, iconScale * 1.6f, EditorResources::GetIcon("Viewport_SpotLight"));
            }

            auto skyLightsView = m_EditorScene->GetAllEntitiesWith<SkyLightComponent, WorldTransformComponent>();
            for (auto entity : skyLightsView)
            {
                const auto& transform = skyLightsView.get<WorldTransformComponent>(entity);
                renderer2D->DrawBillboardFixedSize(transform.Translation, iconScale, EditorResources::GetIcon("Viewport_SkyLight"));
            }
        }

        Entity selectedEntity = m_EditorCtx->SelectedEntity;
        if (selectedEntity)
        {
            LinearColor selectColor = { 1.f, 0.5f, 0.f, 1.f };
            const WorldTransformComponent& worldTransform = selectedEntity.GetComponent<WorldTransformComponent>();

            // 2D Outline
            if(selectedEntity.HasComponent<SpriteComponent>())
            {
                if(selectedEntity.GetComponent<SpriteComponent>().Space == Renderer2DSpace::WorldSpace)
                    renderer2D->DrawRect(worldTransform.AsMatrix(), selectColor);
            }
            else if (selectedEntity.HasComponent<CircleComponent>())
            {
                if (selectedEntity.GetComponent<CircleComponent>().Space == Renderer2DSpace::WorldSpace)
                    renderer2D->DrawRect(worldTransform.AsMatrix(), selectColor);
            }
            else if (selectedEntity.HasComponent<TextComponent>())
            {
                const TextComponent& textComponent = selectedEntity.GetComponent<TextComponent>();
                if (textComponent.Space == Renderer2DSpace::WorldSpace)
                {
                    TextAlignmentOptions options;
                    options.MaxWidth = textComponent.MaxWidth;
                    options.Kerning = textComponent.Kerning;
                    options.LineSpacing = textComponent.LineSpacing;
                    options.InvertY = false;

                    FontGeometry* fontGeometry = textComponent.Font->GetFontGeometry();
                    float height = fontGeometry->InitText(textComponent.Text, options);

                    Matrix4 transformMatrix = worldTransform.AsMatrix();

                    Vector4 p0 = Vector4(0, 0, 0, 1) * transformMatrix;
                    Vector4 p1 = Vector4(textComponent.MaxWidth, 0, 0, 1) * transformMatrix;
                    Vector4 p2 = Vector4(textComponent.MaxWidth, height, 0, 1) * transformMatrix;
                    Vector4 p3 = Vector4(0, height, 0, 1) * transformMatrix;

                    renderer2D->DrawLine(p0, p1, selectColor);
                    renderer2D->DrawLine(p1, p2, selectColor);
                    renderer2D->DrawLine(p2, p3, selectColor);
                    renderer2D->DrawLine(p3, p0, selectColor);
                }
            }
            // Camera frustum view
            else if (selectedEntity.HasComponent<CameraComponent>())
            {
                const SceneCamera& camera = selectedEntity.GetComponent<CameraComponent>().Camera;
                const CameraInfo& cInfo = camera.GetCameraInfo();

                float nearClip = cInfo.NearClip;
                float farClip = cInfo.FarClip;
                float fov = cInfo.FOV;
                float aspectRatio = camera.GetAspectRatio();

                std::array<Vector3, 4> nearPlane;
                std::array<Vector3, 4> farPlane;

                float planeHorHalf = Math::Tan(fov) * nearClip;
                float planeVertHalf = planeHorHalf / aspectRatio;
                nearPlane[0] = Vector3::Left() * planeHorHalf  + Vector3::Down() * planeVertHalf;
                nearPlane[1] = Vector3::Left() * planeHorHalf  + Vector3::Up() * planeVertHalf;
                nearPlane[2] = Vector3::Right() * planeHorHalf + Vector3::Up() * planeVertHalf;
                nearPlane[3] = Vector3::Right() * planeHorHalf + Vector3::Down() * planeVertHalf;

                planeHorHalf = Math::Tan(fov) * farClip;
                planeVertHalf = planeHorHalf / aspectRatio;
                farPlane[0] = Vector3::Left() * planeHorHalf + Vector3::Down() * planeVertHalf;
                farPlane[1] = Vector3::Left() * planeHorHalf + Vector3::Up() * planeVertHalf;
                farPlane[2] = Vector3::Right() * planeHorHalf + Vector3::Up() * planeVertHalf;
                farPlane[3] = Vector3::Right() * planeHorHalf + Vector3::Down() * planeVertHalf;

                for (uint32 i = 0; i < 4; ++i)
                {
                    nearPlane[i] += Vector3::Forward() * nearClip;
                    farPlane[i] += Vector3::Forward() * farClip;

                    nearPlane[i] = worldTransform.Rotation * nearPlane[i];
                    farPlane[i] = worldTransform.Rotation * farPlane[i];

                    nearPlane[i] += worldTransform.Translation;
                    farPlane[i] += worldTransform.Translation;
                }

                renderer2D->DrawLine(nearPlane[0], nearPlane[1], selectColor);
                renderer2D->DrawLine(nearPlane[1], nearPlane[2], selectColor);
                renderer2D->DrawLine(nearPlane[2], nearPlane[3], selectColor);
                renderer2D->DrawLine(nearPlane[3], nearPlane[0], selectColor);

                renderer2D->DrawLine(farPlane[0], farPlane[1], selectColor);
                renderer2D->DrawLine(farPlane[1], farPlane[2], selectColor);
                renderer2D->DrawLine(farPlane[2], farPlane[3], selectColor);
                renderer2D->DrawLine(farPlane[3], farPlane[0], selectColor);

                renderer2D->DrawLine(nearPlane[0], farPlane[0], selectColor);
                renderer2D->DrawLine(nearPlane[1], farPlane[1], selectColor);
                renderer2D->DrawLine(nearPlane[2], farPlane[2], selectColor);
                renderer2D->DrawLine(nearPlane[3], farPlane[3], selectColor);
            } 
            // Light Outline
            else if (selectedEntity.HasComponent<PointLightComponent>())
            {
                const Vector3& position = worldTransform.Translation;
                const auto& comp = selectedEntity.GetComponent<PointLightComponent>();
                float radius = comp.Radius;
                selectColor = comp.Color;

                constexpr uint32 linesCount = 36;
                constexpr float step = (2 * Math::PI<float>()) / linesCount;
                for (uint32 i = 0; i < linesCount; ++i)
                {
                    float angle = i * step;
                    Vector2 offset0 = { radius * Math::Cos(angle), radius * Math::Sin(angle) };
                    Vector2 offset1 = { radius * Math::Cos(angle + step), radius * Math::Sin(angle + step) };

                    Vector3 p0 = position + Vector3(offset0.x, offset0.y, 0.f );
                    Vector3 p1 = position + Vector3(offset1.x, offset1.y, 0.f);

                    renderer2D->DrawLine(p0, p1, selectColor);

					p0 = position + Vector3(offset0.x, 0.f, offset0.y);
					p1 = position + Vector3(offset1.x, 0.f, offset1.y);

                    renderer2D->DrawLine(p0, p1, selectColor);

					p0 = position + Vector3(0.f, offset0.x, offset0.y);
					p1 = position + Vector3(0.f, offset1.x, offset1.y);

                    renderer2D->DrawLine(p0, p1, selectColor);
                }
            }
            else if (selectedEntity.HasComponent<SpotLightComponent>())
            {
                auto& comp = selectedEntity.GetComponent<SpotLightComponent>();
                selectColor = comp.Color;
                float spotAngle = Math::Radians(comp.SpotAngle / 2.f);
                if (spotAngle == 0.f)
                    spotAngle = 0.001f;

                Vector3 direction = worldTransform.Rotation * Vector3::Forward();
                direction.Normalize();

                Vector3 origin = worldTransform.Translation;

                // Circle
                Vector3 circleCenter = origin + direction * (Math::Cos(spotAngle) * comp.Range);
                float radius = Math::Sin(spotAngle) * comp.Range;

                uint32 linesCount = 24;
                float step = (2 * Math::PI<float>()) / linesCount;

                for (uint32 i = 0; i < linesCount; ++i)
                {
                    float angle = i * step;
                    Vector3 offset0 = { radius * Math::Cos(angle), radius * Math::Sin(angle), 0.f };
                    Vector3 offset1 = { radius * Math::Cos(angle + step), radius * Math::Sin(angle + step), 0.f };

                    offset0 = worldTransform.Rotation * offset0;
                    offset1 = worldTransform.Rotation * offset1;

                    Vector3 p0 = circleCenter + offset0;
                    Vector3 p1 = circleCenter + offset1;

                    renderer2D->DrawLine(p0, p1, selectColor);
                }

                // Arcs horizontal and vertical
                circleCenter = origin;
                radius = comp.Range;
                float begin = Math::PI<float>() / 2.f - spotAngle;
                float end = Math::PI<float>() / 2.f + spotAngle;

                linesCount = 12;
                step = (end - begin) / linesCount;

                for (uint32 i = 0; i < linesCount; ++i)
                {
                    float angle = begin + i * step;
                    Vector2 offset0 = { radius * Math::Cos(angle), radius * Math::Sin(angle) };
                    Vector2 offset1 = { radius * Math::Cos(angle + step), radius * Math::Sin(angle + step) };

                    Vector3 horizonOff0 = { offset0.x, 0.f, offset0.y };
                    Vector3 horizonOff1 = { offset1.x, 0.f, offset1.y };

                    horizonOff0 = worldTransform.Rotation * horizonOff0;
                    horizonOff1 = worldTransform.Rotation * horizonOff1;

                    Vector3 h0 = circleCenter - horizonOff0;
                    Vector3 h1 = circleCenter - horizonOff1;

                    renderer2D->DrawLine(h0, h1, selectColor);

                    Vector3 verticalOff0 = { 0.f, offset0.x, offset0.y };
                    Vector3 verticalOff1 = { 0.f, offset1.x, offset1.y };

                    verticalOff0 = worldTransform.Rotation * verticalOff0;
                    verticalOff1 = worldTransform.Rotation * verticalOff1;

                    Vector3 v0 = circleCenter - verticalOff0;
                    Vector3 v1 = circleCenter - verticalOff1;

                    renderer2D->DrawLine(v0, v1, selectColor);

                    // Lines from origin on corners
                    if (i == 0)
                    {
                        renderer2D->DrawLine(origin, h0, selectColor);
                        renderer2D->DrawLine(origin, v0, selectColor);
                    }
                    else if (i == linesCount - 1)
                    {
                        renderer2D->DrawLine(origin, h1, selectColor);
                        renderer2D->DrawLine(origin, v1, selectColor);
                    }
                }

            }
        }

        renderer2D->EndScene();
    }

    void EditorLayer::DrawAboutModal()
    {
        auto logo = EditorResources::GetIcon("Logo");
        UI::DrawImage(logo, { 48, 48 });

        ImGui::SameLine();
        UI::ShiftCursorX(20.0f);

        ImGui::Text("Athena 3D Game Engine");

        if (UI::ButtonCentered("Close"))
        {
            UI::CloseCurrentPopup();
        }

        UI::EndPopup();
    }

    void EditorLayer::DrawThemeEditor()
    {
        UI::Theme& theme = UI::GetTheme();

        UI::ThemeEditor(theme);

        if (ImGui::Button("Reset"))
            theme = UI::Theme::DefaultDark();

        Application::Get().GetImGuiLayer()->UpdateImGuiTheme();

        ImGui::SameLine();
        if (ImGui::Button("Close"))
            UI::CloseCurrentPopup();

        UI::EndPopup();
    }

    void EditorLayer::DrawNewProjectModal()
    {
        static String projectName;
        UI::TextInputWithHint("Project Name", projectName);

        static String projectDir;
        UI::TextInputWithHint("Project Directory", projectDir);

        UI::ShiftCursorY(10);

        if (ImGui::Button("Close"))
        {
            UI::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Create"))
        {
            NewProject(projectName, projectDir);
            projectName.clear();
            projectDir.clear();

            UI::CloseCurrentPopup();
        }

        UI::EndPopup();
    }

    void EditorLayer::DrawNewScriptModal()
    {
        UI::PushFont(UI::Fonts::Default22);
        static String scriptName;
        UI::TextInputWithHint("Script Name", scriptName);
        UI::PopFont();

        UI::ShiftCursorY(10);

        if (ImGui::Button("Close"))
        {
            UI::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Create"))
        {
            ScriptEngine::CreateNewScript(scriptName);
            scriptName.clear();

            UI::CloseCurrentPopup();
        }

        UI::EndPopup();
    }

    void EditorLayer::OnScenePlay()
    {
        ATN_PROFILE_FUNC();

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);

        const auto& vpDesc = viewportPanel->GetDescription();

        if (m_EditorCtx->SceneState == SceneState::Simulation)
            OnSceneStop();

        m_EditorCtx->SceneState = SceneState::Play;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnRuntimeStart();
        m_EditorCtx->ActiveScene = m_RuntimeScene;

        if (m_EditorCtx->SelectedEntity)
            m_EditorCtx->SelectedEntity = m_RuntimeScene->GetEntityByUUID(m_EditorCtx->SelectedEntity.GetID());
    }

    void EditorLayer::OnSceneSimulate()
    {
        ATN_PROFILE_FUNC();

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);
        const auto& vpDesc = viewportPanel->GetDescription();

        m_EditorCtx->SceneState = SceneState::Simulation;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnSimulationStart();
        m_EditorCtx->ActiveScene = m_RuntimeScene;

        if (m_EditorCtx->SelectedEntity)
            m_EditorCtx->SelectedEntity = m_RuntimeScene->GetEntityByUUID(m_EditorCtx->SelectedEntity.GetID());
    }

    void EditorLayer::OnSceneStop()
    {
        ATN_PROFILE_FUNC();

        m_EditorCtx->SceneState = SceneState::Edit;

        m_EditorCtx->ActiveScene->OnRuntimeStop();
        m_EditorCtx->ActiveScene = m_EditorScene;

        if (m_EditorCtx->SelectedEntity)
            m_EditorCtx->SelectedEntity = m_EditorScene->GetEntityByUUID(m_EditorCtx->SelectedEntity.GetID());

        m_RuntimeScene.Release();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        ATN_PROFILE_FUNC();

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);
        const auto& vpDesc = viewportPanel->GetDescription();

        if (m_EditorCtx->SceneState != SceneState::Play && !ImGuizmo::IsUsing())
        {
            bool rightMB = Input::IsMouseButtonPressed(Mouse::Right);
            CursorMode currentMode = Input::GetCursorMode();

            if (rightMB && currentMode != CursorMode::Normal)
				m_EditorCamera->OnEvent(event);

            else if (rightMB && currentMode == CursorMode::Normal && vpDesc.IsHovered)
                Input::SetCursorMode(CursorMode::Disabled);

            else if (!rightMB && currentMode != CursorMode::Normal)
                Input::SetCursorMode(CursorMode::Normal);
        }

        m_ImGuizmoLayer->OnEvent(event);
        PanelManager::OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnMouseReleased));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.IsRepeat())
            return false;

        bool ctrl = event.IsCtrlPressed();
        bool isEdit = m_EditorCtx->SceneState == SceneState::Edit;

        switch (event.GetKeyCode())
        {
        case Keyboard::S:
        {
            if (ctrl && isEdit)
                SaveAll();
            break;
        }

        case Keyboard::F4: if (m_EditorCtx->SceneState == SceneState::Play || m_EditorCtx->SceneState == SceneState::Simulation) OnSceneStop(); break;
        case Keyboard::F5: if (m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Simulation) OnScenePlay(); break;
        case Keyboard::F6: if (m_EditorCtx->SceneState == SceneState::Edit) OnSceneSimulate(); break;
        
            //Entities
		case Keyboard::D: if (ctrl) DuplicateEntity(m_EditorCtx->SelectedEntity); break;
        case Keyboard::Escape: m_EditorCtx->SelectedEntity = {}; break;
        case Keyboard::Delete:
        {
            if (m_EditorCtx->SelectedEntity && m_EditorCtx->SceneState == SceneState::Edit)
            {
                m_EditorScene->DestroyEntity(m_EditorCtx->SelectedEntity);
                m_EditorCtx->SelectedEntity = {};
            }
            break;
        }
        case Keyboard::F11:
        {
            auto& window = Application::Get().GetWindow();
            WindowMode currentMode = window.GetWindowMode();
            if (currentMode == WindowMode::Fullscreen)
            {
                window.SetWindowMode(WindowMode::Maximized);
            }
            else
            {
                window.SetWindowMode(WindowMode::Fullscreen);
            }
            break;
        }
        }

        return false;
    }

    bool EditorLayer::OnMouseReleased(MouseButtonReleasedEvent& event) 
    {
        if (ImGuizmo::IsOver())
            return false;

        switch(event.GetMouseButton())
        {
        case Mouse::Left:
        {
            if (m_EditorCtx->SceneState != SceneState::Play)
            {
                Entity selectedEntity = GetEntityByCurrentMousePosition();
                if (selectedEntity)
                {
                    m_EditorCtx->SelectedEntity = selectedEntity;
                }
            }
            break;
        }
        }
        return false;
    }

    void EditorLayer::NewScene()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        m_CurrentScenePath = FilePath();

        m_RuntimeScene = nullptr;
        m_EditorScene = Ref<Scene>::Create();
        m_EditorCtx->ActiveScene = m_EditorScene;
        m_EditorCtx->SelectedEntity = {};

        ATN_CORE_INFO_TAG("EditorLayer", "Successfully created new scene");
    }

    void EditorLayer::SaveSceneAs()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::SaveFile(TEXT("Athena Scene (*atn)\0*.atn\0"));
        if (!filepath.empty())
            SaveSceneAs(filepath);
        else
            ATN_CORE_ERROR_TAG("EditorLayer", "Invalid filepath to save scene {}", filepath);
    }

    void EditorLayer::SaveSceneAs(const FilePath& path)
    {
        SceneSerializer serializer(m_EditorCtx->ActiveScene);
        serializer.SerializeToFile(path.string());
        ATN_CORE_INFO_TAG("EditorLayer", "Successfully saved scene into {}", path);
    }

    void EditorLayer::OpenScene()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::OpenFile(TEXT("Athena Scene (*.atn)\0*.atn\0"));
        if (!filepath.empty())
            OpenScene(filepath);
        else
            ATN_CORE_ERROR_TAG("EditorLayer", "Invalid filepath to loaded scene {}", filepath);
    }

    void EditorLayer::OpenScene(const FilePath& path)
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = Ref<Scene>::Create();

        SceneSerializer serializer(newScene);
        if(serializer.DeserializeFromFile(path.string()))
        {
            m_CurrentScenePath = path;
            ATN_CORE_INFO_TAG("EditorLayer", "Successfully load scene from {}", path);
        }

        m_RuntimeScene = nullptr;
        m_EditorScene = newScene;
        m_EditorCtx->ActiveScene = newScene;
        m_EditorCtx->SelectedEntity = {};
    }

    void EditorLayer::NewProject(const String& name, const FilePath& path)
    {
        Project::New(name, path);

        FilePath cmakePath = m_Config.EditorResources / "Scripting/CMakeLists.txt";
        FilePath genProjectsPath = m_Config.EditorResources / "Scripting/VS2022-GenProjects.bat";

        FileSystem::Copy(cmakePath, Project::GetScriptsDirectory() / "CMakeLists.txt");
        FileSystem::Copy(genProjectsPath, Project::GetScriptsDirectory() / "VS2022-GenProjects.bat");
    }

    bool EditorLayer::OpenProject()
    {
        FilePath filepath = FileDialogs::OpenFile(TEXT("Athena Project (*.atproj)\0*.atproj\0"));
        if (filepath.empty())
            return false;

        OpenProject(filepath);
        return true;
    }

    void EditorLayer::OpenProject(const FilePath& path)
    {
        if (Project::Load(path))
        {
            ScriptEngine::InitProject();

            OpenScene(Project::GetAssetFileSystemPath(Project::GetActive()->GetConfig().StartScene));

            if (m_IsUIInitialized)
            {
                Ref<ContentBrowserPanel> contentBrowser = PanelManager::GetPanel<ContentBrowserPanel>(CONTENT_BORWSER_PANEL_ID);
                contentBrowser->Refresh();
            }
        }
    }

    void EditorLayer::SaveProject()
    {
        Project::SaveActive();
    }

    void EditorLayer::SaveAll()
    {
        SaveProject();

        if (m_CurrentScenePath == FilePath())
            SaveSceneAs();
        else
            SaveSceneAs(m_CurrentScenePath);
    }
}
