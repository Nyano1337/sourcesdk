//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SMARTPTR_H
#define SMARTPTR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

class CRefCountAccessor
{
public:
	template< class T >
	static void AddRef( T *pObj )
	{
		pObj->AddRef();
	}

	template< class T >
	static void Release( T *pObj )
	{
		pObj->Release();
	}
};

// This can be used if your objects use AddReference/ReleaseReference function names.
class CRefCountAccessorLongName
{
public:
	template< class T >
	static void AddRef( T *pObj )
	{
		pObj->AddReference();
	}

	template< class T >
	static void Release( T *pObj )
	{
		pObj->ReleaseReference();
	}
};

class CNullRefCountAccessor 
{
public:
	template< class T >
	static void AddRef( T *pObj ) 
	{
	}

	template< class T >
	static void Release( T *pObj )
	{
		delete pObj;
	}
};


//
//	CPlainAutoPtr
//		is a smart wrapper for a pointer on the stack that performs "delete" upon destruction.
//
//	No reference counting is performed, copying is prohibited "s_p2.Attach( s_p1.Detach() )" should be used
//	for readability and ease of maintenance.
//
//	Auto pointer supports an "arrow" operator for invoking methods on the pointee and a "dereference" operator
//	for getting a pointee reference.
//
//	No automatic casting to bool/ptrtype is performed to avoid bugs and problems (read on "safe bool idiom"
//	if casting to bool or pointer happens to be useful).
//
//	Test for validity with "IsValid", get the pointer with "Get".
//
template < typename T >
class CPlainAutoPtr
{
public:
	explicit CPlainAutoPtr( T *p = NULL )		: m_p( p ) {}
	~CPlainAutoPtr( void )						{ Delete(); }

public:
	void Delete( void )							{ delete Detach(); }

private:	// Disallow copying, use Detach() instead to avoid ambiguity
	CPlainAutoPtr( CPlainAutoPtr const &x );
	CPlainAutoPtr & operator = ( CPlainAutoPtr const &x );

public:
	void Attach( T *p )							{ m_p = p; }
	T * Detach( void )							{ T * p( m_p ); m_p = NULL; return p; }

public:
	bool IsValid( void ) const					{ return m_p != NULL; }
	T * Get( void ) const						{ return m_p; }
	T * operator -> ( void ) const				{ return Get(); }
	T & operator *  ( void ) const				{ return *Get(); }

private:
	T * m_p;
};

//
//	CArrayAutoPtr
//		is a smart wrapper for an array pointer on the stack that performs "delete []" upon destruction.
//
//	No reference counting is performed, copying is prohibited "s_p2.Attach( s_p1.Detach() )" should be used
//	for readability and ease of maintenance.
//
//	Auto pointer supports an "indexing" operator for accessing array elements.
//
//	No automatic casting to bool/ptrtype is performed to avoid bugs and problems (read on "safe bool idiom"
//	if casting to bool or pointer happens to be useful).
//
//	Test for validity with "IsValid", get the array pointer with "Get".
//
template < typename T >
class CArrayAutoPtr : public CPlainAutoPtr < T > // Warning: no polymorphic destructor (delete on base class will be a mistake)
{
public:
	explicit CArrayAutoPtr( T *p = NULL )		{ this->Attach( p ); }
	~CArrayAutoPtr( void )						{ Delete(); }

public:
	void Delete( void )							{ delete [] this->Detach(); }

public:
	T & operator [] ( int k ) const				{ return this->Get()[ k ]; }
};


// Smart pointers can be used to automatically free an object when nobody points
// at it anymore. Things contained in smart pointers must implement AddRef and Release
// functions. If those functions are private, then the class must make
// CRefCountAccessor a friend.
template<class T, class RefCountAccessor=CRefCountAccessor>
class CSmartPtr
{
public:
					CSmartPtr();
					CSmartPtr( const T &copyFrom );
					CSmartPtr( T &&moveFrom );
					CSmartPtr( T *pObj );
					CSmartPtr( const CSmartPtr<T,RefCountAccessor> &copyFrom );
					CSmartPtr( CSmartPtr<T,RefCountAccessor> &&moveFrom );
					~CSmartPtr();

	CSmartPtr<T,RefCountAccessor> &CopyFrom( const CSmartPtr<T,RefCountAccessor> &copyFrom );
	CSmartPtr<T,RefCountAccessor> &MoveFrom( CSmartPtr<T,RefCountAccessor> &&moveFrom );

	CSmartPtr<T,RefCountAccessor> &operator=( const CSmartPtr<T,RefCountAccessor> &copyFrom );
	CSmartPtr<T,RefCountAccessor> &operator=( CSmartPtr<T,RefCountAccessor> &&moveFrom );
	T*				operator=( T *pObj );
	T&				operator*();
	const T&		operator*() const;
	const T*		operator->() const;
	T*				operator->();
	bool			operator!() const;
	bool			operator==( const T *pOther ) const;

