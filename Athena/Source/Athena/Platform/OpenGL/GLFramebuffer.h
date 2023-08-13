#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Framebuffer.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLFramebuffer: public Framebuffer
	{
	public:
		GLFramebuffer(const FramebufferCreateInfo& info);
		virtual ~GLFramebuffer();

		void Recreate();

		virtual void Resize(uint32 width, uint32 height) override;

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void BindColorAttachmentAsImage(uint32 index, uint32 slot = 0, uint32 mipLevel = 0) const override;
		virtual void BindColorAttachment(uint32 index, uint32 slot = 0) const override;
		virtual void BindDepthAttachment(uint32 slot = 0) const override;

		virtual const FramebufferCreateInfo& GetCreateInfo() const override { return m_CreateInfo; }

		virtual void* GetColorAttachmentRendererID(uint32 index = 0) const override;
		virtual void* GetDepthAttachmentRendererID() const override;

		virtual int ReadPixel(uint32 attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(uint32 attachmentIndex, int value) override;

		virtual void ResolveMutlisampling() override;
		virtual void BlitToScreen() const override;

	private:
		void DeleteAttachments();
		void AttachColorTexture(uint32 id, uint32 samples, uint32 index);
		void AttachDepthTexture(uint32 id, uint32 samples);

		void CreateFramebufferObject(uint32* rendererID, bool resolved);

		bool IsMultisample() const { return m_CreateInfo.Samples > 1; }

	private:
		uint32 m_FramebufferID = 0;		
		uint32 m_ResolvedFramebufferID = 0;		// if not multisample - invalid
		FramebufferCreateInfo m_CreateInfo;

		std::vector<FramebufferAttachmentCreateInfo> m_ColorAttachmentCreateInfos;
		std::vector<uint32> m_ColorAttachments;
		std::vector<uint32> m_ColorAttachmentsResolved; // if not multisample - invalid

		FramebufferAttachmentCreateInfo m_DepthAttachmentCreateInfo = TextureFormat::NONE;
		uint32 m_DepthAttachment;
		uint32 m_DepthAttachmentResolved;   // if not multisample - invalid
	};
}
