#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	enum class RenderTargetLoadOp
	{
		DONT_CARE = 0,
		CLEAR = 2,
		LOAD = 1
	};

	struct RenderTarget
	{
		RenderTarget() = default;
		RenderTarget(const String& name, TextureFormat format, TextureFilter filter = TextureFilter::LINEAR)
		{
			uint32 usage = TextureUsage::ATTACHMENT | TextureUsage::SAMPLED;

			TextureCreateInfo texInfo;
			texInfo.Name = name;
			texInfo.Format = format;
			texInfo.Usage = (TextureUsage)usage;
			texInfo.Width = 1;
			texInfo.Height = 1;
			texInfo.Layers = 1;
			texInfo.GenerateMipMap = false;
			texInfo.Sampler.Filter = filter;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			Texture = Texture2D::Create(texInfo);
			LoadOp = RenderTargetLoadOp::CLEAR;
		}

		RenderTarget(const Ref<Texture2D>& texture, RenderTargetLoadOp loadOp = RenderTargetLoadOp::LOAD)
		{
			Texture = texture;
			LoadOp = loadOp;
		}

		Ref<Texture2D> Texture;
		RenderTargetLoadOp LoadOp = RenderTargetLoadOp::CLEAR;
		LinearColor ClearColor = { 0.f, 0.f, 0.f, 1.0f };
		float DepthClearColor = 0.f;
		uint32 StencilClearColor = 1.f;
	};

	class RenderPass;

	struct RenderPassCreateInfo
	{
		String Name;
		Ref<RenderPass> InputPass;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		LinearColor DebugColor = LinearColor(0.f, 0.f, 0.f, 0.f);
	};

	class ATHENA_API RenderPass
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		virtual ~RenderPass() = default;

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) = 0;

		virtual void Resize(uint32 width, uint32 height) = 0;
		virtual void Bake() = 0;

		void SetOutput(const RenderTarget& target);

		Ref<Texture2D> GetOutput(const String& name) const;
		Ref<Texture2D> GetOutput(uint32 index) const;
		Ref<Texture2D> GetDepthOutput() const;

		const auto& GetAllOutputs() const { return m_Outputs; }

		uint32 GetColorTargetsCount() const;
		bool HasDepthRenderTarget() const;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
		std::vector<RenderTarget> m_Outputs;
	};
}
