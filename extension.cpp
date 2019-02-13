/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Outputinfo g_Outputinfo;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Outputinfo);

#include <isaverestore.h>

#ifdef PLATFORM_WINDOWS
#include <mempool.h>
#else
#include <mempool_hack.h>
#endif

#include <variant_t.h>
#include <itoolentity.h>

IServerTools *servertools = nullptr;

#if SOURCE_ENGINE == SE_CSGO
#ifdef PLATFORM_WINDOWS
typedef int* (*AllocFunction)();
#else
typedef int* (*AllocFunction)(void*);
#endif

CUtlMemoryPool *g_pEntityListPool = nullptr;
AllocFunction g_EntityListPool_Alloc = nullptr;
#endif

#define EVENT_FIRE_ALWAYS	-1

class CEventAction
{
public:
	CEventAction(const char *ActionData = NULL) { m_iIDStamp = 0; };

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext;

	// allocates memory from engine.MPool/g_EntityListPool
#if SOURCE_ENGINE == SE_CSGO
	static void *operator new(size_t stAllocateBlock)
	{
#ifdef PLATFORM_WINDOWS
		return g_EntityListPool_Alloc();
#else
		return g_EntityListPool_Alloc( g_pEntityListPool );
#endif
	}
	static void *operator new(size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine)
	{
#ifdef PLATFORM_WINDOWS
		return g_EntityListPool_Alloc();
#else
		return g_EntityListPool_Alloc( g_pEntityListPool );
#endif
	}
	static void operator delete(void *pMem)
	{
		g_pEntityListPool->Free(pMem);
	}
	static void operator delete( void *pMem , int nBlockUse, const char *pFileName, int nLine )
	{
		operator delete(pMem);
	}
#endif

	DECLARE_SIMPLE_DATADESC();

};

class CBaseEntityOutput
{
public:
	~CBaseEntityOutput();

	void ParseEventAction( const char *EventData );
	void AddEventAction( CEventAction *pEventAction );

	int Save( ISave &save );
	int Restore( IRestore &restore, int elementCount );

	int NumberOfElements( void );

	float GetMaxDelay( void );

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
/*
	/// Delete every single action in the action list.
	void DeleteAllElements( void ) ;
*/
public:
	variant_t m_Value;
	CEventAction *m_ActionList;
	DECLARE_SIMPLE_DATADESC();

	CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	CBaseEntityOutput( CBaseEntityOutput& ); // protect from accidental copying
};

void CBaseEntityOutput::AddEventAction(CEventAction *pEventAction)
{
	pEventAction->m_pNext = m_ActionList;
	m_ActionList = pEventAction;
}

int CBaseEntityOutput::NumberOfElements(void)
{
	int count = 0;
	for (CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
	{
		count++;
	}

	return count;
}

inline int GetDataMapOffset(CBaseEntity *pEnt, const char *pName)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pEnt);
	if(!pMap)
		return -1;

	typedescription_t *pTypeDesc = gamehelpers->FindInDataMap(pMap, pName);

	if(pTypeDesc == NULL)
		return -1;

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	return pTypeDesc->fieldOffset;
#else
	return pTypeDesc->fieldOffset[TD_OFFSET_NORMAL];
#endif
}

string_t AllocPooledString(const char *pszValue)
{
	// This is admittedly a giant hack, but it's a relatively safe method for
	// inserting a string into the game's string pool that isn't likely to break.
	//
	// We find the first valid ent (should always be worldspawn), save off it's
	// current targetname string_t, set it to our string to insert via SetKeyValue,
	// read back the new targetname value, restore the old value, and return the new one.

	CBaseEntity *pEntity = reinterpret_cast<IServerUnknown *>(servertools->FirstEntity())->GetBaseEntity();
	auto *pDataMap = gamehelpers->GetDataMap(pEntity);
	assert(pDataMap);

	static int offset = -1;
	if (offset == -1)
	{
		sm_datatable_info_t info;
		bool found = gamehelpers->FindDataMapInfo(pDataMap, "m_iName", &info);
		assert(found);
		offset = info.actual_offset;
	}

	string_t *pProp = (string_t *) ((intp) pEntity + offset);
	string_t backup = *pProp;
	servertools->SetKeyValue(pEntity, "targetname", pszValue);
	string_t newString = *pProp;
	*pProp = backup;

	return newString;
}

