#include "tier1/keyvalues3.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Nasty hack to redefine gcc's offsetof which doesn't like GET_OUTER macro
#ifdef COMPILER_GCC
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

KeyValues3::KeyValues3( KV3TypeEx_t type, KV3SubType_t subtype ) : 
	KeyValues3( KV3_INVALID_CLUSTER_ELEMENT, type, subtype )
{
}

KeyValues3::KeyValues3( int cluster_elem, KV3TypeEx_t type, KV3SubType_t subtype ) : 
	m_bContextIndependent( true ),
	m_TypeEx( type ),
	m_SubType( subtype ),
	m_nFlags( 0 ),
	m_nClusterElement( (uint16)KV3_INVALID_CLUSTER_ELEMENT ),
	m_nNumArrayElements( 0 ),
	m_nReserved( 0 )
{
	SetClusterElement( cluster_elem );
	ResolveUnspecified();
	Alloc();
}

KeyValues3::~KeyValues3() 
{ 
	Free(); 
}

void KeyValues3::CopyFrom( const KeyValues3& other )
{
	if ( this == &other )
		return;

	SetToNull();

	CKeyValues3Context* context;
	KV3MetaData_t* pDestMetaData = GetMetaData( &context );

	if ( pDestMetaData )
	{
		KV3MetaData_t* pSrcMetaData = other.GetMetaData();

		if ( pSrcMetaData )
			context->CopyMetaData( pDestMetaData, pSrcMetaData );
		else
			pDestMetaData->Clear();
	}

	KV3SubType_t eSrcSubType = other.GetSubType();

	switch ( other.GetType() )
	{
		case KV3_TYPE_BOOL:
			SetBool( other.m_Data.m_Bool );
			break;
		case KV3_TYPE_INT:
			SetValue<int64>( other.m_Data.m_Int, KV3_TYPEEX_INT, eSrcSubType );
			break;
		case KV3_TYPE_UINT:
			SetValue<uint64>( other.m_Data.m_UInt, KV3_TYPEEX_UINT, eSrcSubType );
			break;
		case KV3_TYPE_DOUBLE:
			SetValue<float64>( other.m_Data.m_Double, KV3_TYPEEX_DOUBLE, eSrcSubType );
			break;
		case KV3_TYPE_STRING:
			SetString( other.GetString(), eSrcSubType );
			break;
		case KV3_TYPE_BINARY_BLOB:
			SetToBinaryBlob( other.GetBinaryBlob(), other.GetBinaryBlobSize() );
			break;
		case KV3_TYPE_ARRAY:
		{
			switch ( other.GetTypeEx() )
			{
				case KV3_TYPEEX_ARRAY:
				{
					SetToEmptyKV3Array();
					m_Data.m_Array.m_pRoot->CopyFrom( this, other.m_Data.m_Array.m_pRoot );
					break;
				}
				case KV3_TYPEEX_ARRAY_FLOAT32:
					AllocArray<float32>( other.m_nNumArrayElements, other.m_Data.m_Array.m_f32, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_FLOAT32, eSrcSubType, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32 );
					break;
				case KV3_TYPEEX_ARRAY_FLOAT64:
					AllocArray<float64>( other.m_nNumArrayElements, other.m_Data.m_Array.m_f64, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_FLOAT64, eSrcSubType, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT64 );
					break;
				case KV3_TYPEEX_ARRAY_INT16:
					AllocArray<int16>( other.m_nNumArrayElements, other.m_Data.m_Array.m_i16, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_INT16_SHORT, KV3_TYPEEX_ARRAY_INT16, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT16 );
					break;
				case KV3_TYPEEX_ARRAY_INT32:
					AllocArray<int32>( other.m_nNumArrayElements, other.m_Data.m_Array.m_i32, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_INT32, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT32 );
					break;
				case KV3_TYPEEX_ARRAY_UINT8_SHORT:
					AllocArray<uint8>( other.m_nNumArrayElements, other.m_Data.m_Array.m_u8Short, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, eSrcSubType, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
					break;
				case KV3_TYPEEX_ARRAY_INT16_SHORT:
					AllocArray<int16>( other.m_nNumArrayElements, other.m_Data.m_Array.m_i16Short, KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_INT16_SHORT, KV3_TYPEEX_ARRAY_INT16, eSrcSubType, KV3_TYPEEX_INT, KV3_SUBTYPE_INT16 );
					break;
				default:
					break;
			}
			break;
		}
		case KV3_TYPE_TABLE:
		{
			SetToEmptyTable();
			GetTable()->CopyFrom( this, other.GetTable() );
			break;
		}
		default:
			break;
	}

	m_SubType = eSrcSubType;
	m_nFlags = other.m_nFlags;
}

void KeyValues3::OverlayKeysFrom( const KeyValues3 &other, bool depth )
{
	if ( !IsTable() )
		SetToNull();

	const CKeyValues3Table *pOtherTable = other.GetTable();

	if ( !pOtherTable )
		return;

	const auto pHashes = pOtherTable->HashesBase();
	const auto pMembers = pOtherTable->MembersBase();
	const auto pNames = pOtherTable->NamesBase();

	FOR_EACH_KV3_TABLE( *pOtherTable, id )
	{
		KeyValues3 *kv = FindOrCreateMember( CKV3MemberName( pHashes[id], pNames[id] ) );
		KeyValues3 *other_kv = pMembers[id];

		if ( depth && kv->IsTable() && other_kv->IsTable() )
		{
			OverlayKeysFrom( *other_kv, true );
		}
		else
		{
			kv->CopyFrom( *other_kv );
		}
	}
}

KeyValues3& KeyValues3::operator=( const KeyValues3& copyFrom )
{
	if ( this == &copyFrom )
		return *this;

	CopyFrom( copyFrom );

	return *this;
}

void KeyValues3::Alloc( int initial_size, Data_t data, int preallocated_size, bool should_free )
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_ARRAY:
		{
			if ( preallocated_size <= 0 )
			{
				m_Data.m_Array.m_pRoot = AllocArray();
				m_bFreeArrayMemory = true;
			}
			else
			{
				AllocArrayInPlace( initial_size, data, preallocated_size, should_free );
			}

			break;
		}
		case KV3_TYPEEX_TABLE:
		{
			if ( preallocated_size <= 0 )
			{
				m_Data.m_pTable = AllocTable();
				m_bFreeArrayMemory = true;
			}
			else
			{
				AllocTableInPlace( initial_size, data, preallocated_size, should_free );
			}
			
			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT32:
		case KV3_TYPEEX_ARRAY_FLOAT64:
		case KV3_TYPEEX_ARRAY_INT16:
		case KV3_TYPEEX_ARRAY_INT32:
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			m_bFreeArrayMemory = false;
			m_nNumArrayElements = 0;
			m_Data.m_pMemory = nullptr;
			break;
		}
		default: 
			break;
	}
}

void KeyValues3::AllocArrayInPlace( int initial_size, Data_t data, int preallocated_size, bool should_free )
{
	int bytes_needed = MAX( CKeyValues3Array::TotalSizeOf( 0 ), CKeyValues3Array::TotalSizeOf( initial_size ) );

	if ( bytes_needed > preallocated_size )
	{
		Plat_FatalError( "KeyValues3: pre-allocated array memory is too small for %u elements (%u bytes available, %u bytes needed)\n", initial_size, preallocated_size, bytes_needed );
		DebuggerBreak();
	}

	Construct( m_Data.m_Array.m_pRoot, KV3_INVALID_CLUSTER_ELEMENT, initial_size );

	m_Data.m_Array.m_pRoot = data.m_Array.m_pRoot;
	m_bFreeArrayMemory = should_free;
}

