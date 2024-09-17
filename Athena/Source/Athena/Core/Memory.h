#pragma once

#include <memory>


namespace Athena
{
	// Base class for reference counting objects
	class RefCounted
	{
	public:
		RefCounted()
			: m_Count(0)
		{

		}

		int32_t GetCount() const
		{
			return m_Count;
		}

	private:
		void Increment() const
		{
			m_Count++;
		}

		void Decrement() const
		{
			m_Count--;
		}

	private:
		template <typename T>
		friend class Ref;

	private:
		mutable int32_t m_Count;
	};


	template <typename T>
	class Ref
	{
	public:
		template <typename... Args>
		static Ref Create(Args&&... args)
		{
			return Ref(new T(std::forward<Args>(args)...));
		}

		Ref()
			: m_Object(nullptr)
		{

		}

		Ref(std::nullptr_t)
			: m_Object(nullptr)
		{

		}

		explicit Ref(T* ptr)
		{
			Acquire(ptr);
		}

		Ref(const Ref& other)
		{
			Acquire(other.Raw());
		}

		template <typename U>
		Ref(const Ref<U>& other)
		{
			Acquire(static_cast<T*>(other.Raw()));
		}

		Ref(Ref&& other) noexcept
		{
			m_Object = other.Raw();
			other.m_Object = nullptr;
		}

		template <typename U>
		Ref(Ref<U>&& other) noexcept
		{
			m_Object = static_cast<T*>(other.Raw());
			other.m_Object = nullptr;
		}

		Ref& operator=(const Ref& other)
		{
			if (m_Object == other.Raw())
				return *this;

			Reset(other.Raw());
			return *this;
		}

		template <typename U>
		Ref<T>& operator=(const Ref<U>& other)
		{
			if (m_Object == other.Raw())
				return *this;

			Reset(static_cast<T*>(other.Raw()));
			return *this;
		}

		Ref& operator=(Ref&& other) noexcept
		{
			if (m_Object == other.Raw())
				return *this;

			Release();
			m_Object = other.Raw();
			other.m_Object = nullptr;

			return *this;
		}

		template <typename U>
		Ref<T>& operator=(Ref<U>&& other) noexcept
		{
			if (m_Object == other.Raw())
				return *this;

			Release();
			m_Object = static_cast<T*>(other.Raw());
			other.m_Object = nullptr;

			return *this;
		}

		Ref& operator=(std::nullptr_t)
		{
			Release();
			return *this;
		}

		~Ref()
		{
			Release();
		}

		T* Raw() const
		{
			return m_Object;
		}

		void Reset(T* ptr)
		{
			Release();
			Acquire(ptr);
		}

		void Release()
		{
			if (m_Object)
			{
				RefCounted* objectBase = m_Object;
				objectBase->Decrement();

				if (objectBase->GetCount() == 0)
				{
					delete m_Object;
				}

				m_Object = nullptr;
			}
		}

		template <typename U>
		Ref<U> As() const
		{
			return Ref<U>(static_cast<U*>(m_Object));
		}

		explicit operator bool() const
		{
			return (bool)m_Object;
		}

		T& operator*() const
		{
			return *m_Object;
		}

		T* operator->() const
		{
			return Raw();
		}

		bool operator==(const Ref& other) const
		{
			return m_Object == other.Raw();
		}

		bool operator!=(const Ref& other) const
		{
			return m_Object != other.Raw();
		}

		bool operator==(std::nullptr_t) const
		{
			return m_Object == nullptr;
		}

		bool operator!=(std::nullptr_t) const
		{
			return m_Object != nullptr;
		}

	private:
		void Acquire(T* ptr)
		{
			m_Object = ptr;
			if (m_Object)
			{
				RefCounted* objectBase = m_Object;
				objectBase->Increment();
			}
		}

	private:
		template <typename U>
		friend class Ref;

	private:
		T* m_Object;
	};


	template <typename T>
	class WeakRef
	{
	public:
		WeakRef()
			: m_Object(nullptr)
		{

		}

		template <typename U>
		WeakRef(const Ref<U>& other)
			: m_Object(other.Raw())
		{

		}

		WeakRef(std::nullptr_t)
			: m_Object(nullptr)
		{

		}

