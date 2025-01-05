#pragma once

#include <memory>


namespace Athena
{
    struct RefCntrBlock
    {
        RefCntrBlock() = default;
        virtual ~RefCntrBlock() = default;

        virtual void Dispose() = 0;

        void Increment()
        {
            Counter++;
        }

        void Decrement()
        {
            Counter--;
        }

        void IncrementWeak()
        {
            WeakCounter++;
        }

        void DecrementWeak()
        {
            WeakCounter--;
        }

        uint32 GetCount() const
        {
            return Counter;
        }

        uint32 GetWeakCount() const
        {
            return WeakCounter;
        }

        uint32 Counter = 1;
        uint32 WeakCounter = 0;
    };

    template<class T>
    struct RefPtrAndCntrBlock : RefCntrBlock
    {
        T* Pointer;

        RefPtrAndCntrBlock(T* p)
            : Pointer(p)
        {}

        virtual void Dispose() override
        {
            delete Pointer;
        }
    };

    template<class T>
    struct RefObjectAndCntrBlock : RefCntrBlock
    {
        alignas(T) byte Object[sizeof(T)];

        template<class ... Args>
        RefObjectAndCntrBlock(Args&&... args)
        {
            new(Object) T(std::forward<Args>(args)...);
        }

        virtual void Dispose() override
        {
            reinterpret_cast<T*>(Object)->~T();
        }
    };

    template <typename T>
    class Ref
    {
    public:
        template<class ... Args>
        static Ref<T> Create(Args&&... args)
        {
            Ref<T> result;
            RefObjectAndCntrBlock<T>* block = new RefObjectAndCntrBlock<T>(args...);
            result.m_Pointer = reinterpret_cast<T*>(block->Object);
            result.m_ControlBlock = block;

            return result;
        }

        Ref()
            : m_Pointer(nullptr), m_ControlBlock(nullptr)
        {

        }

        Ref(std::nullptr_t)
            : m_Pointer(nullptr), m_ControlBlock(nullptr)
        {

        }

        explicit Ref(T* ptr)
            : m_Pointer(ptr)
        {
            m_ControlBlock = new RefPtrAndCntrBlock(ptr);
        }

        Ref(const Ref& other)
        {
            Acquire(other.m_ControlBlock, other.m_Pointer);
        }

        template <typename U>
        Ref(const Ref<U>& other)
        {
            Acquire(other.m_ControlBlock, static_cast<T*>(other.m_Pointer));
        }

        Ref(Ref&& other) noexcept
        {
            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = other.m_Pointer;

            other.m_ControlBlock = nullptr;
            other.m_Pointer = nullptr;
        }

        template <typename U>
        Ref(Ref<U>&& other) noexcept
        {
            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = static_cast<T*>(other.m_Pointer);

            other.m_ControlBlock = nullptr;
            other.m_Pointer = nullptr;
        }

        Ref& operator=(const Ref& other)
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();
            Acquire(other.m_ControlBlock, other.m_Pointer);

            return *this;
        }

        template <typename U>
        Ref<T>& operator=(const Ref<U>& other)
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();
            Acquire(other.m_ControlBlock, static_cast<T*>(other.m_Pointer));

