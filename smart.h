#pragma once

template <typename T>
class smartptr
{
public:
	smartptr() : m_Ptr(NULL) {}
	smartptr(T* p)
	{
		if (m_Ptr = p)
			p->AddRef();
	}
	smartptr(int null) : m_Ptr(NULL) { assert(null == 0); }
	smartptr(const smartptr &p)
	{
		if (m_Ptr = p.m_Ptr)
			m_Ptr->AddRef();
	}
	~smartptr()
	{
		if (m_Ptr)
			m_Ptr->Release();
	}
	smartptr& operator=(T* p)
	{
		if (p)
			p->AddRef();
		if (m_Ptr)
			m_Ptr->Release();
		m_Ptr = p;
		return *this;
	}
	smartptr& operator=(const smartptr &p)
	{
		if (p.m_Ptr)
			p.m_Ptr->AddRef();
		if (m_Ptr)
			m_Ptr->Release();
		m_Ptr = p.m_Ptr;
		return *this;
	}
	operator bool() const
	{
		return m_Ptr != NULL;
	}
	bool operator!() const
	{
		return m_Ptr == NULL;
	}
	bool operator==(T* p) const
	{
		return m_Ptr == p;
	}
	bool operator==(const T* p) const
	{
		return m_Ptr == p;
	}
	bool operator==(const smartptr<T>& p) const
	{
		return m_Ptr == p.m_Ptr;
	}
	bool operator!=(T* p) const
	{
		return m_Ptr != p;
	}
	bool operator!=(const T* p) const
	{
		return m_Ptr != p;
	}
	bool operator!=(const smartptr &p) const
	{
		return m_Ptr != p.m_Ptr;
	}
	bool operator<(T* p) const
	{
		return m_Ptr < p;
	}
	bool operator<(const T* p) const
	{
		return m_Ptr < p;
	}
	bool operator<(const smartptr& p) const
	{
		return m_Ptr < p.m_Ptr;
	}
	bool operator<=(T* p) const
	{
		return m_Ptr <= p;
	}
	bool operator<=(const T* p) const
	{
		return m_Ptr <= p;
	}
	bool operator<=(const smartptr& p) const
	{
		return m_Ptr <= p.m_Ptr;
	}
	bool operator>(T* p) const
	{
		return m_Ptr > p;
	}
	bool operator>(const T* p) const
	{
		return m_Ptr > p;
	}
	bool operator>(const smartptr& p) const
	{
		return m_Ptr > p.m_Ptr;
	}
	bool operator>=(T* p) const
	{
		return m_Ptr >= p;
	}
	bool operator>=(const T* p) const
	{
		return m_Ptr >= p;
	}
	bool operator>=(const smartptr& p) const
	{
		return m_Ptr >= p.m_Ptr;
	}
	T* operator->(void) { return m_Ptr; }
	const T* operator->(void) const { return m_Ptr; }
	T& operator*() { return *m_Ptr; }
	const T& operator*() const { return *m_Ptr; }
	operator T*() { return m_Ptr; }
	operator const T*() const { return m_Ptr; }
	T* Get() { return m_Ptr; }
	const T* Get() const { return m_Ptr; }
	void Swap(smartptr<T>& p)
	{
		std::swap(m_Ptr, p.m_Ptr);
	}
	T* ReleasePtr()
	{
		T* p = m_Ptr;
		m_Ptr = NULL;
		return p;
	}

private:
	T* m_Ptr;
};

template <typename T>
inline bool operator==(const smartptr<T> &p, int null)
{
	assert(!null);
	return p.Get() == NULL;
}
template <typename T>
inline bool operator==(int null, const smartptr<T> &p)
{
	assert(!null);
	return p.Get() == NULL;
}
template <typename T>
inline bool operator!=(const smartptr<T> &p, int null)
{
	assert(!null);
	return p.Get() != NULL;
}
template <typename T>
inline bool operator!=(int null, const smartptr<T> &p)
{
	assert(!null);
	return p.Get() != NULL;
}


class refcounted
{
public:
	refcounted(): m_RefCount(0) {}
	virtual ~refcounted() {}
	void AddRef()
	{
		++m_RefCount;
	}
	void Release()
	{
		if (!--m_RefCount)
		{
			delete this;
		}
#ifdef _DEBUG
		else
		{
			assert(m_RefCount > 0);
		}
#endif
	}
private:
	int m_RefCount;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


// works like std::auto_ptr, operator= transfers the ownership
struct SWinHandle
{
	SWinHandle(HANDLE _handle=NULL)
		: handle(_handle)
	{
	}
	SWinHandle(SWinHandle& other)
		: handle(other)
	{
		other.handle = NULL;
	}
	~SWinHandle()
	{
		Close();
	}
	void Close()
	{
		if (handle!=NULL && handle!=INVALID_HANDLE_VALUE)
		{
			::CloseHandle(handle);
			handle = NULL;
		}
	}
	operator HANDLE() const
	{
		return handle;
	}
	HANDLE Release()
	{
		HANDLE h = handle;
		handle = NULL;
		return h;
	}
	SWinHandle& operator=(HANDLE _handle)
	{
		Close();
		handle = _handle;
		return *this;
	}
	SWinHandle& operator=(SWinHandle& other)
	{
		Close();
		handle = other;
		other.handle = NULL;
		return *this;
	}

	HANDLE handle;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


// Use this with handles returned by FindFirstFile().
// works like std::auto_ptr, operator= transfers the ownership
struct SFindHandle
{
	SFindHandle(HANDLE _handle=NULL)
		: handle(_handle)
	{
	}
	SFindHandle(SFindHandle& other)
		: handle(other)
	{
		other.handle = NULL;
	}
	~SFindHandle()
	{
		Close();
	}
	void Close()
	{
		if (handle!=NULL && handle!=INVALID_HANDLE_VALUE)
		{
			::FindClose(handle);
			handle = NULL;
		}
	}
	operator HANDLE() const
	{
		return handle;
	}
	HANDLE Release()
	{
		HANDLE h = handle;
		handle = NULL;
		return h;
	}
	SFindHandle& operator=(HANDLE _handle)
	{
		Close();
		handle = _handle;
		return *this;
	}
	SFindHandle& operator=(SFindHandle& other)
	{
		Close();
		handle = other;
		other.handle = NULL;
		return *this;
	}

	HANDLE handle;
};