void KeyValues3::AllocTableInPlace( int initial_size, Data_t data, int preallocated_size, bool should_free )
{
	int bytes_needed = MAX( CKeyValues3Array::TotalSizeOf( 0 ), CKeyValues3Array::TotalSizeOf( initial_size ) );

	if ( bytes_needed > preallocated_size )
	{
		Plat_FatalError( "KeyValues3: pre-allocated table memory is too small for %u members (%u bytes available, %u bytes needed)\n", initial_size, preallocated_size, bytes_needed );
		DebuggerBreak();
	}

	Construct( data.m_pTable, KV3_INVALID_CLUSTER_ELEMENT, initial_size );

	m_Data.m_pTable = data.m_pTable;
	m_bFreeArrayMemory = should_free;
}

CKeyValues3Array *KeyValues3::AllocArray( int initial_size )
{
	auto context = GetContext();

	if ( context )
	{
		auto arr = context->AllocArray( initial_size );

		if ( arr )
			return arr;
	}

	return AllocateOnHeap<CKeyValues3Array>( initial_size );
}

CKeyValues3Table* KeyValues3::AllocTable( int initial_size )
{
	auto context = GetContext();

	if ( context )
	{
		auto table = context->AllocTable( initial_size );

		if ( table )
			return table;
	}

	return AllocateOnHeap<CKeyValues3Table>( initial_size );
}

void KeyValues3::FreeArray( CKeyValues3Array *element, bool clearing_context )
{
	if ( !element )
		return;

	element->PurgeContent( this, clearing_context );

	if ( !m_bFreeArrayMemory )
	{
		Destruct( element );
	}
	else
	{
		auto context = GetContext();
		bool raw_allocated = context && context->IsArrayAllocated( element );

		if ( !raw_allocated && element->GetClusterElement() < 0 )
		{
			FreeOnHeap( element );
		}
		else if ( !clearing_context )
		{
			if ( !raw_allocated )
				context->FreeArray( element );
			else
				Destruct( element );
		}
	}
}

void KeyValues3::FreeTable( CKeyValues3Table *element, bool clearing_context )
{
	if ( !element )
		return;

	element->PurgeContent( this, clearing_context );

	if ( !m_bFreeArrayMemory )
	{
		Destruct( element );
	}
	else
	{
		auto context = GetContext();
		bool raw_allocated = context && context->IsTableAllocated( element );

		if ( !raw_allocated && element->GetClusterElement() < 0 )
		{
			FreeOnHeap( element );
		}
		else if ( !clearing_context )
		{
			if ( raw_allocated )
				Destruct( element );
			else
				context->FreeTable( element );
		}
	}
}

KeyValues3 *KeyValues3::AllocMember( KV3TypeEx_t type, KV3SubType_t subtype )
{
	auto context = GetContext();

	return context ? context->AllocKV( type, subtype ) : Construct( ::Alloc<KeyValues3>(), type, subtype );
}

void KeyValues3::FreeMember( KeyValues3 *member )
{
	auto context = GetContext();

	if ( context )
	{
		auto cluster = member->GetCluster();

		cluster->PurgeMetaData( cluster->GetNodeIndex( member ) );
		context->FreeKV( member );
	}
	else
	{
		Destruct( member );
	}
}

void KeyValues3::Free( bool bClearingContext )
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_STRING:
		{
			free( (void*)m_Data.m_pString );
			m_Data.m_pString = nullptr;
			break;
		}
		case KV3_TYPEEX_BINARY_BLOB:
		{
			if ( m_Data.m_pBinaryBlob )
				free( m_Data.m_pBinaryBlob );
			m_Data.m_pBinaryBlob = nullptr;
			break;
		}
		case KV3_TYPEEX_BINARY_BLOB_EXTERN:
		{
			if ( m_Data.m_pBinaryBlob )
			{
				if ( m_Data.m_pBinaryBlob->m_bFreeMemory )
					free( (void*)m_Data.m_pBinaryBlob->m_pubData );
				free( m_Data.m_pBinaryBlob );
			}
			m_Data.m_pBinaryBlob = nullptr;
			break;
		}
		case KV3_TYPEEX_ARRAY:
		{
			FreeArray( m_Data.m_Array.m_pRoot, bClearingContext );

			m_bFreeArrayMemory = false;
			m_Data.m_Array.m_pRoot = nullptr;

			break;
		}
		case KV3_TYPEEX_TABLE:
		{
			FreeTable( m_Data.m_pTable, bClearingContext );

			m_bFreeArrayMemory = false;
			m_Data.m_pTable = nullptr;

			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT32:
		case KV3_TYPEEX_ARRAY_FLOAT64:
		case KV3_TYPEEX_ARRAY_INT16:
		case KV3_TYPEEX_ARRAY_INT32:
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			if ( m_bFreeArrayMemory )
				free( m_Data.m_pMemory );
			m_bFreeArrayMemory = false;
			m_nNumArrayElements = 0;
			m_Data.m_nMemory = 0;
			break;
		}
		default: 
			break;
	}
}

void KeyValues3::ResolveUnspecified()
{
	if ( GetSubType() == KV3_SUBTYPE_UNSPECIFIED )
	{
		switch ( GetType() )
		{
			case KV3_TYPE_NULL:
				m_SubType = KV3_SUBTYPE_NULL;
				break;
			case KV3_TYPE_BOOL:
				m_SubType = KV3_SUBTYPE_BOOL8;
				break;
			case KV3_TYPE_INT:
				m_SubType = KV3_SUBTYPE_INT64;
				break;
			case KV3_TYPE_UINT:
				m_SubType = KV3_SUBTYPE_UINT64;
				break;
			case KV3_TYPE_DOUBLE:
				m_SubType = KV3_SUBTYPE_FLOAT64;
				break;
			case KV3_TYPE_STRING:
				m_SubType = KV3_SUBTYPE_STRING;
				break;
			case KV3_TYPE_BINARY_BLOB:
				m_SubType = KV3_SUBTYPE_BINARY_BLOB;
				break;
			case KV3_TYPE_ARRAY:
				m_SubType = KV3_SUBTYPE_ARRAY;
				break;
			case KV3_TYPE_TABLE:
				m_SubType = KV3_SUBTYPE_TABLE;
				break;
			default:
				m_SubType = KV3_SUBTYPE_INVALID;
				break;
		}
	}
}

void KeyValues3::PrepareForType( KV3TypeEx_t type, KV3SubType_t subtype, int initial_size, Data_t data, int bytes_available, bool should_free )
{
	if ( GetTypeEx() == type )
	{
		switch ( type )
		{
			case KV3_TYPEEX_STRING:
			case KV3_TYPEEX_BINARY_BLOB:
			case KV3_TYPEEX_BINARY_BLOB_EXTERN:
			case KV3_TYPEEX_ARRAY_FLOAT32:
			case KV3_TYPEEX_ARRAY_FLOAT64:
			case KV3_TYPEEX_ARRAY_INT16:
			case KV3_TYPEEX_ARRAY_INT32:
			case KV3_TYPEEX_ARRAY_UINT8_SHORT:
			case KV3_TYPEEX_ARRAY_INT16_SHORT:
			{
				Free();
				break;
			}
			default: 
				break;
		}
	}
	else
	{
		Free();
		m_TypeEx = type;
		Alloc( initial_size, data, bytes_available, should_free );
	}

	m_SubType = subtype;
}

CKeyValues3Cluster* KeyValues3::GetCluster() const
{
	if ( m_bContextIndependent )
		return nullptr;

	return GET_OUTER( CKeyValues3Cluster, m_Values[ m_nClusterElement ] );
}

