#include "Renderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	struct RendererData
	{
		Renderer::API API = Renderer::API::None;
		uint32 MaxFramesInFlight = 3;
		uint32 CurrentFrameIndex = 0;
		Ref<RendererAPI> RendererAPI;

		CommandQueue RenderCommandQueue = CommandQueue(1024 * 1024 * 10);	// 10 Mb
		std::vector<CommandQueue> ResourceFreeQueues;						// 2 Mb per queue in flight

		FilePath ShaderPackDirectory;
		FilePath ShaderCacheDirectory;
		std::unordered_map<String, String> GlobalShaderMacroses;
		Ref<ShaderPack> ShaderPack;

		RenderCapabilities RenderCaps;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDF_LUT;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;

		const uint32 BRDF_LUTResolution = 512;
	};

	RendererData s_Data;

	void Renderer::Init(const RendererConfig& config)
	{
		ATN_CORE_VERIFY(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		s_Data.API = config.API;
		s_Data.MaxFramesInFlight = config.MaxFramesInFlight;
		s_Data.CurrentFrameIndex = config.MaxFramesInFlight - 1;

		s_Data.ResourceFreeQueues.reserve(s_Data.MaxFramesInFlight);
		for (uint32 i = 0; i < s_Data.MaxFramesInFlight; ++i)
		{
			s_Data.ResourceFreeQueues.push_back(CommandQueue(1024 * 1024 * 2));	// 2 Mb
		}

		const FilePath& resourcesPath = Application::Get().GetConfig().EngineResourcesPath;
		s_Data.ShaderPackDirectory = resourcesPath / "ShaderPack";
		s_Data.ShaderCacheDirectory = resourcesPath / "Cache/ShaderPack";

		if (!FileSystem::Exists(s_Data.ShaderCacheDirectory))
			FileSystem::CreateDirectory(s_Data.ShaderCacheDirectory);

		s_Data.RendererAPI = RendererAPI::Create(s_Data.API);
		s_Data.RendererAPI->Init();
		s_Data.RendererAPI->GetRenderCapabilities(s_Data.RenderCaps);

		uint32 whiteTextureData = 0xffffffff;

		TextureCreateInfo texInfo;
		texInfo.Data = &whiteTextureData;
		texInfo.Width = 1;
		texInfo.Height = 1;
		texInfo.Layers = 1;
		texInfo.MipLevels = 1;
		texInfo.GenerateMipMap = false;
		texInfo.sRGB = false;
		texInfo.Format = TextureFormat::RGBA8;
		texInfo.Usage = TextureUsage::SHADER_READ_ONLY;
		texInfo.GenerateSampler = true;
		texInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.MipMapFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

		s_Data.WhiteTexture = Texture2D::Create(texInfo);

		uint32 blackTextureData = 0xff000000;
		texInfo.Data = &blackTextureData;

		s_Data.BlackTexture = Texture2D::Create(texInfo); 
	}

	void Renderer::Shutdown()
	{
		s_Data.WhiteTexture.Release();
		s_Data.BlackTexture.Release();

		s_Data.RendererAPI->Shutdown();
		s_Data.RendererAPI->WaitDeviceIdle();

		for (auto& queue : s_Data.ResourceFreeQueues)
		{
			queue.Flush();
		}
	}

	void Renderer::BeginFrame()
	{
		ATN_PROFILE_FUNC()
		s_Data.CurrentFrameIndex = (s_Data.CurrentFrameIndex + 1) % s_Data.MaxFramesInFlight;

		// Acquire image from swapchain
		Application::Get().GetWindow().GetSwapChain()->AcquireImage();

		// Free resources in queue
		// If resource is submitted to be freed on 'i' frame index, 
		// then it will be freed on 'i + FramesInFlight' frame, so it guarantees
		// that currently used resource will not be freed
		{
			ATN_PROFILE_SCOPE("ResourceFreeQueue::Flush")
			s_Data.ResourceFreeQueues[Renderer::GetCurrentFrameIndex()].Flush();
		}

		s_Data.RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		ATN_PROFILE_FUNC()
		s_Data.RendererAPI->EndFrame();
	}

	void Renderer::WaitAndRender()
	{
		ATN_PROFILE_FUNC()
		Timer timer = Timer();

		s_Data.RenderCommandQueue.Flush();

		Application::Get().GetStats().Renderer_WaitAndRender = timer.ElapsedTime();
	}

	void Renderer::BlitToScreen(const Ref<Texture2D>& texture)
	{
		s_Data.RendererAPI->BlitToScreen(texture);
	}

	Renderer::API Renderer::GetAPI()
	{
		return s_Data.API;
	}

	uint32 Renderer::GetFramesInFlight()
	{
		return s_Data.MaxFramesInFlight;
	}

	uint32 Renderer::GetCurrentFrameIndex()
	{
		return s_Data.CurrentFrameIndex;
	}

	const FilePath& Renderer::GetShaderPackDirectory()
	{
		return s_Data.ShaderPackDirectory;
	}

	const FilePath& Renderer::GetShaderCacheDirectory()
	{
		return s_Data.ShaderCacheDirectory;
	}

	Ref<ShaderPack> Renderer::GetShaderLibrary()
	{
		return s_Data.ShaderPack;
	}

	const std::unordered_map<String, String>& Renderer::GetGlobalShaderMacroses()
	{
		return s_Data.GlobalShaderMacroses;
	}

	void Renderer::SetGlobalShaderMacros(const String& name, const String& value)
	{
		s_Data.GlobalShaderMacroses[name] = value;
	}

	const RenderCapabilities& Renderer::GetRenderCaps()
	{
		return s_Data.RenderCaps;
	}

	uint64 Renderer::GetMemoryUsage()
	{
		return s_Data.RendererAPI->GetMemoryUsage();
	}

	Ref<Texture2D> Renderer::GetBRDF_LUT()
	{
		return s_Data.BRDF_LUT;
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data.BlackTexture;
	}

	Ref<VertexBuffer> Renderer::GetCubeVertexBuffer()
	{
		return s_Data.CubeVertexBuffer;
	}

	Ref<VertexBuffer> Renderer::GetQuadVertexBuffer()
	{
		return s_Data.QuadVertexBuffer;
	}

	CommandQueue& Renderer::GetRenderCommandQueue()
	{
		return s_Data.RenderCommandQueue;
	}

	CommandQueue& Renderer::GetResourceFreeQueue()
	{
		return s_Data.ResourceFreeQueues[Renderer::GetCurrentFrameIndex()];
	}
}