inline CBaseEntityOutput *GetOutput(CBaseEntity *pEntity, const char *pOutput)
{
	int offset = GetDataMapOffset(pEntity, pOutput);

	if(offset == -1)
		return nullptr;

	return (CBaseEntityOutput *)((intptr_t)pEntity + offset);
}

cell_t GetOutputActionCount(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL)
		return 0;

	return pEntityOutput->NumberOfElements();
}

cell_t GetOutputActionTarget(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	pContext->StringToLocal(params[4], params[5], pAction->m_iTarget.ToCStr());

	return 1;
}

cell_t SetOutputActionTarget(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	char *szTarget;
	pContext->LocalToString(params[4], &szTarget);
	pAction->m_iTarget = AllocPooledString(szTarget);

	return 1;
}

cell_t GetOutputActionTargetInput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	pContext->StringToLocal(params[4], params[5], pAction->m_iTargetInput.ToCStr());

	return 1;
}

cell_t SetOutputActionTargetInput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	char *szTargetInput;
	pContext->LocalToString(params[4], &szTargetInput);
	pAction->m_iTargetInput = AllocPooledString(szTargetInput);

	return 1;
}

cell_t GetOutputActionParameter(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	pContext->StringToLocal(params[4], params[5], pAction->m_iParameter.ToCStr());

	return 1;
}

cell_t SetOutputActionParameter(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	char *szParameter;
	pContext->LocalToString(params[4], &szParameter);
	pAction->m_iParameter = AllocPooledString(szParameter);

	return 1;
}

cell_t GetOutputActionDelay(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1.0f;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return -1.0f;

		pAction = pAction->m_pNext;
	}

	return sp_ftoc(pAction->m_flDelay);
}

cell_t SetOutputActionDelay(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	pAction->m_flDelay = sp_ctof(params[4]);
	return 1;
}

cell_t GetOutputActionTimesToFire(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pAction = pAction->m_pNext;
	}

	return pAction->m_nTimesToFire;
}

cell_t SetOutputActionTimesToFire(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pPrev = nullptr;
	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pPrev = pAction;
		pAction = pAction->m_pNext;
	}

	if (params[4] == 0) // delete this action
	{
		if (pPrev != nullptr)
		{
			pPrev->m_pNext = pAction->m_pNext;
		}
		else
		{
			pEntityOutput->m_ActionList = pAction->m_pNext;
		}

		delete pAction;
	}
	else
	{
		pAction->m_nTimesToFire = params[4];
	}

	return 1;
}

cell_t RemoveOutputAction(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pPrev = nullptr;
	CEventAction *pAction = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if( pAction->m_pNext == NULL)
			return 0;

		pPrev = pAction;
		pAction = pAction->m_pNext;
	}

	if (pPrev != nullptr)
	{
		pPrev->m_pNext = pAction->m_pNext;
	}
	else
	{
		pEntityOutput->m_ActionList = pAction->m_pNext;
	}

	delete pAction;

	return 1;
#else
	return pContext->ThrowNativeError( "This feature is unsupported on this version of the engine." );
#endif
}