CKeyValues3Context* KeyValues3::GetContext() const
{ 
	CKeyValues3Cluster* cluster = GetCluster();

	if ( !cluster )
		return nullptr;

	return cluster->GetContext();
}

KV3MetaData_t* KeyValues3::GetMetaData( CKeyValues3Context** ppCtx ) const
{
	CKeyValues3Cluster* cluster = GetCluster();

	if ( cluster )
	{
		if ( ppCtx )
			*ppCtx = cluster->GetContext();

		return cluster->GetMetaData( m_nClusterElement );
	}
	else
	{
		if ( ppCtx )
			*ppCtx = nullptr;

		return nullptr;
	}
}

const char* KeyValues3::GetString( const char* defaultValue ) const
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_STRING:
		case KV3_TYPEEX_STRING_EXTERN:
			return m_Data.m_pString;
		case KV3_TYPEEX_STRING_SHORT:
			return m_Data.m_szStringShort;
		default:
			return defaultValue;
	}
}

void KeyValues3::SetString( const char* pString, KV3SubType_t subtype )
{
	if ( !pString )
		pString = "";

	if ( strlen( pString ) < sizeof( m_Data.m_szStringShort ) )
	{
		PrepareForType( KV3_TYPEEX_STRING_SHORT, subtype );
		V_strncpy( m_Data.m_szStringShort, pString, sizeof( m_Data.m_szStringShort ) );
	}
	else
	{
		PrepareForType( KV3_TYPEEX_STRING, subtype );
		m_Data.m_pString = strdup( pString );
	}
}

void KeyValues3::SetStringExternal( const char* pString, KV3SubType_t subtype )
{
	if ( strlen( pString ) < sizeof( m_Data.m_szStringShort ) )
	{
		PrepareForType( KV3_TYPEEX_STRING_SHORT, subtype );
		V_strncpy( m_Data.m_szStringShort, pString, sizeof( m_Data.m_szStringShort ) );
	}
	else
	{
		PrepareForType( KV3_TYPEEX_STRING_EXTERN, subtype );
		m_Data.m_pString = pString;
	}
}

const byte* KeyValues3::GetBinaryBlob() const
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_BINARY_BLOB:
			return m_Data.m_pBinaryBlob ? m_Data.m_pBinaryBlob->m_ubData : nullptr;
		case KV3_TYPEEX_BINARY_BLOB_EXTERN:
			return m_Data.m_pBinaryBlob ? m_Data.m_pBinaryBlob->m_pubData : nullptr;
		default:
			return nullptr;
	}
}

int KeyValues3::GetBinaryBlobSize() const
{
	if ( GetType() != KV3_TYPE_BINARY_BLOB || !m_Data.m_pBinaryBlob )
		return 0;

	return ( int )m_Data.m_pBinaryBlob->m_nSize;
}

void KeyValues3::SetToBinaryBlob( const byte* blob, int size )
{
	PrepareForType( KV3_TYPEEX_BINARY_BLOB, KV3_SUBTYPE_BINARY_BLOB );

	if ( size > 0 )
	{
		m_Data.m_pBinaryBlob = (KV3BinaryBlob_t*)malloc( sizeof( size_t ) + size );
		m_Data.m_pBinaryBlob->m_nSize = size;
		memcpy( m_Data.m_pBinaryBlob->m_ubData, blob, size );
	}
	else
	{
		m_Data.m_pBinaryBlob = nullptr;
	}
}

void KeyValues3::SetToBinaryBlobExternal( const byte* blob, int size, bool free_mem )
{
	PrepareForType( KV3_TYPEEX_BINARY_BLOB_EXTERN, KV3_SUBTYPE_BINARY_BLOB );

	if ( size > 0 )
	{
		m_Data.m_pBinaryBlob = (KV3BinaryBlob_t*)malloc( sizeof( KV3BinaryBlob_t ) );
		m_Data.m_pBinaryBlob->m_nSize = size;
		m_Data.m_pBinaryBlob->m_pubData = blob;
		m_Data.m_pBinaryBlob->m_bFreeMemory = free_mem;
	}
	else
	{
		m_Data.m_pBinaryBlob = nullptr;
	}
}

Color KeyValues3::GetColor( const Color &defaultValue ) const
{
	int32 color[4];
	if ( ReadArrayInt32( 4, color ) )
	{
		return Color( color[0], color[1], color[2], color[3] );
	}
	else if ( ReadArrayInt32( 3, color ) )
	{
		return Color( color[0], color[1], color[2], 255 );
	}
	else
	{
		return defaultValue;
	}
}

