#pragma once

#include "Athena/Renderer/Shader.h"

namespace Athena
{
	class ATHENA_API GLShader: public Shader
	{
	public:
		GLShader(const Filepath& filepath);
		GLShader(const String& name, const String& vertexSrc, const String& fragmentSrc);
		virtual ~GLShader();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void Reload() override;

	private:
		bool Compile(const std::unordered_map<ShaderType, String>& shaderSources);

	private:
		uint32 m_RendererID = 0;
		bool m_Compiled = false;
	};
}