		WeakRef(const WeakRef& other)
			: m_Object(other.Raw())
		{
			
		}

		template <typename U>
		WeakRef(const WeakRef<U>& other)
			: m_Object(static_cast<T*>(other.Raw()))
		{
		
		}

		WeakRef(WeakRef&& other) noexcept
		{
			m_Object = other.Raw();
			other.m_Object = nullptr;
		}

		template <typename U>
		WeakRef(WeakRef<U>&& other) noexcept
		{
			m_Object = static_cast<T*>(other.Raw());
			other.m_Object = nullptr;
		}

		WeakRef& operator=(const WeakRef& other)
		{
			if (m_Object == other.Raw())
				return *this;

			m_Object = other.Raw();
			return *this;
		}

		template <typename U>
		WeakRef<T>& operator=(const WeakRef<U>& other)
		{
			if (m_Object == other.Raw())
				return *this;

			m_Object = static_cast<T*>(other.Raw());
			return *this;
		}

		WeakRef& operator=(WeakRef&& other) noexcept
		{
			if (m_Object == other.Raw())
				return *this;

			m_Object = other.Raw();
			other.m_Object = nullptr;

			return *this;
		}

		template <typename U>
		WeakRef<T>& operator=(WeakRef<U>&& other) noexcept
		{
			if (m_Object == other.Raw())
				return *this;

			m_Object = static_cast<T*>(other.Raw());
			other.m_Object = nullptr;

			return *this;
		}

		WeakRef& operator=(std::nullptr_t)
		{
			m_Object = nullptr;
			return *this;
		}

		T* Raw() const
		{
			return m_Object;
		}

		template <typename U>
		WeakRef<U> As() const
		{
			return WeakRef<U>(static_cast<U*>(m_Object));
		}

		explicit operator bool() const
		{
			return (bool)m_Object;
		}

		T& operator*() const
		{
			return *m_Object;
		}

		T* operator->() const
		{
			return Raw();
		}

		bool operator==(const WeakRef& other) const
		{
			return m_Object == other.Raw();
		}

		bool operator!=(const WeakRef& other) const
		{
			return m_Object != other.Raw();
		}

		bool operator==(std::nullptr_t) const
		{
			return m_Object == nullptr;
		}

		bool operator!=(std::nullptr_t) const
		{
			return m_Object != nullptr;
		}

	private:
		T* m_Object;
	};


	template <typename T>
	class Scope
	{
	public:
		template <typename... Args>
		static Scope Create(Args&&... args)
		{
			return Scope(new T(std::forward<Args>(args)...));
		}

		Scope()
			: m_Object(nullptr)
		{

		}

		Scope(T* ptr)
			: m_Object(ptr)
		{

		}

		Scope(const Scope& other) = delete;
		Scope& operator=(const Scope& other) = delete;

		template <typename U>
		Scope(Scope<U>&& other) noexcept
		{
			m_Object = static_cast<T*>(other.m_Object);
			other.m_Object = nullptr;
		}

		template <typename U>
		Scope<T>& operator=(Scope<U>&& other) noexcept
		{
			if (m_Object == other.m_Object)
				return *this;

			m_Object = static_cast<T*>(other.m_Object);
			other.m_Object = nullptr;

			return *this;
		}

		Scope& operator=(std::nullptr_t)
		{
			Release();
			return *this;
		}

		~Scope()
		{
			Release();
		}

		T* Raw() const
		{
			return m_Object;
		}

		void Release()
		{
			delete m_Object;
			m_Object = nullptr;
		}

		void Reset(T* ptr)
		{
			Release();
			m_Object = ptr;
		}

		explicit operator bool() const
		{
			return (bool)m_Object;
		}

		T& operator*() const
		{
			return *m_Object;
		}

		T* operator->() const
		{
			return Raw();
		}

		bool operator==(const Scope& other) const
		{
			return m_Object == other.Raw();
		}

		bool operator!=(const Scope& other) const
		{
			return m_Object != other.Raw();
		}

		bool operator==(std::nullptr_t) const
		{
			return m_Object == nullptr;
		}

		bool operator!=(std::nullptr_t) const
		{
			return m_Object != nullptr;
		}

	private:
		template <typename U>
		friend class Scope;

	private:
		T* m_Object;
	};
}
