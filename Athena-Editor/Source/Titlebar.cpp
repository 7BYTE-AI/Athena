#include "Titlebar.h"

#include "Athena/Core/Application.h"
#include "Athena/Project/Project.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    Titlebar::Titlebar(const Ref<EditorContext>& editorCtx)
    {
        m_EditorCtx = editorCtx;
    }

	void Titlebar::OnImGuiRender()
	{
        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
        const ImVec2 windowPadding = isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f); // ImGui::GetCurrentWindow()->WindowPadding

        static float moveOffsetX;
        static float moveOffsetY;
        const float w = ImGui::GetContentRegionAvail().x;
        const float buttonsAreaWidth = 94;

        // Title bar drag area
        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
        const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
        const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
                                     ImGui::GetCursorScreenPos().y + m_Height };
        auto* bgDrawList = ImGui::GetBackgroundDrawList();
        auto* fgDrawList = ImGui::GetForegroundDrawList();
        bgDrawList->AddRectFilled(titlebarMin, titlebarMax, UI::GetTheme().Titlebar);

        // Logo
        {
            const int logoWidth = 48;
            const int logoHeight = 48;
            ImVec2 logoOffset(10.0f + windowPadding.x, 5.0f + windowPadding.y + titlebarVerticalOffset);
            if (isMaximized)
                logoOffset.y += 3.f;
            const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
            const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };
            fgDrawList->AddImage(UI::GetTextureID(EditorResources::GetIcon("Logo")), logoRectStart, logoRectMax);
        }

        ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing() });

        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos
        // DEBUG DRAG BOUNDS
        //fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth, ImGui::GetCursorScreenPos().y + m_Height), IM_COL32(222, 43, 43, 255));

        UI::InvisibleItem("##TitlebarDragZone", ImVec2(w - buttonsAreaWidth, m_Height));
        m_Hovered = ImGui::IsItemHovered();

        if (isMaximized)
        {
            float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
            if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
                m_Hovered = true; // Account for the top-most pixels which don't register
        }

        float menubarOffsetX = 0.f;
        // Draw Menubar
        if (m_MenubarCallback)
        {
            ImGui::SuspendLayout();
            {
                ImGui::SetItemAllowOverlap();
                const float logoHorizontalOffset = 14.0f * 2.0f + 48.0f + windowPadding.x;
                ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 4.0f ));

                const ImRect menuBarRect = { ImGui::GetCursorPos(), { ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing() } };

                ImGui::BeginGroup();
                if (UI::BeginMenubar(menuBarRect))
                {
                    m_MenubarCallback();
                }

                menubarOffsetX = ImGui::GetCursorPos().x;
                UI::EndMenubar();
                ImGui::EndGroup();

                if (ImGui::IsItemHovered())
                    m_Hovered = false;
            }

            ImGui::ResumeLayout();
        }

        // Scene name
        {
            std::string_view sceneName = m_EditorCtx->ActiveScene->GetSceneName();
            ImGui::SetCursorPos({ menubarOffsetX + 75.f, isMaximized ? -titlebarVerticalOffset : 0.f });

            float rectOffsetY = -5.f; // Do not draw upper rounded corners
            UI::ShiftCursorY(rectOffsetY);

            ImVec2 framePadding = { 15.f, 5.f };
            ImVec2 textSize = ImGui::CalcTextSize(sceneName.data());
            ImVec2 itemSize = { textSize.x + framePadding.x * 2.f, textSize.y + framePadding.y * 2.f - rectOffsetY };

            ImDrawList* list = ImGui::GetWindowDrawList();
            list->AddRect(ImGui::GetCursorScreenPos(), { ImGui::GetCursorScreenPos().x + itemSize.x, ImGui::GetCursorScreenPos().y + itemSize.y }, IM_COL32(61, 61, 61, 255), 5.f);

            UI::ShiftCursor(framePadding.x, framePadding.y - rectOffsetY);
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32( 200, 200, 200, 255 ));
            ImGui::Text(sceneName.data());
            ImGui::PopStyleColor();
        }
        
        // Centered Window title
        {
            String name = Project::GetActive()->GetConfig().Name;

            ImVec2 currentCursorPos = ImGui::GetCursorPos();
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f, 2.0f + windowPadding.y + 2.0f));

            UI::PushFont(UI::Fonts::Default22);
            ImGui::Text("%s", name.c_str());
            UI::PopFont();

            ImGui::SetCursorPos(currentCursorPos);
        }


        // Window buttons
        const ImU32 buttonColN = UI::MultiplyColorByScalar(UI::GetTheme().Text, 0.8f);
        const ImU32 buttonColH = UI::MultiplyColorByScalar(UI::GetTheme().Text, 1.2f);
        const ImU32 buttonColP = IM_COL32(128, 128, 128, 255);
        const float buttonWidth = 14.0f;
        const float buttonHeight = 14.0f;
        const float paddingY = isMaximized ? 16.f : 8.f;

        auto& window = Application::Get().GetWindow();

        // Minimize Button
        ImGui::Spring();
        UI::ShiftCursorY(paddingY);
        {
            if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
            {
                window.SetWindowMode(WindowMode::Minimized);
            }

            UI::ButtonImage(EditorResources::GetIcon("Titlebar_MinimizeWindow"), buttonColN, buttonColH, buttonColP);
        }


        // Maximize Button
        ImGui::Spring(-1.0f, 17.0f);
        UI::ShiftCursorY(paddingY);
        {
            if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
            {
                if (isMaximized)
                    window.SetWindowMode(WindowMode::Default);
                else
                    window.SetWindowMode(WindowMode::Maximized);
            }

            UI::ButtonImage(isMaximized ? EditorResources::GetIcon("Titlebar_RestoreWindow") : EditorResources::GetIcon("Titlebar_MaximizeWindow"), buttonColN, buttonColH, buttonColP);
        }

        // Close Button
        ImGui::Spring(-1.0f, 15.0f);
        UI::ShiftCursorY(paddingY);
        {
            if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
            {
                Application::Get().Close();
            }

            UI::ButtonImage(EditorResources::GetIcon("Titlebar_CloseWindow"), buttonColN, buttonColH, buttonColP);
        }

        ImGui::Spring(-1.0f, 18.0f);

        ImGui::EndHorizontal();
	}
}
