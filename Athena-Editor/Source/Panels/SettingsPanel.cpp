#include "SettingsPanel.h"

#include "Athena/Asset/TextureImporter.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/Scripting/ScriptEngine.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"
#include "EditorResources.h"
#include "EditorLayer.h"

#include <ImGui/imgui.h>


namespace Athena
{
	SettingsPanel::SettingsPanel(std::string_view name, const Ref<EditorContext>& context)
		: Panel(name, context)
	{
		UI::RegisterEnum("TonemapMode");
		UI::EnumAdd("TonemapMode", 0, "None");
		UI::EnumAdd("TonemapMode", 1, "ACES-Filmic");
		UI::EnumAdd("TonemapMode", 2, "ACES-True");

		UI::RegisterEnum("Antialising");
		UI::EnumAdd("Antialising", 0, "None");
		UI::EnumAdd("Antialising", 1, "FXAA");
		UI::EnumAdd("Antialising", 2, "SMAA");

		UI::RegisterEnum("DebugView");
		UI::EnumAdd("DebugView", 0, "None");
		UI::EnumAdd("DebugView", 1, "Shadow Cascades");
		UI::EnumAdd("DebugView", 2, "Light Complexity");
		UI::EnumAdd("DebugView", 3, "GBuffer");
	}

	void SettingsPanel::OnImGuiRender()
	{
		if (ImGui::Begin("Editor Settings"))
		{
			if (UI::BeginPropertyTable())
			{
				EditorSettings& settings = m_EditorCtx.EditorSettings;

				UI::PropertyCheckbox("Gizmos Local", &settings.GizmosLocalTransform);
				UI::PropertyCheckbox("Show Renderer Icons", &settings.ShowRendererIcons);
				UI::PropertySlider("Renderer Icons Scale", &settings.RendererIconsScale, 0.4f, 3.f);
				UI::PropertySlider("Camera Speed", &settings.CameraSpeedLevel, 0.f, 10.f);
				UI::PropertyDrag("Camera Near/Far", &settings.NearFarClips);
				UI::PropertyCheckbox("Show Physics Colliders", &settings.ShowPhysicsColliders);

				UI::EndPropertyTable();
			}
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0.f });
		ImGui::Begin("SceneRenderer");
		ImGui::PopStyleVar();

		SceneRendererSettings& settings = m_ViewportRenderer->GetSettings();
		ShaderPack& shaderPack = *Renderer::GetShaderPack();

		bool shaderPackOpen = UI::TreeNode("ShaderPack", false);
		if (!shaderPackOpen)
		{
			float frameHeight = ImGui::GetFrameHeight();
			ImVec2 regionAvail = ImGui::GetContentRegionAvail();
			ImVec2 textSize = ImGui::CalcTextSize("Reload All");

			ImGui::SameLine(regionAvail.x - frameHeight);
			UI::ShiftCursor(-textSize.x, 0.f);
			if (ImGui::Button("Reload All"))
				shaderPack.Reload();

			UI::ShiftCursorY(-3.f);
		}

		if (shaderPackOpen)
		{
			if (UI::BeginPropertyTable())
			{
				for (const auto& [name, shader] : shaderPack)
				{
					bool isCompiled = shader->IsCompiled();

					if (!isCompiled)
						ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);

					UI::PropertyRow(name, ImGui::GetFrameHeight());

					if (!isCompiled)
						ImGui::PopStyleColor();

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.f, 3.f});
					ImGui::PushID(name.c_str());

					if (UI::ButtonCentered("Reload"))
						shader->Reload();

