#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"

#include <functional>


namespace Athena
{
	enum class EventType
	{
		None = 0,
		WindowClosed, WindowResized, WindowMoved,
		WindowGainedFocus, WindowLostFocus, WindowMaximized, WindowIconified, WindowRestored,
		KeyPressed, KeyReleased, KeyTyped,
		MouseMoved, MouseButtonPressed, MouseButtonReleased, MouseScrolled
	};


	enum EventCategory
	{
		None = 0,
		EventCategoryWindow			    = BIT(0),
		EventCategoryInput				= BIT(1),
		EventCategoryKeyboard			= BIT(2),
		EventCategoryMouse				= BIT(3),
		EventCategoryMouseButton		= BIT(4)
	};

	                                                                      
#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
						       virtual EventType GetEventType() const override { return GetStaticType(); }\
							   virtual const char* GetName() const override { return #type"Event"; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }


	class ATHENA_API Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual String ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}

	public:
		bool Handled = false;
	};


	class EventDispatcher
	{
		template <typename T>
		using EventFn = std::function<bool(T&)>;

	public:
		EventDispatcher(Event& event)
			: m_Event(event) {}

		template <typename T>
		bool Dispatch(const EventFn<T>& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(*(T*)&m_Event);
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};


	template <>
	inline String ToString<Event>(const Event& event)
	{
		return event.ToString();
	}
}
