#include "atnpch.h"
#include "GLRendererAPI.h"

#include <glad/glad.h>


namespace Athena
{
	void GLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         ATN_CORE_FATAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       ATN_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          ATN_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: ATN_CORE_TRACE(message); return;
		}

		ATN_CORE_ASSERT(false, "Unknown severity level!");
	}

	void GLRendererAPI::Init()
	{
#ifdef ATN_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);

		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_MULTISAMPLE);
	}

	void GLRendererAPI::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		glViewport(x, y, width, height);
	}

	void GLRendererAPI::Clear(const LinearColor& color)
	{
		if (m_OutputFramebuffer != nullptr)
		{
			m_OutputFramebuffer->ClearColorAndDepth(color);
		}
		else
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	void GLRendererAPI::DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount)
	{
		vertexBuffer->Bind();
		uint32 count = indexCount ? indexCount : vertexBuffer->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void GLRendererAPI::DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount)
	{
		vertexBuffer->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void GLRendererAPI::BindFramebuffer(const Ref<Framebuffer>& framebuffer)
	{
		m_OutputFramebuffer = framebuffer;
	}

	void GLRendererAPI::UnBindFramebuffer()
	{
		m_OutputFramebuffer->ResolveMutlisampling();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_OutputFramebuffer = nullptr;
	}
}