void KeyValues3::SetColor( const Color &color )
{
	if ( color.a() == 255 )
		AllocArray<uint8>( 3, &color[0], KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, KV3_SUBTYPE_COLOR32, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
	else
		AllocArray<uint8>( 4, &color[0], KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_ARRAY_UINT8_SHORT, KV3_TYPEEX_INVALID, KV3_SUBTYPE_COLOR32, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 );
}

int KeyValues3::GetArrayElementCount() const
{
	if ( IsArray() )
	{
		const CKeyValues3Array *pArray = GetKV3Array();

		if ( !pArray )
			return m_nNumArrayElements;

		return pArray->Count();
	}

	return -1;
}

KeyValues3** KeyValues3::GetArrayBase()
{
	CKeyValues3Array *pArray = GetKV3Array();

	if ( !pArray )
		return nullptr;

	return pArray->Base();
}

KeyValues3* KeyValues3::GetArrayElement( int elem )
{
	CKeyValues3Array *pArray = GetKV3Array();

	if ( !pArray || elem < 0 || elem >= pArray->Count() )
		return nullptr;

	return pArray->Element( elem );
}

KeyValues3* KeyValues3::ArrayInsertElementBefore( int elem )
{
	if ( !IsKV3Array() )
		SetToEmptyKV3Array();

	return *GetKV3Array()->InsertMultipleBefore( this, elem, 1 );
}

KeyValues3* KeyValues3::ArrayAddElementToTail()
{
	if ( !IsArray() )
		SetToEmptyKV3Array();

	CKeyValues3Array *pArray = GetKV3Array();

	return *pArray->InsertMultipleBefore( this, pArray->Count(), 1 );
}

void KeyValues3::ArraySwapItems( int idx1, int idx2 )
{
	CKeyValues3Array *pArray = GetKV3Array();

	if ( !pArray )
		return;

	if ( idx1 < 0 || idx1 >= pArray->Count() )
		return;

	if ( idx2 < 0 || idx2 >= pArray->Count() )
		return;

	auto base = pArray->Base();

	auto temp = base[idx1];
	base[idx1] = base[idx2];
	base[idx2] = temp;
}

void KeyValues3::SetArrayElementCount( int count, KV3TypeEx_t type, KV3SubType_t subtype )
{
	if ( !IsKV3Array() )
		SetToEmptyKV3Array();

	GetKV3Array()->SetCount( this, count, type, subtype );
}

void KeyValues3::ArrayRemoveElements( int elem, int num )
{
	CKeyValues3Array *pArray = GetKV3Array();

	if ( !pArray )
		return;

	pArray->RemoveMultiple( this, elem, num );
}

void KeyValues3::NormalizeArray()
{
	switch ( GetTypeEx() )
	{
		case KV3_TYPEEX_ARRAY_FLOAT32:
		{
			NormalizeArray<float32>( KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32, m_nNumArrayElements, m_Data.m_Array.m_f32, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_FLOAT64:
		{
			NormalizeArray<float64>( KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT64, m_nNumArrayElements, m_Data.m_Array.m_f64, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT16:
		{
			NormalizeArray<int16>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT16, m_nNumArrayElements, m_Data.m_Array.m_i16, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT32:
		{
			NormalizeArray<int32>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT32, m_nNumArrayElements, m_Data.m_Array.m_i32, m_bFreeArrayMemory );
			break;
		}
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		{
			uint8 u8ArrayShort[8];
			memcpy( u8ArrayShort, m_Data.m_Array.m_u8Short, sizeof( u8ArrayShort ) );
			NormalizeArray<uint8>( KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8, m_nNumArrayElements, u8ArrayShort, false );
			break;
		}
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
		{
			int16 i16ArrayShort[4];
			memcpy( i16ArrayShort, m_Data.m_Array.m_u8Short, sizeof( i16ArrayShort ) );
			NormalizeArray<int16>( KV3_TYPEEX_INT, KV3_SUBTYPE_INT16, m_nNumArrayElements, i16ArrayShort, false );
			break;
		}
		default: 
			break;
	}
}

bool KeyValues3::ReadArrayInt32( int dest_size, int32* data ) const
{
	int src_size = 0;

	if ( IsString() )
	{
		CSplitString values( GetString(), " " );
		src_size = values.Count();
		int count = MIN( src_size, dest_size );
		for ( int i = 0; i < count; ++i )
			data[ i ] = V_StringToInt32( values[ i ], 0, NULL, NULL, PARSING_FLAG_SKIP_ASSERT );
	}
	else
	{
		switch ( GetTypeEx() )
		{
			case KV3_TYPEEX_ARRAY:
			{
				CKeyValues3Array *pArray = m_Data.m_Array.m_pRoot;

				src_size = pArray->Count();
				int count = MIN( src_size, dest_size );
				KeyValues3** arr = pArray->Base();
				for ( int i = 0; i < count; ++i )
					data[ i ] = arr[ i ]->GetInt();
				break;
			}
			case KV3_TYPEEX_ARRAY_INT16:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_Data.m_Array.m_u8Short[ i ];
				break;
			}
			case KV3_TYPEEX_ARRAY_INT32:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				memcpy( data, m_Data.m_Array.m_i32, count * sizeof( int32 ) );
				break;
			}
			case KV3_TYPEEX_ARRAY_UINT8_SHORT:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_Data.m_Array.m_u8Short[ i ];
				break;
			}
			case KV3_TYPEEX_ARRAY_INT16_SHORT:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( int32 )m_Data.m_Array.m_u8Short[ i ];
				break;
			}
			default: 
				break;
		}
	}

	if ( src_size < dest_size )
		memset( &data[ src_size ], 0, ( dest_size - src_size ) * sizeof( int32 ) ); 

	return ( src_size == dest_size );
}

bool KeyValues3::ReadArrayFloat32( int dest_size, float32* data ) const
{
	int src_size = 0;

	if ( IsString() )
	{
		CSplitString values( GetString(), " " );
		src_size = values.Count();
		int count = MIN( src_size, dest_size );
		for ( int i = 0; i < count; ++i )
			data[ i ] = ( float32 )V_StringToFloat64( values[ i ], 0, NULL, NULL, PARSING_FLAG_SKIP_ASSERT );
	}
	else
	{
		switch ( GetTypeEx() )
		{
			case KV3_TYPEEX_ARRAY:
			{
				CKeyValues3Array *pArray = m_Data.m_Array.m_pRoot;

				src_size = pArray->Count();
				int count = MIN( src_size, dest_size );
				KeyValues3** arr = pArray->Base();
				for ( int i = 0; i < count; ++i )
					data[ i ] = arr[ i ]->GetFloat();
				break;
			}
			case KV3_TYPEEX_ARRAY_FLOAT32:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				memcpy( data, m_Data.m_Array.m_f32, count * sizeof( float32 ) );
				break;
			}
			case KV3_TYPEEX_ARRAY_FLOAT64:
			{
				src_size = m_nNumArrayElements;
				int count = MIN( src_size, dest_size );
				for ( int i = 0; i < count; ++i )
					data[ i ] = ( float32 )m_Data.m_Array.m_f64[ i ];
				break;
			}
			default:
				break;
		}
	}

	if ( src_size < dest_size )
		memset( &data[ src_size ], 0, ( dest_size - src_size ) * sizeof( float32 ) ); 

	return ( src_size == dest_size );
}

void KeyValues3::SetToEmptyTable()
{
	PrepareForType( KV3_TYPEEX_TABLE, KV3_SUBTYPE_TABLE );
	GetTable()->RemoveAll( this );
}

int KeyValues3::GetMemberCount() const
{
	const CKeyValues3Table *pTable = GetTable();

	if ( pTable )
		return pTable->GetMemberCount();

	return KV3_INVALID_MEMBER;
}

KeyValues3* KeyValues3::GetMember( KV3MemberId_t id )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable || id < 0 || id >= pTable->GetMemberCount() )
		return nullptr;
	
	return pTable->GetMember( id );
}

const char* KeyValues3::GetMemberName( KV3MemberId_t id ) const
{
	const CKeyValues3Table *pTable = GetTable();

	if ( !pTable || id < 0 || id >= pTable->GetMemberCount() )
		return nullptr;
	
	return pTable->GetMemberName( id );
}

CKV3MemberName KeyValues3::GetKV3MemberName( KV3MemberId_t id ) const
{
	const CKeyValues3Table *pTable = GetTable();

	if ( !pTable || id < 0 || id >= pTable->GetMemberCount() )
		return CKV3MemberName();

	return CKV3MemberName( pTable->GetMemberHash( id ), pTable->GetMemberName( id ) );
}

CKV3MemberHash KeyValues3::GetMemberHash( KV3MemberId_t id ) const
{
	const CKeyValues3Table *pTable = GetTable();

	if ( !pTable || id < 0 || id >= pTable->GetMemberCount() )
		return CKV3MemberHash();
	
	return pTable->GetMemberHash( id );
}

KeyValues3* KeyValues3::Internal_FindMember( const CKV3MemberName &name, KV3MemberId_t &next, KeyValues3* defaultValue )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable )
		return defaultValue;

	KV3MemberId_t id = pTable->Internal_FindMember( name, next );

	if ( id == KV3_INVALID_MEMBER )
		return defaultValue;

	return pTable->GetMember( id );
}

KeyValues3* KeyValues3::FindOrCreateMember( const CKV3MemberName &name, bool *pCreated )
{
	if ( !IsTable() )
		SetToEmptyTable();

	CKeyValues3Table *pTable = GetTable();

	KV3MemberId_t id = pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
	{
		if ( pCreated )
			*pCreated = true;

		id = pTable->CreateMember( this, name );
	}
	else
	{
		if ( pCreated )
			*pCreated = false;
	}

	return pTable->GetMember( id );
}

CKV3MemberHash KeyValues3::RenameMember( const CKV3MemberName &name, const CKV3MemberName &newName )
{
	CKeyValues3Table *pTable = GetTable();

	KV3MemberId_t id = pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
		return CKV3MemberHash();

	pTable->RenameMember( this, id, newName );

	return pTable->GetMemberHash( id );
}

bool KeyValues3::RemoveMember( KV3MemberId_t id )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable || id < 0 || id >= pTable->GetMemberCount() )
		return false;

	pTable->RemoveMember( this, id );

	return true;
}

bool KeyValues3::RemoveMember( const KeyValues3* kv )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable )
		return false;

	KV3MemberId_t id = pTable->FindMember( kv );

	if ( id == KV3_INVALID_MEMBER )
		return false;

	pTable->RemoveMember( this, id );

	return true;
}

