#include "ViewportPanel.h"

#include "Athena/UI/UI.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "ImGuizmoLayer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ViewportPanel::ViewportPanel(std::string_view name, const Ref<EditorContext>& context)
        : Panel(name, context)
    {

    }

	void ViewportPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::Begin("Viewport");

        ImVec2 cursor = ImGui::GetCursorPos();

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_Description.Position = { viewportOffset.x, viewportOffset.y };
        m_Description.Bounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_Description.Bounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_Description.IsFocused = ImGui::IsWindowFocused();
        m_Description.IsHovered = ImGui::IsWindowHovered();

        const auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_Description.Size = { viewportX, viewportY };

        if (m_ViewportRenderer->GetSettings().DebugView != DebugView::GBUFFER)
        {
            Ref<Texture2D> image = m_ViewportRenderer->GetFinalImage();
            ImGui::Image(UI::GetTextureID(image), ImVec2((float)m_Description.Size.x, (float)m_Description.Size.y));
        }
        else
        {
            Ref<RenderPass> gbuffer = m_ViewportRenderer->GetGBufferPass();
            Ref<RenderPass> aoPass = m_ViewportRenderer->GetAOPass();
            const uint32 rows = 2;
            const uint32 columns = 3;
            const uint32 texNum = rows * columns;

            TextureViewCreateInfo view;
            view.EnableAlphaBlending = false;
            
            TextureViewCreateInfo grayScaleView;
            grayScaleView.GrayScale = true;

            std::array<Ref<TextureView>, texNum> textures;
            textures[0] = gbuffer->GetOutput("SceneDepth")->GetView(grayScaleView);
            textures[1] = gbuffer->GetOutput("SceneNormalsEmission")->GetView(view);
            textures[2] = gbuffer->GetOutput("SceneRoughnessMetalness")->GetView(view);
            textures[3] = aoPass->GetOutput("SceneAO")->GetView(grayScaleView);
            textures[4] = gbuffer->GetOutput("SceneAlbedo")->GetView(view);
            textures[5] = m_ViewportRenderer->GetFinalImage()->GetView(view);

            ImVec2 textureSize = { viewportX / columns, viewportY / rows };
            ImVec2 pos = cursor;
            for (uint32 x = 0; x < columns; ++x)
            {
                for (uint32 y = 0; y < rows; ++y)
                {
                    uint32 index = x + y * columns;

                    ImVec2 imagePos = pos;
                    imagePos.x += x * textureSize.x;
                    imagePos.y += y * textureSize.y;

                    ImGui::SetCursorPos(imagePos);
                    ImGui::Image(UI::GetTextureID(textures[index]), textureSize);
                }
            }
        }

        if (ImGui::BeginDragDropTarget())
        {
            m_DragDropCallback();
            ImGui::EndDragDropTarget();
        }

        if (m_UIOverlayCallback)
        {
            ImGui::SetCursorPos(cursor);
            m_UIOverlayCallback();
        }

        if (m_ImGuizmoLayer)
        {
            m_ImGuizmoLayer->OnImGuiRender();
        }

        ImGui::End();
        ImGui::PopStyleVar();
	}

    void ViewportPanel::SetImGuizmoLayer(const Ref<ImGuizmoLayer>& layer)
    {
        m_ImGuizmoLayer = layer;
        m_ImGuizmoLayer->m_ViewportPanel = this;
    }
}