	bool			IsValid() const; // Tells if the pointer is valid.
	T*				GetObject() const; // Get temporary object pointer, don't store it for later reuse!
	T*				SetObject( T* pObj );
	void			MarkDeleted();

private:
	T				*m_pObj;
};

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr() : m_pObj( NULL )
{
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( const T &copyFrom ) : m_pObj( new T() )
{
	CopyConstruct( m_pObj, copyFrom );
	RefCountAccessor::AddRef( m_pObj );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( T &&moveFrom ) : m_pObj( new T() )
{
	MoveConstruct( m_pObj, Move( moveFrom ) );
	RefCountAccessor::AddRef( m_pObj );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( T *pObj ) : m_pObj( NULL )
{
	SetObject( pObj );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( const CSmartPtr<T,RefCountAccessor> &copyFrom ) : m_pObj( NULL )
{
	CopyFrom( copyFrom );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::CSmartPtr( CSmartPtr<T,RefCountAccessor> &&moveFrom ) : m_pObj( NULL )
{
	MoveFrom( Move( moveFrom ) );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor>::~CSmartPtr()
{
	if ( m_pObj )
	{
		RefCountAccessor::Release( m_pObj );
	}
}

template< class T, class RefCountAccessor >
inline void	CSmartPtr<T,RefCountAccessor>::MarkDeleted()
{
	m_pObj = NULL;
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor> &CSmartPtr<T,RefCountAccessor>::operator=( const CSmartPtr<T,RefCountAccessor> &copyFrom )
{
	return CopyFrom( copyFrom );
}

template< class T, class RefCountAccessor >
inline CSmartPtr<T,RefCountAccessor> &CSmartPtr<T,RefCountAccessor>::operator=( CSmartPtr<T,RefCountAccessor> &&moveFrom )
{
	return MoveFrom( Move( moveFrom ) );
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::operator=( T *pObj )
{
	return SetObject( pObj );
}

template< class T, class RefCountAccessor >
inline T& CSmartPtr<T,RefCountAccessor>::operator*()
{
	return *m_pObj;
}

template< class T, class RefCountAccessor >
inline const T& CSmartPtr<T,RefCountAccessor>::operator*() const
{
	return *m_pObj;
}

template< class T, class RefCountAccessor >
inline const T* CSmartPtr<T,RefCountAccessor>::operator->() const
{
	return m_pObj;
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::operator->()
{
	return m_pObj;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::operator!() const
{
	return !m_pObj;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::operator==( const T *pOther ) const
{
	return m_pObj == pOther;
}

template< class T, class RefCountAccessor >
CSmartPtr<T,RefCountAccessor> &CSmartPtr<T,RefCountAccessor>::CopyFrom( const CSmartPtr<T,RefCountAccessor> &copyFrom )
{
	m_pObj = copyFrom.m_pObj;
	return *this;
}

template< class T, class RefCountAccessor >
CSmartPtr<T,RefCountAccessor> &CSmartPtr<T,RefCountAccessor>::MoveFrom( CSmartPtr<T,RefCountAccessor> &&moveFrom )
{
	m_pObj = Move( moveFrom.m_pObj );
	moveFrom.MarkDeleted();
	return *this;
}

template< class T, class RefCountAccessor >
inline bool CSmartPtr<T,RefCountAccessor>::IsValid() const
{
	return m_pObj != NULL;
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::GetObject() const
{
	return m_pObj;
}

template< class T, class RefCountAccessor >
inline T* CSmartPtr<T,RefCountAccessor>::SetObject( T *pObj )
{
	if ( pObj == m_pObj )
		return pObj;

	if ( pObj )
	{
		RefCountAccessor::AddRef( pObj );
	}
	if ( m_pObj )
	{
		RefCountAccessor::Release( m_pObj );
	}
	m_pObj = pObj;
	return pObj;
}


//
// CAutoPushPop
//				allows you to set value of a variable upon construction and destruction.
// Constructors:
//		CAutoPushPop x( myvar )
//			saves the value and restores upon destruction.
//		CAutoPushPop x( myvar, newvalue )
//			saves the value, assigns new value upon construction, restores saved value upon destruction.
//		CAutoPushPop x( myvar, newvalue, restorevalue )
//			assigns new value upon construction, assignes restorevalue upon destruction.
//
template < typename T >
class CAutoPushPop
{
public:
	explicit CAutoPushPop( T& var ) : m_rVar( var ), m_valPop( var ) {}
	CAutoPushPop( T& var, T const &valPush ) : m_rVar( var ), m_valPop( var ) { m_rVar = valPush; }
	CAutoPushPop( T& var, T const &valPush, T const &valPop ) : m_rVar( var ), m_valPop( var ) { m_rVar = valPush; }

	~CAutoPushPop() { m_rVar = m_valPop; }

private:	// forbid copying
	CAutoPushPop( CAutoPushPop const &x );
	CAutoPushPop & operator = ( CAutoPushPop const &x );

public:
	T & Get() { return m_rVar; }

private:
	T &m_rVar;
	T m_valPop;
};


#endif // SMARTPTR_H
