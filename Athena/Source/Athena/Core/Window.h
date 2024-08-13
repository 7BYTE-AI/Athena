#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Input/Event.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/SwapChain.h"


namespace Athena
{
	using WindowEventCallback = std::function<void(const Ref<Event>&)>;
	using TitlebarHitTestCallback = std::function<bool()>;

	enum class WindowMode
	{
		Default = 0,
		Maximized = 1,
		Minimized = 2,
		Fullscreen = 3
	};

	struct WindowCreateInfo
	{
		uint32 Width = 1280; 
		uint32 Height = 720;
		String Title = "Athena App";
		bool VSync = true;
		WindowMode StartMode = WindowMode::Default;
		bool CustomTitlebar = false;
		bool WindowResizeable = true;
		FilePath Icon;
		WindowEventCallback EventCallback = nullptr;
	};

	class ATHENA_API Window
	{
	public:
		static Scope<Window> Create(const WindowCreateInfo& desc);

		~Window();
		
		void PollEvents();
		void SwapBuffers();

		uint32 GetWidth() const { return m_Data.Width; }
		uint32 GetHeight() const { return m_Data.Height; }

		void DestroySwapChain() { m_SwapChain.Release(); }
		Ref<SwapChain> GetSwapChain() const { return m_SwapChain; }

		inline void SetEventCallback(const WindowEventCallback& callback)
		{
			m_Data.EventCallback = callback;
		}

		inline void SetTitlebarHitTestCallback(const TitlebarHitTestCallback& callback)
		{
			m_Data.TitlebarHitTest = callback;
		}

		void SetVSync(bool enabled);
		bool IsVSync() const { return m_Data.VSync; }

		void* GetNativeWindow() { return m_WindowHandle; }

		void SetIcon(const FilePath& path);

		void SetWindowMode(WindowMode mode);
		WindowMode GetWindowMode() const { return m_Data.Mode; }

	public:
		struct WindowData
		{
			uint32 Width = 1280;
			uint32 Height = 720;
			bool VSync = true;
			String Title = "Athena App";
			WindowMode Mode = WindowMode::Default;
			bool CustomTitlebar = false;

			WindowEventCallback EventCallback = [](const Ref<Event>&) {};
			TitlebarHitTestCallback TitlebarHitTest = []() { return false; };
		};

	private:
		void* m_WindowHandle = nullptr;
		WindowData m_Data;
		Ref<SwapChain> m_SwapChain;

		static inline uint8 m_WindowCount = 0;
	};
}