            return *this;
        }

        Ref& operator=(Ref&& other) noexcept
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();

            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = other.m_Pointer;

            other.m_Pointer = nullptr;
            other.m_ControlBlock = nullptr;

            return *this;
        }

        template <typename U>
        Ref<T>& operator=(Ref<U>&& other) noexcept
        {
            if (m_Pointer == static_cast<T*>(other.m_Pointer))
                return *this;

            Release();

            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = static_cast<T*>(other.m_Pointer);

            other.m_Pointer = nullptr;
            other.m_ControlBlock = nullptr;

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
            return m_Pointer;
        }

        void Reset(T* ptr)
        {
            Release();

            m_Pointer = ptr;
            m_ControlBlock = new RefPtrAndCntrBlock(ptr);
        }

        void Release()
        {
            if (m_Pointer)
            {
                m_ControlBlock->Decrement();

                if (m_ControlBlock->GetCount() == 0)
                {
                    m_ControlBlock->Dispose();

                    if (m_ControlBlock->GetWeakCount() == 0)
                    {
                        delete m_ControlBlock;
                    }
                }

                m_Pointer = nullptr;
                m_ControlBlock = nullptr;
            }
        }

        uint32 GetCount() const
        {
            if (m_ControlBlock)
                return m_ControlBlock->GetCount();

            return 0;
        }

        uint32 GetWeakCount() const
        {
            if (m_ControlBlock)
                return m_ControlBlock->GetWeakCount();

            return 0;
        }

        template <typename U>
        Ref<U> As() const
        {
            return Ref<U>(*this);
        }

        explicit operator bool() const
        {
            return (bool)m_Pointer;
        }

        T& operator*() const
        {
            return *m_Pointer;
        }

        T* operator->() const
        {
            return Raw();
        }

        bool operator==(const Ref& other) const
        {
            return m_Pointer == other.Raw();
        }

        bool operator!=(const Ref& other) const
        {
            return m_Pointer != other.Raw();
        }

        bool operator==(std::nullptr_t) const
        {
            return m_Pointer == nullptr;
        }

        bool operator!=(std::nullptr_t) const
        {
            return m_Pointer != nullptr;
        }

    private:
        void Acquire(RefCntrBlock* cntr, T* pointer)
        {
            m_ControlBlock = cntr;
            m_Pointer = pointer;

            if (m_ControlBlock)
                m_ControlBlock->Increment();
        }

    private:
        template <typename U>
        friend class Ref;

        template <typename U>
        friend class WeakRef;

    private:
        RefCntrBlock* m_ControlBlock;
        T* m_Pointer;
    };


    template <typename T>
    class WeakRef
    {
    public:
        WeakRef()
            : m_Pointer(nullptr), m_ControlBlock(nullptr)
        {

        }

        WeakRef(std::nullptr_t)
            : m_Pointer(nullptr), m_ControlBlock(nullptr)
        {

        }

        template <typename U>
        WeakRef(const Ref<U>& other)
        {
            Acquire(other.m_ControlBlock, static_cast<T*>(other.m_Pointer));
        }

        WeakRef(const WeakRef& other)
        {
            Acquire(other.m_ControlBlock, other.m_Pointer);
        }

        template <typename U>
        WeakRef(const WeakRef<U>& other)
        {
            Acquire(other.m_ControlBlock, static_cast<T*>(other.m_Pointer));
        }

        WeakRef(WeakRef&& other) noexcept
        {
            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = other.m_Pointer;

            other.m_ControlBlock = nullptr;
            other.m_Pointer = nullptr;
        }

        template <typename U>
        WeakRef(WeakRef<U>&& other) noexcept
        {
            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = static_cast<T*>(other.m_Pointer);

            other.m_ControlBlock = nullptr;
            other.m_Pointer = nullptr;
        }

        WeakRef& operator=(const WeakRef& other)
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();
            Acquire(other.m_ControlBlock, other.m_Pointer);

            return *this;
        }

        template <typename U>
        WeakRef<T>& operator=(const WeakRef<U>& other)
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();
            Acquire(other.m_ControlBlock, static_cast<T*>(other.m_Pointer));

            return *this;
        }

        WeakRef& operator=(WeakRef&& other) noexcept
        {
            if (m_Pointer == other.m_Pointer)
                return *this;

            Release();

            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = other.m_Pointer;

            other.m_Pointer = nullptr;
            other.m_ControlBlock = nullptr;

            return *this;
        }

        template <typename U>
        WeakRef<T>& operator=(WeakRef<U>&& other) noexcept
        {
            if (m_Pointer == static_cast<T*>(other.m_Pointer))
                return *this;

            Release();

            m_ControlBlock = other.m_ControlBlock;
            m_Pointer = static_cast<T*>(other.m_Pointer);

            other.m_Pointer = nullptr;
            other.m_ControlBlock = nullptr;

            return *this;
        }

        WeakRef& operator=(std::nullptr_t)
        {
            Release();
            return *this;
        }

        ~WeakRef()
        {
            Release();
        }

        T* Raw() const
        {
            return m_Pointer;
        }

        void Release()
        {
            if (m_Pointer)
            {
                m_ControlBlock->DecrementWeak();

                if (m_ControlBlock->GetWeakCount() == 0 && m_ControlBlock->GetCount() == 0)
                {
                    delete m_ControlBlock;
                }

                m_Pointer = nullptr;
                m_ControlBlock = nullptr;
            }
        }

        uint32 GetRefCount() const
        {
            if (m_ControlBlock)
                return m_ControlBlock->GetCount();

            return 0;
        }

        uint32 GetWeakCount() const
        {
            if (m_ControlBlock)
                return m_ControlBlock->GetWeakCount();

            return 0;
        }

        bool Expired() const
        {
            return GetRefCount() == 0;
        }

        template <typename U>
        WeakRef<U> As() const
        {
            return WeakRef<U>(*this);
        }

        explicit operator bool() const
        {
            return (bool)m_Pointer;
        }

        T& operator*() const
        {
            return *m_Pointer;
        }

        T* operator->() const
        {
            return Raw();
        }

        bool operator==(const WeakRef& other) const
        {
            return m_Pointer == other.Raw();
        }

        bool operator!=(const WeakRef& other) const
        {
            return m_Pointer != other.Raw();
        }

        bool operator==(std::nullptr_t) const
        {
            return m_Pointer == nullptr;
        }

        bool operator!=(std::nullptr_t) const
        {
            return m_Pointer != nullptr;
        }

    private:
        void Acquire(RefCntrBlock* cntr, T* pointer)
        {
            m_ControlBlock = cntr;
            m_Pointer = pointer;

            if (m_ControlBlock)
                m_ControlBlock->IncrementWeak();
        }

    private:
        template <typename U>
        friend class WeakRef;

    private:
        RefCntrBlock* m_ControlBlock;
        T* m_Pointer;
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