bool KeyValues3::RemoveMember( const CKV3MemberName &name )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable )
		return false;

	KV3MemberId_t id = pTable->FindMember( name );

	if ( id == KV3_INVALID_MEMBER )
		return false;

	pTable->RemoveMember( this, id );

	return true;
}

bool KeyValues3::HasInvalidMemberNames() const
{
	const CKeyValues3Table *pTable = GetTable();

	if ( !pTable )
		return false;

	return pTable->HasInvalidMemberNames();
}

void KeyValues3::SetHasInvalidMemberNames( bool bValue )
{
	CKeyValues3Table *pTable = GetTable();

	if ( !pTable )
		return;

	pTable->SetHasInvalidMemberNames( bValue );
}

const char* KeyValues3::GetTypeAsString() const
{
	static const char* s_Types[] =
	{
		"invalid",
		"null",
		"bool",
		"int",
		"uint",
		"double",
		"string",
		"binary_blob",
		"array",
		"table",
		nullptr
	};

	KV3Type_t type = GetType();

	if ( type < KV3_TYPE_COUNT )
		return s_Types[type];

	return "<unknown>";
}

const char* KeyValues3::GetSubTypeAsString() const
{
	static const char* s_SubTypes[] =
	{
		"invalid",
		"resource",
		"resource_name",
		"panorama",
		"soundevent",
		"subclass",
		"entity_name",
		"localize",
		"unspecified",
		"null",
		"binary_blob",
		"array",
		"table",
		"bool8",
		"char8",
		"uchar32",
		"int8",
		"uint8",
		"int16",
		"uint16",
		"int32",
		"uint32",
		"int64",
		"uint64",
		"float32",
		"float64",
		"string",
		"pointer",
		"color32",
		"vector",
		"vector2d",
		"vector4d",
		"rotation_vector",
		"quaternion",
		"qangle",
		"matrix3x4",
		"transform",
		"string_token",
		"ehandle",
		nullptr
	};

	KV3SubType_t subtype = GetSubType();

	if ( subtype < KV3_SUBTYPE_COUNT )
		return s_SubTypes[subtype];

	return "<unknown>";
}

