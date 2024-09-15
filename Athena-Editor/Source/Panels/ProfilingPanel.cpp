#include "ProfilingPanel.h"

#include "Athena/Core/Application.h"
#include "Athena/UI/UI.h"
#include "Athena/Utils/StringUtils.h"
#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ProfilingPanel::ProfilingPanel(const Ref<EditorContext>& context)
        : Panel(PROFILING_PANEL_ID, context)
    {

    }

	void ProfilingPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 3 });
        if (ImGui::Begin("Profiling"))
        {
            bool vsync = Application::Get().GetWindow().IsVSync();
            bool newVsync = vsync;
            ImGui::Checkbox("VSync", &newVsync);
            if (newVsync != vsync)
                Application::Get().GetWindow().SetVSync(newVsync);

            const auto& appstats = Application::Get().GetStats();

            ImGui::Text("FPS: %d", (int)(1.f / appstats.FrameTime.AsSeconds()));
            ImGui::Text("FrameTime: %.3f ms", appstats.FrameTime.AsMilliseconds());
            
            Vector2u size;
            size.x = Application::Get().GetWindow().GetWidth();
            size.y = Application::Get().GetWindow().GetHeight();
            ImGui::Text("WindowSize: { %u, %u }", size.x, size.y);

            ImGui::Spacing();

            ImGuiTabBarFlags tabBarFlags = 0;
            if (ImGui::BeginTabBar("ProfilingTabBar", tabBarFlags))
            {
                if (ImGui::BeginTabItem("Application"))
                {
                    ImGui::Text("RAM: %s", Utils::MemoryBytesToString(Platform::GetMemoryUsage()).data());
                    ImGui::Text("VRAM: %s", Utils::MemoryBytesToString(Renderer::GetMemoryUsage()).data());
                    ImGui::Spacing();

                    ImGui::Text("CPUWait: %.3f ms", appstats.CPUWait.AsMilliseconds());
                    ImGui::Text("GPUWait: %.3f ms", appstats.GPUWait.AsMilliseconds());
                    ImGui::Text("Application::ProcessEvents: %.3f ms", appstats.Application_ProcessEvents.AsMilliseconds());
                    ImGui::Text("Application::OnUpdate: %.3f ms", appstats.Application_OnUpdate.AsMilliseconds());
                    ImGui::Text("Application::RenderImGui: %.3f ms", appstats.Application_RenderImGui.AsMilliseconds());
                    ImGui::Spacing();

                    ImGui::Text("SwapChain::Present: %.3f ms", appstats.SwapChain_Present.AsMilliseconds());
                    ImGui::Text("SwapChain::AcquireImage: %.3f ms", appstats.SwapChain_AcquireImage.AsMilliseconds());
                    ImGui::Text("Renderer::QueueSubmit: %.3f ms", appstats.Renderer_QueueSubmit.AsMilliseconds());

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SceneRenderer"))
                {
                    if (m_SceneRenderer)
                    {
                        auto& settings = m_SceneRenderer->GetSettings();
                        Antialising antialising = m_SceneRenderer->GetAntialising();
                        size = m_SceneRenderer->GetViewportSize();

                        ImGui::Text("ViewportSize: { %u, %u }", size.x, size.y);

                        auto& stats = m_SceneRenderer->GetStatistics();
                        ImGui::Text("GPUTime: %.3f ms", stats.GPUTime.AsMilliseconds());
                        ImGui::Text("DirShadowMap: %.3f ms", stats.DirShadowMapPass.AsMilliseconds());
                        ImGui::Text("GBuffer: %.3f ms", stats.GBufferPass.AsMilliseconds());
                        ImGui::Text("HiZ: %.3f ms", stats.HiZPass.AsMilliseconds());
                        ImGui::Text("LightCulling: %.3f ms", stats.LightCullingPass.AsMilliseconds());

                        if (settings.AOSettings.Enable)
                        {
                            ImGui::Text("HBAO-Deinterleave: %.3f ms", stats.HBAODeinterleavePass.AsMilliseconds());
                            ImGui::Text("HBAO-Compute: %.3f ms", stats.HBAOComputePass.AsMilliseconds());
                            ImGui::Text("HBAO-Blur: %.3f ms", stats.HBAOBlurPass.AsMilliseconds());
                        }

                        ImGui::Text("DeferredLighting: %.3f ms", stats.DeferredLightingPass.AsMilliseconds());
                        ImGui::Text("Skybox: %.3f ms", stats.SkyboxPass.AsMilliseconds());

                        if (settings.SSRSettings.Enable)
                        {
                            ImGui::Text("Pre-Convolution: %.3f ms", stats.PreConvolutionPass.AsMilliseconds());
                            ImGui::Text("SSR-Compute: %.3f ms", stats.SSRComputePass.AsMilliseconds());
                            ImGui::Text("SSR-Composite: %.3f ms", stats.SSRCompositePass.AsMilliseconds());
                        }

                        if (settings.BloomSettings.Enable)
                        {
                            ImGui::Text("Bloom: %.3f ms", stats.BloomPass.AsMilliseconds());
                        }

                        ImGui::Text("SceneComposite: %.3f ms", stats.SceneCompositePass.AsMilliseconds());
                        ImGui::Text("JumpFlood: %.3f ms", stats.JumpFloodPass.AsMilliseconds());
                        ImGui::Text("Render2D: %.3f ms", stats.Render2DPass.AsMilliseconds());

                        if (antialising == Antialising::FXAA)
                            ImGui::Text("FXAA: %.3f ms", stats.AAPass.AsMilliseconds());
                        else if (antialising == Antialising::SMAA)
                            ImGui::Text("SMAA: %.3f ms", stats.AAPass.AsMilliseconds());

                        if (UI::TreeNode("Pipeline Statistics", false))
                        {
                            ImGui::Text("InputAssemblyVertices: %lld", stats.PipelineStats.InputAssemblyVertices);
                            ImGui::Text("InputAssemblyPrimitives: %lld", stats.PipelineStats.InputAssemblyPrimitives);
                            ImGui::Text("VertexShaderInvocations: %lld", stats.PipelineStats.VertexShaderInvocations);
                            ImGui::Text("GeometryShaderInvocations: %lld", stats.PipelineStats.GeometryShaderInvocations);
                            ImGui::Text("GeometryShaderPrimitives: %lld", stats.PipelineStats.GeometryShaderPrimitives);
                            ImGui::Text("ClippingInvocations: %lld", stats.PipelineStats.ClippingInvocations);
                            ImGui::Text("ClippingPrimitives: %lld", stats.PipelineStats.ClippingPrimitives);
                            ImGui::Text("FragmentShaderInvocations: %lld", stats.PipelineStats.FragmentShaderInvocations);
                            ImGui::Text("ComputeShaderInvocations: %lld", stats.PipelineStats.ComputeShaderInvocations);

                            UI::TreePop();
                        }

                        if (UI::TreeNode("Draw Statistics", false))
                        {
                            ImGui::Text("Meshes: %u", stats.Meshes);
                            ImGui::Text("Instances: %u", stats.Instances);
                            ImGui::Text("Draws saved(by instancing): %u", stats.Meshes - stats.Instances);
                            ImGui::Spacing();
                            ImGui::Text("AnimMeshes: %u", stats.AnimMeshes);

                            UI::TreePop();
                        }
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System Info"))
                {
                    if (UI::TreeNode("CPU Capabilites"))
                    {
                        auto& cpuCaps = Platform::GetCPUCapabilities();

                        ImGui::Text(cpuCaps.Name.c_str());
                        ImGui::Text("RAM: %d Mb", cpuCaps.RAM / 1024);
                        ImGui::Text("Cores: %d", cpuCaps.Cores);
                        ImGui::Text("Logical Processors: %d", cpuCaps.LogicalProcessors);

                        UI::TreePop();
                    }

                    if (UI::TreeNode("GPU Capabilites"))
                    {
                        auto& gpuCaps = Renderer::GetRenderCaps();

                        ImGui::Text(gpuCaps.Name.c_str());
                        ImGui::Text("VRAM: %d Mb", gpuCaps.VRAM / 1024);

                        ImGui::Text("MaxImageDimension2D: %u", gpuCaps.MaxImageDimension2D);
                        ImGui::Text("MaxImageDimensionCube: %u", gpuCaps.MaxImageDimensionCube);
                        ImGui::Text("MaxImageArrayLayers: %u", gpuCaps.MaxImageArrayLayers);
                        ImGui::Spacing();
                        ImGui::Text("MaxSamplerLodBias: %.1f", gpuCaps.MaxSamplerLodBias);
                        ImGui::Text("MaxSamplerAnisotropy: %.1f", gpuCaps.MaxSamplerAnisotropy);
                        ImGui::Spacing();
                        ImGui::Text("MaxFramebufferWidth: %u", gpuCaps.MaxFramebufferWidth);
                        ImGui::Text("MaxFramebufferHeight: %u", gpuCaps.MaxFramebufferHeight);
                        ImGui::Text("MaxFramebufferLayers: %u", gpuCaps.MaxFramebufferLayers);
                        ImGui::Text("MaxFramebufferColorAttachments: %u", gpuCaps.MaxFramebufferColorAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxUniformBufferRange: %u", gpuCaps.MaxUniformBufferRange);
                        ImGui::Text("MaxStorageBufferRange: %u", gpuCaps.MaxStorageBufferRange);
                        ImGui::Text("MaxPushConstantRange: %u", gpuCaps.MaxPushConstantRange);
                        ImGui::Spacing();
                        ImGui::Text("MaxBoundDescriptorSets: %u", gpuCaps.MaxBoundDescriptorSets);
                        ImGui::Text("MaxDescriptorSetSamplers: %u", gpuCaps.MaxDescriptorSetSamplers);
                        ImGui::Text("MaxDescriptorSetUniformBuffers: %u", gpuCaps.MaxDescriptorSetUnifromBuffers);
                        ImGui::Text("MaxDescriptorSetStorageBuffers: %u", gpuCaps.MaxDescriptorSetStorageBuffers);
                        ImGui::Text("MaxDescriptorSetSampledImages: %u", gpuCaps.MaxDescriptorSetSampledImages);
                        ImGui::Text("MaxDescriptorSetStorageImages: %u", gpuCaps.MaxDescriptorSetStorageImages);
                        ImGui::Text("MaxDescriptorSetInputAttachments: %u", gpuCaps.MaxDescriptorSetInputAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxViewportDimensions: { %u, %u }", gpuCaps.MaxViewportDimensions[0], gpuCaps.MaxViewportDimensions[1]);
                        ImGui::Text("MaxClipDistances: %u", gpuCaps.MaxClipDistances);
                        ImGui::Text("MaxCullDistances: %u", gpuCaps.MaxCullDistances);
                        ImGui::Text("LineWidthRange: { %.1f, %.1f }", gpuCaps.LineWidthRange[0], gpuCaps.LineWidthRange[1]);
                        ImGui::Spacing();
                        ImGui::Text("MaxVertexInputAttributes: %u", gpuCaps.MaxVertexInputAttributes);
                        ImGui::Text("MaxVertexInputBindingStride: %u", gpuCaps.MaxVertexInputBindingStride);
                        ImGui::Text("MaxFragmentInputComponents: %u", gpuCaps.MaxFragmentInputComponents);
                        ImGui::Text("MaxFragmentOutputAttachments: %u", gpuCaps.MaxFragmentOutputAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxComputeWorkGroupSize: { %u, %u, %u }", gpuCaps.MaxComputeWorkGroupSize[0], gpuCaps.MaxComputeWorkGroupSize[1], gpuCaps.MaxComputeWorkGroupSize[2]);
                        ImGui::Text("MaxComputeSharedMemorySize: %u", gpuCaps.MaxComputeSharedMemorySize);
                        ImGui::Text("MaxComputeWorkGroupInvocations: %u", gpuCaps.MaxComputeWorkGroupInvocations);
                        ImGui::Spacing();
                        ImGui::Text("TimestampComputeAndGraphics: %s", gpuCaps.TimestampComputeAndGraphics ? "true" : "false");
                        ImGui::Text("TimestampPeriod: %f", gpuCaps.TimestampPeriod);

                        UI::TreePop();
                    }

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
        ImGui::PopStyleVar();
	}
}
