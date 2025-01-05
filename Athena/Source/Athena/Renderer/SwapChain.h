#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API SwapChain
	{
	public:
		static Ref<SwapChain> Create(void* windowHandle, bool vsync = false);
		virtual ~SwapChain() = default;

		virtual void OnWindowResize() = 0;
		virtual bool Recreate() = 0;
		virtual void SetVSync(bool enabled) = 0;

		virtual void AcquireImage() = 0;
		virtual void Present() = 0;

		virtual uint32 GetCurrentImageIndex() = 0;
	};
}
