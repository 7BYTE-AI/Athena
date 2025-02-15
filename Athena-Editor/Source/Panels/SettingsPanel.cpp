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
	static std::string_view TonemapModeToString(TonemapMode mode)
	{
		switch (mode)
		{
		case TonemapMode::NONE: return "None";
		case TonemapMode::ACES_FILMIC: return "ACES-Filmic";
		case TonemapMode::ACES_TRUE: return "ACES-True";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}

	static TonemapMode TonemapModeFromString(std::string_view str)
	{
		if (str == "None")
			return TonemapMode::NONE;
		if (str == "ACES-Filmic")
			return TonemapMode::ACES_FILMIC;
		if (str == "ACES-True")
			return TonemapMode::ACES_TRUE;

		ATN_CORE_ASSERT(false);
		return TonemapMode::NONE;
	}

	static std::string_view AntialisingToString(Antialising antialiasing)
	{
		switch (antialiasing)
		{
		case Antialising::NONE: return "None";
		case Antialising::FXAA: return "FXAA";
		case Antialising::SMAA: return "SMAA";
		}

		ATN_ASSERT(false);
		return "";
	}

	static Antialising AntialisingFromString(std::string_view str)
	{
		if(str == "None")
			return Antialising::NONE;
		else if (str == "FXAA")
			return Antialising::FXAA;
		else if (str == "SMAA")
			return Antialising::SMAA;

		ATN_ASSERT(false);
		return (Antialising)0;
	}

	static std::string_view DebugViewToString(DebugView view)
	{
		switch (view)
		{
		case DebugView::NONE: return "None";
		case DebugView::SHADOW_CASCADES: return "ShadowCascades";
		case DebugView::LIGHT_COMPLEXITY: return "LightComplexity";
		case DebugView::GBUFFER: return "GBuffer";
		}

		ATN_ASSERT(false);
		return "";
	}

	static DebugView DebugViewFromString(std::string_view str)
	{
		if (str == "None")
			return DebugView::NONE;

		if (str == "ShadowCascades")
			return DebugView::SHADOW_CASCADES;

		if (str == "LightComplexity")
			return DebugView::LIGHT_COMPLEXITY;

		if (str == "GBuffer")
			return DebugView::GBUFFER;

		ATN_ASSERT(false);
		return (DebugView)0;
	}

	SettingsPanel::SettingsPanel(std::string_view name, const Ref<EditorContext>& context)
		: Panel(name, context)
	{

	}

	void SettingsPanel::OnImGuiRender()
	{
		if (ImGui::Begin("Editor Settings"))
		{
			if (UI::BeginPropertyTable())
			{
				EditorSettings& settings = m_EditorCtx.EditorSettings;

				UI::PropertyCheckbox("GizmosLocal", &settings.GizmosLocalTransform);
				UI::PropertyCheckbox("ShowRendererIcons", &settings.ShowRendererIcons);
				UI::PropertySlider("RendererIconsScale", &settings.RendererIconsScale, 0.4f, 3.f);
				UI::PropertySlider("CameraSpeed", &settings.CameraSpeedLevel, 0.f, 10.f);
				UI::PropertyDrag("Camera Near/Far", &settings.NearFarClips);
				UI::PropertyCheckbox("ShowPhysicsColliders", &settings.ShowPhysicsColliders);
				UI::PropertyCheckbox("ReloadScriptsOnStart", &settings.ReloadScriptsOnStart);

				UI::PropertyRow("Reload Scripts", ImGui::GetFrameHeight());
				ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
				if (ImGui::Button("Reload All Scripts"))
				{
					m_EditorCtx.ActiveScene->LoadAllScripts();
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();

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

		if (UI::TreeNode("Debug", false))
		{
			ImGui::Text("DebugView");
			ImGui::SameLine();

			std::string_view views[] = { "None", "ShadowCascades", "LightComplexity", "GBuffer" };
			std::string_view selected = DebugViewToString(settings.DebugView);
			if (UI::ComboBox("##DebugView", views, std::size(views), &selected))
			{
				settings.DebugView = DebugViewFromString(selected);
			}

			ImGui::Spacing();
			ImGui::Spacing();

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
				UI::PropertyDrag("NearPlaneOffset", &shadowSettings.NearPlaneOffset);
				UI::PropertyDrag("FarPlaneOffset", &shadowSettings.FarPlaneOffset);

				UI::EndPropertyTable();
				UI::TreePop();
			}

			if (UI::TreeNode("ShadowMap", false, true))
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
			UI::PropertySlider("BlurSharpness", &ao.BlurSharpness, 0.f, 100.f);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		if (UI::TreeNode("SSR", false) && UI::BeginPropertyTable())
		{
			SSRSettings& ssr = settings.SSRSettings;

			UI::PropertyCheckbox("Enable", &ssr.Enable);
			if (UI::PropertyCheckbox("HalfRes", &ssr.HalfRes))
				m_ViewportRenderer->ApplySettings();

			UI::PropertyCheckbox("ConeTrace", &ssr.ConeTrace);
			UI::PropertySlider("Intensity", &ssr.Intensity, 0.f, 1.f);
			UI::PropertySlider("MaxRoughness", &ssr.MaxRoughness, 0.f, 1.f);

			int maxSteps = ssr.MaxSteps;
			if (UI::PropertyDrag("MaxSteps", &maxSteps, 1, 1024))
				ssr.MaxSteps = maxSteps;

			UI::PropertySlider("ScreenEdgesFade", &ssr.ScreenEdgesFade, 0.f, 0.4f);
			UI::PropertyCheckbox("BackwardRays", &ssr.BackwardRays);

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
			UI::PropertyDrag("DirtIntensity", &bloomSettings.DirtIntensity, 0.1f, 0, 200);

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

			if (UI::TreeNode("BloomTexture", false, true))
			{
				Ref<Texture2D> bloomTexture = m_ViewportRenderer->GetBloomTexture();

				static int mip = 0;
				ImGui::SliderInt("Mip", &mip, 0, bloomTexture->GetMipLevelsCount() - 4);
				ImGui::Image(UI::GetTextureID(bloomTexture->GetMipView(mip)), {256, 256});

				UI::TreePop();
			}

			UI::TreePop();
		}

		if (UI::TreeNode("PostProcessing", false) && UI::BeginPropertyTable())
		{
			PostProcessingSettings& postProcess = settings.PostProcessingSettings;

			{
				std::string_view views[] = { "None", "ACES-Filmic", "ACES-True" };
				std::string_view selected = TonemapModeToString(postProcess.TonemapMode);

				if (UI::PropertyCombo("Tonemap Mode", views, std::size(views), &selected))
					postProcess.TonemapMode = TonemapModeFromString(selected);

				UI::PropertySlider("Exposure", &postProcess.Exposure, 0.f, 10.f);
			}

			{
				std::string_view views[] = { "None", "FXAA", "SMAA"};
				std::string_view selected = AntialisingToString(postProcess.AntialisingMethod);

				if (UI::PropertyCombo("Antialiasing", views, std::size(views), &selected))
					postProcess.AntialisingMethod = AntialisingFromString(selected);
			}

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