					ImGui::PopID();
					ImGui::PopStyleVar();
				}

				UI::EndPropertyTable();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Debug", false) && UI::BeginPropertyTable())
		{
			UI::PropertyEnumCombo("Debug View", ATN_STRINGIFY_MACRO(DebugView), (void*)&settings.DebugView);
			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Shadows", false) && UI::BeginPropertyTable())
		{
			ShadowSettings& shadowSettings = settings.ShadowSettings;

			UI::PropertyCheckbox("Soft Shadows", &shadowSettings.SoftShadows);
			UI::PropertyDrag("Max Distance", &shadowSettings.MaxDistance);
			UI::PropertyDrag("Fade Out", &shadowSettings.FadeOut);
			UI::PropertyDrag("Bias Gradient", &shadowSettings.BiasGradient, 0.01f);
				
			UI::EndPropertyTable();

			if (UI::TreeNode("Cascade Settings", true, true) && UI::BeginPropertyTable())
			{
				UI::PropertySlider("Blend Distance", &shadowSettings.CascadeBlendDistance, 0.f, 1.f);
				UI::PropertySlider("Split", &shadowSettings.CascadeSplit, 0.f, 1.f);
				UI::PropertyDrag("Near Plane Offset", &shadowSettings.NearPlaneOffset);
				UI::PropertyDrag("Far Plane Offset", &shadowSettings.FarPlaneOffset);

				UI::EndPropertyTable();
				UI::TreePop();
			}

			if (UI::TreeNode("Shadow Map", false, true))
			{
				static int layer = 0;
				ImGui::SliderInt("Layer", &layer, 0, ShaderDef::SHADOW_CASCADES_COUNT - 1);

				Ref<Texture2D> shadowMap = m_ViewportRenderer->GetShadowMap();
				TextureViewCreateInfo view;
				view.BaseLayer = layer;
				view.GrayScale = true;

				ImGui::Image(UI::GetTextureID(shadowMap->GetView(view)), {256, 256});

				UI::TreePop();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Ambient Occlusion", false) && UI::BeginPropertyTable())
		{
			AmbientOcclusionSettings& ao = settings.AOSettings;

			UI::PropertyCheckbox("Enable", &ao.Enable);
			UI::PropertySlider("Intensity", &ao.Intensity, 0.1f, 5.f);
			UI::PropertySlider("Radius", &ao.Radius, 0.1f, 3.f);
			UI::PropertySlider("Bias", &ao.Bias, 0.f, 0.5f);
			UI::PropertySlider("Blur Sharpness", &ao.BlurSharpness, 0.f, 100.f);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("SSR", false) && UI::BeginPropertyTable())
		{
			SSRSettings& ssr = settings.SSRSettings;

			UI::PropertyCheckbox("Enable", &ssr.Enable);
			if (UI::PropertyCheckbox("HalfRes", &ssr.HalfRes))
				m_ViewportRenderer->ApplySettings();

			UI::PropertyCheckbox("Cone Trace", &ssr.ConeTrace);
			UI::PropertySlider("Intensity", &ssr.Intensity, 0.f, 1.f);
			UI::PropertySlider("Max Roughness", &ssr.MaxRoughness, 0.f, 1.f);

			int maxSteps = ssr.MaxSteps;
			if (UI::PropertyDrag("Max Steps", &maxSteps, 1, 1024))
				ssr.MaxSteps = maxSteps;

			UI::PropertySlider("Screen Edges Fade", &ssr.ScreenEdgesFade, 0.f, 0.4f);
			UI::PropertyCheckbox("Backward Rays", &ssr.BackwardRays);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Bloom", false) && UI::BeginPropertyTable())
		{
			BloomSettings& bloomSettings = settings.BloomSettings;

			UI::PropertyCheckbox("Enable", &bloomSettings.Enable);
			UI::PropertyDrag("Intensity", &bloomSettings.Intensity, 0.05f, 0, 10);
			UI::PropertyDrag("Threshold", &bloomSettings.Threshold, 0.05f, 0, 10);
			UI::PropertyDrag("Knee", &bloomSettings.Knee, 0.05f, 0, 10);
			UI::PropertyDrag("Dirt Intensity", &bloomSettings.DirtIntensity, 0.1f, 0, 200);

			Ref<Texture2D> displayTex = bloomSettings.DirtTexture;
			if (!displayTex || displayTex == TextureGenerator::GetBlackTexture())
				displayTex = EditorResources::GetIcon("EmptyTexture");

			if (UI::PropertyImage("Dirt Texture", displayTex, { 45.f, 45.f }))
			{
				FilePath path = FileDialogs::OpenFile(TEXT("Texture\0*.png;*.jpg\0"));
				if (!path.empty())
				{
					TextureImportOptions options;
					options.sRGB = false;
					options.GenerateMipMaps = false;

					bloomSettings.DirtTexture = TextureImporter::Load(path, options);
				}
			}

			UI::EndPropertyTable();

			if (UI::TreeNode("Bloom Texture", false, true))
			{
				Ref<Texture2D> bloomTexture = m_ViewportRenderer->GetBloomTexture();

				static int mip = 0;
				ImGui::SliderInt("Mip", &mip, 0, bloomTexture->GetMipLevelsCount() - 4);
				ImGui::Image(UI::GetTextureID(bloomTexture->GetMipView(mip)), {256, 256});

				UI::TreePop();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("Post Processing", false) && UI::BeginPropertyTable())
		{
			PostProcessingSettings& postProcess = settings.PostProcessingSettings;

			UI::PropertyEnumCombo("Tonemap Mode", ATN_STRINGIFY_MACRO(TonemapMode), (void*)&postProcess.TonemapMode);
			UI::PropertySlider("Exposure", &postProcess.Exposure, 0.f, 10.f);
			UI::PropertyEnumCombo("Antialiasing", ATN_STRINGIFY_MACRO(Antialiasing), (void*)&postProcess.AntialisingMethod);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("Quality", false) && UI::BeginPropertyTable())
		{
			QualitySettings& quality = settings.Quality;
			UI::PropertySlider("Renderer Scale", &quality.RendererScale, 0.5f, 4.f);

			UI::EndPropertyTable();

			if (UI::ButtonCentered("Apply"))
				m_ViewportRenderer->ApplySettings();

			UI::TreePop();
		}

		ImGui::End();
	}
}
