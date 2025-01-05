#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Font.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/Material.h"

// WINAPI 
#ifdef DrawText
	#undef DrawText
#endif


namespace Athena
{
	enum class Renderer2DSpace
	{
		WorldSpace = 1,
		ScreenSpace,
		Billboard	// TODO
	};

	// QUADS

	struct QuadVertex
	{
		LinearColor Position; 	// Vector4 has alignment 16
		LinearColor Color;
		Vector2 TexCoords;
		uint32 TexIndex;
	};

	struct QuadBatch
	{
		uint32 VertexOffset;
		uint32 IndexCount;
		Ref<Material> Material;
	};

	// CIRCLES

	struct CircleVertex
	{
		LinearColor WorldPosition;
		LinearColor Color;
		Vector3 LocalPosition;
		float Thickness;
		float Fade;
	};

	// LINES

	struct LineVertex
	{
		Vector4 Position;
		LinearColor Color;
	};

	struct LineBatch
	{
		uint32 VertexOffset;
		uint32 VertexCount;
		float LineWidth;
	};

	// TEXT

	struct TextVertex
	{
		LinearColor Position;
		LinearColor Color;
		Vector2 TexCoords;
	};

	struct TextBatch
	{
		uint32 VertexOffset;
		uint32 IndexCount;
		Ref<Material> Material; // contains only 1 atlas texture
	};

	struct TextParams
	{
		TextParams() = default;
		TextParams(const LinearColor& color)
			: Color(color) {}

		LinearColor Color;
		float MaxWidth = 10.f;
		float Kerning = 0.f;
		float LineSpacing = 0.f;
		bool Shadowing = false;
		float ShadowDistance = 0.f;
		LinearColor ShadowColor;
	};


	class ATHENA_API SceneRenderer2D
	{
	public:
		static Ref<SceneRenderer2D> Create(const Ref<RenderPass>& renderPass);
		~SceneRenderer2D();

		void Init(const Ref<RenderPass>& renderPass);
		void Shutdown();

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
		void EndScene();

		void DrawQuad(Vector2 position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawQuad(Vector3 position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawQuad(Vector2 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawQuad(Vector3 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawQuad(const Matrix4& transform, Renderer2DSpace space = Renderer2DSpace::WorldSpace, const LinearColor& color = LinearColor::White);
		void DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, Renderer2DSpace space = Renderer2DSpace::WorldSpace, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawBillboardFixedSize(const Vector3& position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawBillboardFixedSize(const Vector3& position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawCircle(const Matrix4& transform, Renderer2DSpace space = Renderer2DSpace::WorldSpace, const LinearColor& color = LinearColor::White, float thickness = 1.f, float fade = 0.005f);

		void DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color = LinearColor::White);

		void DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		void DrawRect(const Matrix4& transform, const LinearColor& color = LinearColor::White);

		void DrawText(const String& text, const Ref<Font>& font, const Matrix4& transform, Renderer2DSpace space = Renderer2DSpace::WorldSpace, const TextParams& params = TextParams());
		void DrawScreenSpaceText(const String& text, const Ref<Font>& font, Vector2 position, Vector2 scale, const TextParams& params = TextParams());

		void SetLineWidth(float width);
		float GetLineWidth();

	private:
		void FlushQuads();
		void NextBatchQuads();

		void FlushLines();
		void NextBatchLines();

		void FlushText();
		void NextBatchText();

		void FlushIndexBuffer();

		Matrix4 GetSpaceTransform(const Matrix4& transform, Renderer2DSpace space);

	private:
		static const uint32 s_MaxTextureSlots = 32;

	private:
		bool m_BeginScene = false;
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Vector2u m_ViewportSize;

		Vector3 m_CameraPos;
		Matrix4 m_ViewProjection;
		Matrix4 m_InverseView;
		Matrix4 m_OrthoViewProjection;

		Vector4 m_QuadVertexPositions[4];

		// Shared indices for quads, circles and text
		DynamicGPUBuffer<IndexBuffer> m_IndexBuffer;
		std::vector<uint32> m_IndicesCount; // Per frame in flight

		Ref<Pipeline> m_QuadPipeline;
		DynamicGPUBuffer<VertexBuffer> m_QuadVertexBuffer;
		std::vector<QuadBatch> m_QuadBatches;	// TODO: clear batches(materials) when too much allocated
		uint32 m_QuadBatchIndex = 0;

		std::array<Ref<Texture2D>, s_MaxTextureSlots> m_TextureSlots;
		uint32 m_TextureSlotIndex = 1; // 0 - white texture

		Ref<Pipeline> m_CirclePipeline;
		DynamicGPUBuffer<VertexBuffer> m_CircleVertexBuffer;
		uint64 m_CircleIndexCount = 0;

		Ref<Pipeline> m_LinePipeline;
		DynamicGPUBuffer<VertexBuffer> m_LineVertexBuffer;
		std::vector<LineBatch> m_LineBatches;
		uint32 m_LineBatchIndex = 0;
		float m_LineWidth = 1.f;

		Ref<Pipeline> m_TextPipeline;
		DynamicGPUBuffer<VertexBuffer> m_TextVertexBuffer;
		std::vector<TextBatch> m_TextBatches;	// TODO: clear batches(materials) when too much allocated
		uint32 m_TextBatchIndex = 0;
		Ref<Font> m_CurrentFont;
	};
}