const char* KeyValues3::ToString( CBufferString& buff, uint flags ) const
{
	if ( ( flags & KV3_TO_STRING_DONT_CLEAR_BUFF ) != 0 )
		flags &= ~KV3_TO_STRING_DONT_APPEND_STRINGS;
	else
		buff.Clear();

	KV3Type_t type = GetType();

	switch ( type )
	{
		case KV3_TYPE_NULL:
		{
			return buff.Get();
		}
		case KV3_TYPE_BOOL:
		{
			const char* str = m_Data.m_Bool ? "true" : "false";

			if ( ( flags & KV3_TO_STRING_DONT_APPEND_STRINGS ) != 0 )
				return str;

			buff.Insert( buff.Length(), str );
			return buff.Get();
		}
		case KV3_TYPE_INT:
		{
			buff.AppendFormat( "%lld", m_Data.m_Int );
			return buff.Get();
		}
		case KV3_TYPE_UINT:
		{
			if ( GetSubType() == KV3_SUBTYPE_POINTER )
			{
				if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
					buff.Insert( buff.Length(), "<pointer>" );

				if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
					return nullptr;

				return buff.Get();
			}
			
			buff.AppendFormat( "%llu", m_Data.m_UInt );
			return buff.Get();
		}
		case KV3_TYPE_DOUBLE:
		{
			buff.AppendFormat( "%g", m_Data.m_Double );
			return buff.Get();
		}
		case KV3_TYPE_STRING:
		{
			const char* str = GetString();

			if ( ( flags & KV3_TO_STRING_DONT_APPEND_STRINGS ) != 0 )
				return str;

			buff.Insert( buff.Length(), str );
			return buff.Get();
		}
		case KV3_TYPE_BINARY_BLOB:
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<binary blob: %u bytes>", GetBinaryBlobSize() );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return nullptr;

			return buff.Get();
		}
		case KV3_TYPE_ARRAY:
		{
			int elements = GetArrayElementCount();

			if ( elements > 0 && elements <= 4 )
			{
				switch ( GetTypeEx() )
				{
					case KV3_TYPEEX_ARRAY:
					{
						bool unprintable = false;
						CBufferStringN<128> temp;

						CKeyValues3Array::Element_t* arr = m_Data.m_Array.m_pRoot->Base();
						for ( int i = 0; i < elements; ++i )
						{
							switch ( arr[i]->GetType() )
							{
								case KV3_TYPE_INT:
									temp.AppendFormat( "%lld", arr[i]->m_Data.m_Int );
									break;
								case KV3_TYPE_UINT:
									if ( arr[i]->GetSubType() == KV3_SUBTYPE_POINTER )
										unprintable = true;
									else
										temp.AppendFormat( "%llu", arr[i]->m_Data.m_UInt );
									break;
								case KV3_TYPE_DOUBLE:
									temp.AppendFormat( "%g", arr[i]->m_Data.m_Double );
									break;
								default:
									unprintable = true;
									break;
							}

							if ( unprintable )
								break;

							if ( i != elements - 1 ) temp.Insert( temp.Length(), " " );
						}

						if ( unprintable )
							break;

						buff.Insert( buff.Length(), temp.Get() );
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_FLOAT32:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%g", m_Data.m_Array.m_f32[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_FLOAT64:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%g", m_Data.m_Array.m_f64[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_INT16:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_Data.m_Array.m_i16Short[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_INT32:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_Data.m_Array.m_i32[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_UINT8_SHORT:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%u", m_Data.m_Array.m_u8Short[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					case KV3_TYPEEX_ARRAY_INT16_SHORT:
					{
						for ( int i = 0; i < elements; ++i )
						{
							buff.AppendFormat( "%d", m_Data.m_Array.m_i16Short[i] );
							if ( i != elements - 1 ) buff.Insert( buff.Length(), " " );
						}
						return buff.Get();
					}
					default:
						break;
				}
			}

			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<array: %u elements>", elements );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return nullptr;

			return buff.Get();
		}
		case KV3_TYPE_TABLE:
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<table: %u members>", GetMemberCount() );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return nullptr;

			return buff.Get();
		}
		default: 
		{
			if ( ( flags & KV3_TO_STRING_APPEND_ONLY_NUMERICS ) == 0 )
				buff.AppendFormat( "<unknown KV3 basic type '%s' (%d)>", GetTypeAsString(), type );

			if ( ( flags & KV3_TO_STRING_RETURN_NON_NUMERICS ) == 0 )
				return nullptr;

			return buff.Get();
		}
	}
}

void CKeyValues3Iterator::Init( KeyValues3 *kv )
{
	m_Stack.Purge();

	if ( kv )
	{
		auto entry = m_Stack.AddToTailGetPtr();
		entry->m_nIndex = -1;
		entry->m_pKV = kv;

		Advance();
	}
}

void CKeyValues3Iterator::Advance()
{
	while ( m_Stack.Count() > 0 )
	{
		auto &entry = m_Stack[m_Stack.Count() - 1];
		auto kv = entry.m_pKV;

		if ( kv->GetType() == KV3_TYPE_ARRAY )
		{
			entry.m_nIndex++;

			if ( entry.m_nIndex < kv->GetArrayElementCount() )
			{
				auto new_entry = m_Stack.AddToTailGetPtr();
				new_entry->m_nIndex = -1;
				new_entry->m_pKV = kv->GetArrayElement( entry.m_nIndex );
				return;
			}
		}
		else if ( kv->GetType() == KV3_TYPE_TABLE )
		{
			entry.m_nIndex++;

			if ( entry.m_nIndex < kv->GetMemberCount() )
			{
				auto new_entry = m_Stack.AddToTailGetPtr();
				new_entry->m_nIndex = -1;
				new_entry->m_pKV = kv->GetMember( entry.m_nIndex );
				return;
			}
		}

		m_Stack.RemoveMultipleFromTail( 1 );
	}
}

CKeyValues3Array::CKeyValues3Array( int cluster_elem, int alloc_size ) :
	m_nClusterElement( cluster_elem ),
	m_nAllocatedChunks( alloc_size ),
	m_nCount( 0 ),
	m_nInitialSize( MIN( alloc_size, 255 ) ),
	m_bIsDynamicallySized( false ),
	m_unk001( false ),
	m_unk002( false ),
	m_pDynamicElements( nullptr )
{
}

CKeyValues3ArrayCluster* CKeyValues3Array::GetCluster() const
{
	if ( !HasCluster() )
		return nullptr;

	return GET_OUTER( CKeyValues3ArrayCluster, m_Values[ m_nClusterElement ] );
}

CKeyValues3Context* CKeyValues3Array::GetContext() const
{ 
	CKeyValues3ArrayCluster* cluster = GetCluster();

	if ( !cluster )
		return nullptr;

	return cluster->GetContext();
}

KeyValues3* CKeyValues3Array::Element( int i )
{
	Assert( 0 <= i && i < m_nCount );

	return Base()[i];
}

void CKeyValues3Array::EnsureElementCapacity( int count, bool force, bool dont_move )
{
	if ( count <= m_nAllocatedChunks )
		return;

	if ( count > ALLOC_KV3ARRAY_MAX )
	{
		Plat_FatalError( "%s: element count overflow (%u)\n", __FUNCTION__, count );
		DebuggerBreak();
	}

	const int new_count = force ? count : KV3Helpers::CalcNewBufferSize( m_nAllocatedChunks, count, ALLOC_KV3ARRAY_MIN, ALLOC_KV3ARRAY_MAX );
	const int new_byte_size = TotalSizeOfData( new_count );

	Element_t *new_base = nullptr;

	if ( m_bIsDynamicallySized )
	{
		new_base = (Element_t *)realloc( m_pDynamicElements, new_byte_size );
	}
	else
	{
		new_base = (Element_t *)malloc( new_byte_size );

		if ( m_nCount > 0 && !dont_move )
		{
			memmove( new_base, Base(), sizeof( Element_t ) * m_nCount );
		}
	}

	m_pDynamicElements = new_base;
	m_nAllocatedChunks = new_count;
	m_bIsDynamicallySized = true;
}

void CKeyValues3Array::SetCount( KeyValues3 *parent, int count, KV3TypeEx_t type, KV3SubType_t subtype )
{
	Element_t *elements_base = Base();

	for ( int i = count; i < m_nCount; i++ )
	{
		parent->FreeMember( elements_base[i] );
	}

	EnsureElementCapacity( count );

	elements_base = Base();
	for ( int i = m_nCount; i < count; i++ )
	{
		elements_base[i] = parent->AllocMember( type, subtype );
	}

	m_nCount = count;
}

CKeyValues3Array::Element_t* CKeyValues3Array::InsertMultipleBefore( KeyValues3 *parent, int from, int num )
{
	if ( from < 0 || from > m_nCount )
	{
		Plat_FatalError( "%s: invalid insert point %u (current count %u)\n", __FUNCTION__, from, m_nCount );
		DebuggerBreak();
	}

	if ( num > ALLOC_KV3ARRAY_MAX - m_nCount )
	{
		Plat_FatalError( "%s: max element overflow, cur count %u + %u\n", __FUNCTION__, m_nCount, num );
		DebuggerBreak();
	}

	int new_size = m_nCount + num;

	EnsureElementCapacity( new_size );

	Element_t *base = Base();

	if ( from < m_nCount )
	{
		memmove( (void *)base[from + num], (void *)base[from], sizeof(Element_t) * (m_nCount - from) );
	}

	for ( int i = 0; i < num; ++i )
	{
		base[from + i] = parent->AllocMember();
	}

	m_nCount = new_size;

	return &base[from];
}

void CKeyValues3Array::CopyFrom( KeyValues3 *parent, const CKeyValues3Array* pSrc )
{
	int nNewSize = pSrc->Count();

	SetCount( parent, nNewSize );

	Element_t *base = Base();
	Element_t const *pSrcKV = pSrc->Base();

	for ( int i = 0; i < nNewSize; ++i )
		*base[i] = *pSrcKV[i];
}

void CKeyValues3Array::RemoveMultiple( KeyValues3 *parent, int from, int num )
{
	Element_t *base = Base();

	for ( int i = 0; i <= num; ++i )
	{
		parent->FreeMember( base[from + i] );
	}

	m_nCount -= num;
}

void CKeyValues3Array::PurgeBuffers()
{
	if ( m_bIsDynamicallySized )
	{
		free( m_pDynamicElements );
		m_nAllocatedChunks = m_nInitialSize;
		m_bIsDynamicallySized = false;
	}

	m_nCount = 0;
}

void CKeyValues3Array::PurgeContent( KeyValues3 *parent, bool clearing_context )
{
	if ( !clearing_context && parent )
	{
		auto elements_base = Base();

		for(int i = 0; i < m_nCount; i++)
		{
			parent->FreeMember( elements_base[i] );
		}
	}
}

CKeyValues3Table::CKeyValues3Table( int cluster_elem, int alloc_size ) :
	m_nClusterElement( cluster_elem ),
	m_nAllocatedChunks( alloc_size ),
	m_pFastSearch( nullptr ),
	m_nCount( 0 ),
	m_nInitialSize( MIN( alloc_size, 255 ) ),
	m_bIsDynamicallySized( false ),
	m_bHasInvalidMemberNames( false ),
	m_unk002( false ),
	m_pDynamicBuffer( nullptr )
{
}

CKeyValues3TableCluster* CKeyValues3Table::GetCluster() const
{
	if ( !HasCluster() )
		return nullptr;

	return GET_OUTER( CKeyValues3TableCluster, m_Values[ m_nClusterElement ] );
}

CKeyValues3Context* CKeyValues3Table::GetContext() const
{ 
	CKeyValues3TableCluster* cluster = GetCluster();

	if ( !cluster )
		return nullptr;

	return cluster->GetContext();
}

KeyValues3* CKeyValues3Table::GetMember( KV3MemberId_t id )
{
	Assert( 0 <= id && id < m_nCount );

	return MembersBase()[id];
}

const CKeyValues3Table::Name_t CKeyValues3Table::GetMemberName( KV3MemberId_t id ) const
{
	Assert( 0 <= id && id < m_nCount );

	return NamesBase()[id];
}

const CKeyValues3Table::Hash_t CKeyValues3Table::GetMemberHash( KV3MemberId_t id ) const
{
	Assert( 0 <= id && id < m_nCount );

	return HashesBase()[id];
}

void CKeyValues3Table::EnableFastSearch()
{
	if ( m_pFastSearch )
		m_pFastSearch->m_member_ids.RemoveAll();
	else
		Construct( m_pFastSearch = ::Alloc<kv3tablefastsearch_t>() );

	const Hash_t* pHashes = HashesBase();

	for ( int i = 0; i < m_nCount; ++i )
	{
		m_pFastSearch->m_member_ids.Insert( pHashes[i], i );
	}

	m_pFastSearch->m_ignore = false;
	m_pFastSearch->m_ignores_counter = 0;
}

void CKeyValues3Table::EnsureMemberCapacity( int count, bool force, bool dont_move )
{
	if ( count <= m_nAllocatedChunks )
		return;

	if ( count > ALLOC_KV3TABLE_MAX )
	{
		Plat_FatalError( "%s member count overflow (%u)\n", __FUNCTION__, count );
		DebuggerBreak();
	}

	const int new_count = force ? count : KV3Helpers::CalcNewBufferSize( m_nAllocatedChunks, count, ALLOC_KV3TABLE_MIN, ALLOC_KV3TABLE_MAX );
	const int new_byte_size = TotalSizeOfData( new_count );

	void *new_base = nullptr;

	if ( m_bIsDynamicallySized )
	{
		new_base = realloc( m_pDynamicBuffer, new_byte_size );

		memmove( (uint8 *)new_base + OffsetToFlagsBase( new_count ), FlagsBase(), m_nCount * sizeof( Flags_t ) );
		memmove( (uint8 *)new_base + OffsetToNamesBase( new_count ), NamesBase(), m_nCount * sizeof( Name_t ) );
		memmove( (uint8 *)new_base + OffsetToMembersBase( new_count ), MembersBase(), m_nCount * sizeof( Member_t ) );
	}
	else
	{
		new_base = malloc( new_byte_size );

		if ( m_nCount > 0 && !dont_move )
		{
			memmove( (uint8 *)new_base + OffsetToHashesBase( new_count ), HashesBase(), m_nCount * sizeof( Hash_t ) );
			memmove( (uint8 *)new_base + OffsetToMembersBase( new_count ), MembersBase(), m_nCount * sizeof( Member_t ) );
			memmove( (uint8 *)new_base + OffsetToNamesBase( new_count ), NamesBase(), m_nCount * sizeof( Name_t ) );
			memmove( (uint8 *)new_base + OffsetToFlagsBase( new_count ), FlagsBase(), m_nCount * sizeof( Flags_t ) );
		}
	}

	m_pDynamicBuffer = new_base;
	m_nAllocatedChunks = new_count;
	m_bIsDynamicallySized = true;
}

KV3MemberId_t CKeyValues3Table::Internal_FindMember( const CKV3MemberName &name, KV3MemberId_t &next )
{
	bool bFastSearch = false;

	if ( m_pFastSearch )
	{
		if ( m_pFastSearch->m_ignore )
		{
			if ( ++m_pFastSearch->m_ignores_counter > 4 )
			{
				EnableFastSearch();
				bFastSearch = true;
			}
		}
		else
			bFastSearch = true;
	}

	if ( bFastSearch )
	{
		UtlHashHandle_t h = m_pFastSearch->m_member_ids.Find( name );

		if ( h != m_pFastSearch->m_member_ids.InvalidHandle() )
		{
			KV3MemberId_t res = m_pFastSearch->m_member_ids[ h ];

			next = res + 1;

			return res;
		}
	}
	else
	{
		const Hash_t* pHashes = HashesBase();

		for ( KV3MemberId_t i = 0; i < m_nCount; ++i )
			if ( pHashes[i] == name )
			{
				next = i + 1;

				return i;
			}
	}

	return KV3_INVALID_MEMBER;
}

KV3MemberId_t CKeyValues3Table::FindMember( const KeyValues3* kv ) const
{
	const Member_t* pMembers = MembersBase();

	for ( int i = 0; i < m_nCount; ++i )
		if ( pMembers[i] == kv )
			return i;

	return KV3_INVALID_MEMBER;
}

KV3MemberId_t CKeyValues3Table::CreateMember( KeyValues3 *parent, const CKV3MemberName &name, bool name_external )
{
	if ( GetMemberCount() >= 128 && !m_pFastSearch )
		EnableFastSearch();

	KV3MemberId_t curr = m_nCount;

	int new_size = m_nCount + 1;
	EnsureMemberCapacity( new_size );

	Hash_t *hashes_base = HashesBase();
	Member_t *members_base = MembersBase();
	Name_t *names_base = NamesBase();
	Flags_t *flags_base = FlagsBase();

	members_base[curr] = parent->AllocMember();
	hashes_base[curr] = name.GetHashCode();

	auto &curr_name = names_base[curr];
	auto &flags = flags_base[curr];

	if ( name_external )
	{
		curr_name = name.GetString();
		flags |= MEMBER_FLAG_EXTERNAL_NAME;
	}
	else
	{
		auto context = parent->GetContext();

		if ( context )
			curr_name = context->AllocString( name.GetString() );
		else
			curr_name = strdup( name.GetString() );
	}

	if ( m_pFastSearch && !m_pFastSearch->m_ignore )
		m_pFastSearch->m_member_ids.Insert( name.GetHashCode(), curr );

	m_nCount = new_size;

	return curr;
}

void CKeyValues3Table::CopyFrom( KeyValues3 *parent, const CKeyValues3Table* src )
{
	int new_size = src->GetMemberCount();

	RemoveAll( parent, new_size );
	EnsureMemberCapacity( new_size, true, true );

	auto context = parent->GetContext();

	Hash_t *hashes_base = HashesBase();
	Member_t *members_base = MembersBase();
	Name_t *names_base = NamesBase();
	Flags_t *flags_base = FlagsBase();

	const Hash_t *src_hashes_base = src->HashesBase();
	const Member_t *src_members_base = src->MembersBase();
	const Name_t *src_names_base = src->NamesBase();
	const Flags_t *src_flags_base = src->FlagsBase();

	memmove( hashes_base, src_hashes_base, sizeof(Hash_t) * new_size );

	for ( int i = 0; i < new_size; i++ )
	{
		flags_base[i] = src_flags_base[i] & ~MEMBER_FLAG_EXTERNAL_NAME;

		if ( context )
			names_base[i] = context->AllocString( src_names_base[i] );
		else
			names_base[i] = strdup( src_names_base[i] );

		members_base[i] = parent->AllocMember();
		members_base[i]->CopyFrom( *src_members_base[i] );
	}

	if ( new_size >= 128 )
		EnableFastSearch();

	m_nCount = new_size;
}

void CKeyValues3Table::RenameMember( KeyValues3 *parent, KV3MemberId_t id, const CKV3MemberName &newName )
{
	Hash_t* hashes_base = HashesBase();
	Name_t* names_base = NamesBase();
	Flags_t* flags_base = FlagsBase();

	auto &name = names_base[id];
	auto &flags = flags_base[id];

	hashes_base[id] = newName;

	auto context = parent->GetContext();

	if ( context )
	{
		name = context->AllocString( newName.GetString() );
	}
	else
	{
		if ( flags & MEMBER_FLAG_EXTERNAL_NAME )
			flags &= ~MEMBER_FLAG_EXTERNAL_NAME;
		else if ( name )
			free( (void *)name );

		name = strdup( newName.GetString() );
	}

	if ( m_pFastSearch )
	{
		m_pFastSearch->m_ignore = true;
		m_pFastSearch->m_ignores_counter = 1;
	}
}

void CKeyValues3Table::RemoveMember( KeyValues3 *parent, KV3MemberId_t id )
{
	m_nCount--;

	Hash_t* hashes_base = HashesBase();
	Member_t* members_base = MembersBase();
	Name_t* names_base = NamesBase();
	Flags_t* flags_base = FlagsBase();

	parent->FreeMember( members_base[id] );

	if ( ( flags_base[id] & MEMBER_FLAG_EXTERNAL_NAME ) == 0 && !parent->GetContext() && names_base[id] )
	{
		free( (void *)names_base[id] );
	}

	if ( id < m_nCount )
	{
		int shift_size = m_nCount - id;
		int shift_from = id + 1;

		memmove( &hashes_base[id], &hashes_base[shift_from], shift_size * sizeof(Hash_t) );
		memmove( &members_base[id], &members_base[shift_from], shift_size * sizeof(Member_t) );
		memmove( &names_base[id], &names_base[shift_from], shift_size * sizeof(Name_t) );
		memmove( &flags_base[id], &flags_base[shift_from], shift_size * sizeof(Flags_t) );
	}

	if ( m_pFastSearch )
	{
		m_pFastSearch->m_ignore = true;
		m_pFastSearch->m_ignores_counter = 1;
	}
}

void CKeyValues3Table::RemoveAll( KeyValues3 *parent, int new_size )
{
	Member_t *members_base = MembersBase();
	Name_t *names_base = NamesBase();
	Flags_t *flags_base = FlagsBase();

	for(int i = 0; i < m_nCount; i++)
	{
		parent->FreeMember( members_base[i] );

		if ( ( flags_base[i] & MEMBER_FLAG_EXTERNAL_NAME ) == 0 && !parent->GetContext() && names_base[i] )
		{
			free( (void *)names_base[i] );
		}
	}

	m_nCount = 0;
	if ( new_size > 0 )
	{
		EnsureMemberCapacity( new_size, true, true );
	}

	if ( new_size < 128 )
	{
		PurgeFastSearch();
	}
	else
	{
		EnableFastSearch();
		m_pFastSearch->m_member_ids.Reserve( new_size );
	}
}

void CKeyValues3Table::PurgeFastSearch()
{
	if ( m_pFastSearch )
	{
		Delete( m_pFastSearch );
	}

	m_pFastSearch = nullptr;
}

void CKeyValues3Table::PurgeContent( KeyValues3 *parent, bool bClearingContext )
{
	Member_t *members_base = MembersBase();
	Name_t *names_base = NamesBase();
	Flags_t *flags_base = FlagsBase();

	for ( int i = 0; i < m_nCount; ++i )
	{
		if ( !bClearingContext && parent )
		{
			parent->FreeMember( members_base[i] );
		}

		if ( ( flags_base[i] & MEMBER_FLAG_EXTERNAL_NAME ) == 0 && parent && !parent->GetContext() && names_base[i] )
		{
			free( (void *)names_base[i] );
		}
	}

	m_nCount = 0;

	PurgeFastSearch();
}

void CKeyValues3Table::PurgeBuffers()
{
	if ( m_bIsDynamicallySized )
	{
		free( m_pDynamicBuffer );
		m_nAllocatedChunks = m_nInitialSize;
		m_bIsDynamicallySized = false;
	}

	m_nCount = 0;
}

CKeyValues3ContextBase::CKeyValues3ContextBase( CKeyValues3Context* context ) : 	
	m_pContext( context ),
	m_KV3BaseCluster( context ),
	m_bMetaDataEnabled( false ),
	m_bFormatConverted( false ),
	m_bRootAvailabe( false ),
	m_pParsingErrorListener( nullptr )
{
	m_KV3PartialClusters.AddToChain( &m_KV3BaseCluster );
}

void CKeyValues3ContextBase::Clear()
{
	m_BinaryData.Clear();
	m_KV3BaseCluster.Clear();
	m_Symbols.RemoveAll();
	
	m_bFormatConverted = false;
}

void CKeyValues3ContextBase::Purge()
{
	m_BinaryData.Purge();
	m_KV3BaseCluster.Purge();
	m_Symbols.Purge();

	m_bFormatConverted = false;
}

CKeyValues3Context::CKeyValues3Context( bool bNoRoot ) : BaseClass( this ), pad{}
{
	if ( bNoRoot )
	{
		m_bRootAvailabe =  false;
	}
	else
	{
		m_bRootAvailabe = true;
		m_KV3BaseCluster.Alloc();
	}

	m_bMetaDataEnabled = false;
	m_bFormatConverted = false;
}

void CKeyValues3Context::Clear()
{
	BaseClass::Clear();

	ClearClusterNodeChain( m_KV3PartialClusters );
	ClearClusterNodeChain( m_KV3FullClusters );
	MoveToPartial( m_KV3FullClusters, m_KV3PartialClusters );

	ClearClusterNodeChain( m_PartialArrayClusters );
	ClearClusterNodeChain( m_FullArrayClusters );
	MoveToPartial( m_FullArrayClusters, m_PartialArrayClusters );
	m_RawArrayEntries.Clear();

	ClearClusterNodeChain( m_PartialTableClusters );
	ClearClusterNodeChain( m_FullTableClusters );
	MoveToPartial( m_FullTableClusters, m_PartialTableClusters );
	m_RawTableEntries.Clear();

	if ( m_bRootAvailabe )
		m_KV3BaseCluster.Alloc();
}

void CKeyValues3Context::Purge()
{
	BaseClass::Purge();

	PurgeClusterNodeChain( m_KV3PartialClusters );
	PurgeClusterNodeChain( m_KV3FullClusters );
	m_KV3PartialClusters.AddToChain( &m_KV3BaseCluster );

	PurgeClusterNodeChain( m_PartialArrayClusters );
	PurgeClusterNodeChain( m_FullArrayClusters );
	m_RawArrayEntries.Purge();

	PurgeClusterNodeChain( m_PartialTableClusters );
	PurgeClusterNodeChain( m_FullTableClusters );
	m_RawTableEntries.Purge();

	if ( m_bRootAvailabe )
		m_KV3BaseCluster.Alloc();
}

KeyValues3* CKeyValues3Context::Root()
{
	if ( !m_bRootAvailabe )
	{
		Plat_FatalError( "FATAL: %s called on a pool context (no root available)\n", __FUNCTION__ );
		DebuggerBreak();
	}

	return &m_KV3BaseCluster.Head()->m_Value;
}

const char* CKeyValues3Context::AllocString( const char* pString )
{
	return m_Symbols.AddString( pString ).String();
}

void CKeyValues3Context::EnableMetaData( bool bEnable )
{
	if ( bEnable != m_bMetaDataEnabled )
	{
		m_KV3BaseCluster.EnableMetaData( bEnable );

		m_bMetaDataEnabled = bEnable;
	}
}

void CKeyValues3Context::CopyMetaData( KV3MetaData_t* pDest, const KV3MetaData_t* pSrc )
{
	pDest->m_nLine = pSrc->m_nLine;
	pDest->m_nColumn = pSrc->m_nColumn;
	pDest->m_nFlags = pSrc->m_nFlags;
	pDest->m_sName = m_Symbols.AddString( pSrc->m_sName.String() );

	pDest->m_Comments.Purge();
	pDest->m_Comments.EnsureCapacity( pSrc->m_Comments.Count() );

	FOR_EACH_MAP_FAST( pSrc->m_Comments, iter )
	{
		pDest->m_Comments.Insert( pSrc->m_Comments.Key( iter ), pSrc->m_Comments.Element( iter ) );
	}
}

KeyValues3* CKeyValues3Context::AllocKV( KV3TypeEx_t type, KV3SubType_t subtype )
{
	return Alloc( m_KV3PartialClusters, m_KV3FullClusters, CKeyValues3Cluster::CLUSTER_SIZE, type, subtype );
}

void CKeyValues3Context::FreeKV( KeyValues3* kv )
{
	CKeyValues3Context* context;
	KV3MetaData_t* metadata = kv->GetMetaData( &context );

	if ( metadata )
		metadata->Clear();

	Free<CKeyValues3Cluster, KeyValues3>( kv, m_KV3PartialClusters, m_KV3FullClusters );
}

#include "tier0/memdbgoff.h"
