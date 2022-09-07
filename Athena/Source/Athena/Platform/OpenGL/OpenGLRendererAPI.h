#pragma once

#include "Athena/Renderer/RendererAPI.h"


namespace Athena
{
	class ATHENA_API OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) override;
		virtual void Clear(const LinearColor& color) override;
		virtual void DrawTriangles(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount = 0) override;
		virtual void SetLineWidth(float width) override;
	};
}