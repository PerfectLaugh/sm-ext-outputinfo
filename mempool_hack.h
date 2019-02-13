typedef void(*MemoryPoolReportFunc_t)(char const* pMsg, ...);

enum MemoryPoolGrowType_t
{
	UTLMEMORYPOOL_GROW_NONE=0,		// Don't allow new blobs.
	UTLMEMORYPOOL_GROW_FAST=1,		// New blob size is numElements * (i+1)  (ie: the blocks it allocates
									// get larger and larger each time it allocates one).
	UTLMEMORYPOOL_GROW_SLOW=2		// New blob size is numElements.
};

class CUtlMemoryPool
{
public:
	// !KLUDGE! For legacy code support, import the global enum into this scope
	enum MemoryPoolGrowType_t
	{
		GROW_NONE=UTLMEMORYPOOL_GROW_NONE,
		GROW_FAST=UTLMEMORYPOOL_GROW_FAST,
		GROW_SLOW=UTLMEMORYPOOL_GROW_SLOW
	};

				CUtlMemoryPool( int blockSize, int numElements, int growMode = UTLMEMORYPOOL_GROW_FAST, const char *pszAllocOwner = NULL, int nAlignment = 0 );
				~CUtlMemoryPool();

	void*		Alloc();	// Allocate the element size you specified in the constructor.
	void*		Alloc( size_t amount );
	void*		AllocZero();	// Allocate the element size you specified in the constructor, zero the memory before construction
	void*		AllocZero( size_t amount );
	void		Free(void *pMem);
	
	// Frees everything
	void		Clear();

	// Error reporting... 
	static void SetErrorReportFunc( MemoryPoolReportFunc_t func );

	// returns number of allocated blocks
	int Count() { return m_BlocksAllocated; }
	int PeakCount() { return m_PeakAlloc; }

protected:
	class CBlob
	{
	public:
		CBlob	*m_pPrev, *m_pNext;
		int		m_NumBytes;		// Number of bytes in this blob.
		char	m_Data[1];
		char	m_Padding[3]; // to int align the struct
	};

	// Resets the pool
	void		Init();
	void		AddNewBlob();
	void		ReportLeaks();

	int			m_BlockSize;
	int			m_BlocksPerBlob;

	int			m_GrowMode;	// GROW_ enum.

	// Put m_BlocksAllocated in front of m_pHeadOfFreeList for better
	// packing on 64-bit where pointers are 8-byte aligned.
	int				m_BlocksAllocated;
	// FIXME: Change m_ppMemBlob into a growable array?
	void			*m_pHeadOfFreeList;
	int				m_PeakAlloc;
	unsigned short	m_nAlignment;
	unsigned short	m_NumBlobs;
	const char *	m_pszAllocOwner;
	// CBlob could be not a multiple of 4 bytes so stuff it at the end here to keep us otherwise aligned
	CBlob			m_BlobHead;

	static MemoryPoolReportFunc_t g_ReportFunc;
};