cell_t InsertOutputAction(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_CSGO
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pNewAction = new CEventAction;
	char *buffer;

	pContext->LocalToString(params[3], &buffer);
	pNewAction->m_iTarget = AllocPooledString(buffer);

	pContext->LocalToString(params[4], &buffer);
	pNewAction->m_iTargetInput = AllocPooledString(buffer);

	pContext->LocalToString(params[5], &buffer);
	pNewAction->m_iParameter = AllocPooledString(buffer);

	pNewAction->m_flDelay = sp_ctof(params[6]);

	pNewAction->m_nTimesToFire = params[7];

	if(params[8] == 0)
	{
		pEntityOutput->AddEventAction(pNewAction);
	}
	else
	{
		CEventAction *pPrev = nullptr;
		CEventAction *pAction = pEntityOutput->m_ActionList;
		for( int i = 0; i < params[3]; i++ )
		{
			if( pAction->m_pNext == NULL )
				return 0;

			pPrev = pAction;
			pAction = pAction->m_pNext;
		}

		pPrev->m_pNext = pNewAction;
		pNewAction->m_pNext = pAction;
	}

	return 1;
#else
	return pContext->ThrowNativeError( "This feature is unsupported on this version of the engine." );
#endif
}

const sp_nativeinfo_t MyNatives[] =
{
	{ "GetOutputActionCount",		GetOutputActionCount },
	{ "GetOutputActionTarget",		GetOutputActionTarget },
	{ "SetOutputActionTarget",		SetOutputActionTarget },
	{ "GetOutputActionTargetInput",	GetOutputActionTargetInput },
	{ "SetOutputActionTargetInput",	SetOutputActionTargetInput },
	{ "GetOutputActionParameter",	GetOutputActionParameter },
	{ "SetOutputActionParameter",	SetOutputActionParameter },
	{ "GetOutputActionDelay",		GetOutputActionDelay },
	{ "SetOutputActionDelay",		SetOutputActionDelay },
	{ "GetOutputActionTimesToFire",	GetOutputActionTimesToFire },
	{ "SetOutputActionTimesToFire",	SetOutputActionTimesToFire },
	{ "InsertOutputAction",			InsertOutputAction },
	{ "RemoveOutputAction",			RemoveOutputAction },
	{ NULL, NULL },
};

bool Outputinfo::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
#if SOURCE_ENGINE == SE_CSGO
	IGameConfig *pGameConf;

	if(!gameconfs->LoadGameConfigFile("outputinfo.games", &pGameConf, error, maxlength))
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS
	pGameConf->GetAddress("g_EntityListPool", reinterpret_cast<void**>(&g_pEntityListPool));
	if(g_pEntityListPool == nullptr)
	{
		snprintf(error, maxlength, "Failed to obtain g_pEntityListPool from gamedata");
		gameconfs->CloseGameConfigFile(pGameConf);
		return false;
	}
#else
	char *operator_new_reloffset = nullptr;
	pGameConf->GetMemSig("rel_CEventAction_operator_new", reinterpret_cast<void**>(&operator_new_reloffset));
	if (operator_new_reloffset == nullptr) {
		snprintf( error, maxlength, "Failed to obtain rel_CEventAction_operator_new from gamedata" );
		gameconfs->CloseGameConfigFile(pGameConf);
		return false;
	}
	operator_new_reloffset += 1;
	g_pEntityListPool = *(CUtlMemoryPool**)((intptr_t)(operator_new_reloffset + 4) + *(intptr_t*)operator_new_reloffset + 12);
#endif

#ifdef PLATFORM_WINDOWS
	pGameConf->GetMemSig("g_EntityListPool.Alloc", reinterpret_cast<void**>(&g_EntityListPool_Alloc));
	if(g_EntityListPool_Alloc == nullptr)
	{
		snprintf( error, maxlength, "Failed to obtain g_EntityListPool.Alloc from gamedata" );
		gameconfs->CloseGameConfigFile(pGameConf);
		return false;
	}
#else
	pGameConf->GetMemSig( "CUtlMemoryPool_Alloc", reinterpret_cast<void**>( &g_EntityListPool_Alloc ) );
	if( g_EntityListPool_Alloc == nullptr )
	{
		snprintf( error, maxlength, "Failed to obtain CUtlMemoryPool_Alloc from gamedata" );
		gameconfs->CloseGameConfigFile( pGameConf );
		return false;
	}
#endif

	gameconfs->CloseGameConfigFile(pGameConf);
#endif

	return true;
}

void Outputinfo::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
}

bool Outputinfo::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	return true;
}
