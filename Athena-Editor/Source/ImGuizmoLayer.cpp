#include "ImGuizmoLayer.h"

#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Input/Input.h"
#include "Athena/Scene/Components.h"
#include "Panels/ViewportPanel.h"


namespace Athena
{
    ImGuizmoLayer::ImGuizmoLayer(const Ref<EditorContext>& context, const Ref<EditorCamera>& camera)
    {
        m_EditorCtx = context;
        m_EditorCamera = camera;
    }

	void ImGuizmoLayer::OnImGuiRender() 
	{
        Entity entity = m_EditorCtx->SelectedEntity;

        Entity cameraEntity = m_EditorCtx->ActiveScene->GetPrimaryCameraEntity();
        bool isRuntime = m_EditorCtx->SceneState == SceneState::Play;
        SceneCamera* runtimeCamera = isRuntime && cameraEntity ? &cameraEntity.GetComponent<CameraComponent>().Camera : nullptr;

        bool hasCamera = isRuntime ? cameraEntity != Entity{} : m_EditorCamera != nullptr;

        if (hasCamera && m_ViewportPanel && entity && m_GuizmoOperation != ImGuizmo::OPERATION::BOUNDS && entity.HasComponent<TransformComponent>())
        {
            bool isOrthographic = false;

            if (entity.HasComponent<TextComponent>())
            {
                if(entity.GetComponent<TextComponent>().Space == Renderer2DSpace::ScreenSpace)
                    isOrthographic = true;
            }
            else if (entity.HasComponent<SpriteComponent>())
            {
                if (entity.GetComponent<SpriteComponent>().Space == Renderer2DSpace::ScreenSpace)
                    isOrthographic = true;
            }
            else if (entity.HasComponent<CircleComponent>())
            {
                if (entity.GetComponent<CircleComponent>().Space == Renderer2DSpace::ScreenSpace)
                    isOrthographic = true;
            }

            auto& desc = m_ViewportPanel->GetDescription();

            ImGuizmo::SetOrthographic(isOrthographic);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(desc.Bounds[0].x, desc.Bounds[0].y,
                desc.Bounds[1].x - desc.Bounds[0].x, desc.Bounds[1].y - desc.Bounds[0].y);

            Matrix4 cameraProjection;
            Matrix4 cameraView;

            if (isOrthographic)
            {
                // Must match with SceneRenderer2D
                const float size = 10.f;
                cameraProjection = Math::Ortho(0.f, 16.f / 9.f * size, size, 0.f, 0.f, size); // reverse Y
                cameraView = Matrix4::Identity();
            }
            else
            {
                // Need to recreate projection matrix, because ImGuizmo 
                // does not work properly with reversed Z projection

                CameraInfo info = isRuntime ? runtimeCamera->GetCameraInfo() : m_EditorCamera->GetCameraInfo();
                float fov = info.FOV;
                float znear = info.NearClip;
                float zfar = info.FarClip;
                float aspectRatio = isRuntime ? runtimeCamera->GetAspectRatio() : m_EditorCamera->GetAspectRatio();

                cameraProjection = Math::Perspective(fov, aspectRatio, znear, zfar);
                cameraProjection[1][1] = -cameraProjection[1][1]; // invert y

                cameraView = isRuntime ?
                    Math::AffineInverse(cameraEntity.GetComponent<WorldTransformComponent>().AsMatrix()) :
                    m_EditorCamera->GetViewMatrix();
            }

            const WorldTransformComponent& worldTransform = m_EditorCtx->SelectedEntity.GetComponent<WorldTransformComponent>();
            Matrix4 worldTransformMatrix = worldTransform.AsMatrix();

            // Discard if not in viewport space
            Vector4 viewPos = Vector4(0, 0, 0, 1) * worldTransformMatrix * cameraView;
            if (!isOrthographic && viewPos.z > 0)
                return;

            bool snap = Input::IsKeyPressed(Keyboard::LCtrl);
            float snapValue = 0.5f;
            if (m_GuizmoOperation == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.f;

            Vector3 snapValues = Vector3(snapValue);
            ImGuizmo::MODE mode = m_EditorCtx->EditorSettings.GizmosLocalTransform ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;

            ImGuizmo::Manipulate(cameraView.Data(), cameraProjection.Data(),
                m_GuizmoOperation, mode, worldTransformMatrix.Data(),
                nullptr, snap ? snapValues.Data() : nullptr);

            if (ImGuizmo::IsUsing())
            {
                Vector3 translation, rotation, scale;
                Math::DecomposeTransform(worldTransformMatrix, translation, rotation, scale);

                WorldTransformComponent newWorldTransform;
                newWorldTransform.Translation = translation;
                newWorldTransform.Rotation = rotation;
                newWorldTransform.Scale = scale;

               m_EditorCtx->SelectedEntity.GetComponent<TransformComponent>().UpdateLocalTransform(newWorldTransform, worldTransform);
            }
        }
	}

    void ImGuizmoLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(ImGuizmoLayer::OnKeyPressedEvent));
    }

    bool ImGuizmoLayer::OnKeyPressedEvent(KeyPressedEvent& event)
    {
        if (m_ViewportPanel && !Input::IsMouseButtonPressed(Mouse::Right))
        {
            auto& desc = m_ViewportPanel->GetDescription();
            switch (event.GetKeyCode())
            {
            case Keyboard::Q: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::BOUNDS); break;
            case Keyboard::W: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE); break;
            case Keyboard::E: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::ROTATE); break;
            case Keyboard::R: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::SCALE); break;
            }
        }

        return false;
    }
}
