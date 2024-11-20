#include "PrecompCommon.h"
#include "IGameManager.h"
#include "ScriptManager.h"
#include "Base_Messages.h"

namespace AiState
{
	SensoryMemory::pfnGetEntityOffset SensoryMemory::m_pfnGetTraceOffset = NULL;
	SensoryMemory::pfnGetEntityOffset SensoryMemory::m_pfnGetAimOffset = NULL;
	SensoryMemory::pfnGetEntityVisDistance SensoryMemory::m_pfnGetVisDistance = NULL;
	SensoryMemory::pfnCanSensoreEntity SensoryMemory::m_pfnCanSensoreEntity = NULL;
	SensoryMemory::pfnAddSensorCategory SensoryMemory::m_pfnAddSensorCategory = NULL;

	SensoryMemory::SensoryMemory() : StateChild("SensoryMemory", UpdateDelay(Utils::HzToSeconds(10))),
		m_MemorySpan(5000)
	{
		m_DebugFlags.SetFlag(Dbg_ShowPerception,true);
		m_DebugFlags.SetFlag(Dbg_ShowEntities,true);
	}

	SensoryMemory::~SensoryMemory()
	{
	}

	int SensoryMemory::GetAllRecords(MemoryRecord *_records, int _max)
	{
		int iNum = 0;
		for(int i = 0; i < NumRecords; ++i)
		{
			if(!m_Records[i].GetEntity().IsValid())
				continue;

			_records[iNum++] = m_Records[i];

			if(iNum>=_max-1)
				break;
		}
		return iNum;
	}

	void SensoryMemory::GetDebugString(StringStr &out)
	{
		int iNumRecords = 0;
		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
				++iNumRecords;
		}
		out << iNumRecords;
	}

	void SensoryMemory::RenderDebug()
	{
		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
			{
				const MemoryRecord &r = m_Records[i];

				if(m_DebugFlags.CheckFlag(Dbg_ShowEntities))
				{
					AABB box;
					EngineFuncs::EntityWorldAABB(r.GetEntity(), box);
					if(box.GetArea() <= 5.f)
						box.Expand(10.f);

					obColor col = r.IsShootable()?COLOR::GREEN:COLOR::RED;

					if(r.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD) || 
						r.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
						col = COLOR::BLACK;

					Utils::OutlineAABB(box, col, IGame::GetDeltaTimeSecs() * 3.f);

					Vector3f vCenter;
					box.CenterPoint(vCenter);

					const char *ClassName = Utils::FindClassName(m_Records[i].m_TargetInfo.m_EntityClass);
					Utils::PrintText(
						vCenter,
						COLOR::WHITE,
						IGame::GetDeltaTimeSecs() * 3.f,
						ClassName?ClassName:"<unknown>");
				}
			}
		}
	}

	void SensoryMemory::Enter()
	{
	}

	void SensoryMemory::Exit()
	{
		// clear all shootable ents
		for(int i = 0; i < NumRecords; ++i)
		{
			m_Records[i].m_IsShootable		= false;
			m_Records[i].m_InFOV			= false;
			m_Records[i].m_TimeLastSensed	= -1;
		}
	}

	void SensoryMemory::SetEntityTraceOffsetCallback(pfnGetEntityOffset _pfnCallback)
	{
		m_pfnGetTraceOffset = _pfnCallback;
	}

	void SensoryMemory::SetEntityAimOffsetCallback(pfnGetEntityOffset _pfnCallback)
	{
		m_pfnGetAimOffset = _pfnCallback;
	}
	void SensoryMemory::SetEntityVisDistanceCallback(pfnGetEntityVisDistance _pfnCallback)
	{
		m_pfnGetVisDistance = _pfnCallback;
	}
	void SensoryMemory::SetCanSensoreEntityCallback(pfnCanSensoreEntity _pfnCallback)
	{
		m_pfnCanSensoreEntity = _pfnCallback;
	}

	void SensoryMemory::UpdateEntities()
	{
		Prof(UpdateEntities);

		bool bFoundEntity = false;
		
		GameEntity selfEntity = GetClient()->GetGameEntity();

		IGame::EntityIterator ent;
		while(IGame::IterateEntity(ent))
		{
			if(m_pfnCanSensoreEntity && !m_pfnCanSensoreEntity(ent.GetEnt()))
				continue;

			// skip myself
			if(selfEntity == ent.GetEnt().m_Entity)
				continue;

			// skip internal entities
			if(ent.GetEnt().m_EntityCategory.CheckFlag(ENT_CAT_INTERNAL))
				continue;

			obint32 iFreeRecord = -1;
			bFoundEntity = false;

			const int iStartIndex = 
				ent.GetEnt().m_EntityClass < FilterSensory::ANYPLAYERCLASS || 
				ent.GetEnt().m_EntityClass == ENT_CLASS_GENERIC_SPECTATOR ? 0 : 64;

			for(int i = iStartIndex; i < NumRecords; ++i)
			{
				if(m_Records[i].GetEntity().IsValid())
				{
					if(m_Records[i].GetEntity().GetIndex() == ent.GetEnt().m_Entity.GetIndex())
					{
						m_Records[i].m_Entity = ent.GetEnt().m_Entity; // update just in case
						bFoundEntity = true;
						break;
					}
				}
				else
				{
					if(iFreeRecord == -1)
						iFreeRecord = i;
				}
			}

			if(!bFoundEntity)
			{
				OBASSERT(iFreeRecord!=-1, "No Free Record Slot!");
				if(iFreeRecord!=-1)
				{
					m_Records[iFreeRecord].m_Entity = ent.GetEnt().m_Entity;
					m_Records[iFreeRecord].m_TargetInfo.m_EntityCategory = ent.GetEnt().m_EntityCategory;
					m_Records[iFreeRecord].m_TargetInfo.m_EntityClass = ent.GetEnt().m_EntityClass;
					m_Records[iFreeRecord].m_TimeLastUpdated = -1; // initial update
				}
			}
		}
	}

	void SensoryMemory::UpdateSight()
	{
		Prof(UpdateSight);

		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
			{
				if(!IGame::IsEntityValid(m_Records[i].GetEntity()))
				{
					m_Records[i].Reset();
					continue;
				}
				UpdateRecord(m_Records[i]);
			}
		}
	}

	void SensoryMemory::UpdateSound()
	{
		Prof(UpdateSound);
	}

	void SensoryMemory::UpdateSmell()
	{
		Prof(UpdateSmell);
	}

	void SensoryMemory::UpdateWithSoundSource(const Event_Sound *_sound)
	{
		// TODO: don't bother storing sound memory from myself.
		if(_sound->m_Source.IsValid() && (GetClient()->GetGameEntity() != _sound->m_Source))
		{
		}
	}

	void SensoryMemory::UpdateWithTouchSource(GameEntity _sourceent)
	{
		// Paranoid check that we aren't touching ourselves, har har
		if(_sourceent.IsValid() && (GetClient()->GetGameEntity() != _sourceent))
		{
			MemoryRecord *pRecord = GetMemoryRecord(_sourceent, true, false);
			if(pRecord)
			{
				pRecord->m_TargetInfo.m_EntityClass = g_EngineFuncs->GetEntityClass(_sourceent);
				if(!pRecord->m_TargetInfo.m_EntityClass)
					return;

				pRecord->m_TargetInfo.m_EntityCategory.ClearAll();
				InterfaceFuncs::GetEntityCategory(_sourceent, pRecord->m_TargetInfo.m_EntityCategory);

				// Get the entity position.
				Vector3f vThreatPosition(Vector3f::ZERO);
				EngineFuncs::EntityPosition(_sourceent, vThreatPosition);

				// Add it to the list.
				pRecord->m_InFOV			= true;
				pRecord->m_IsShootable		= GetClient()->HasLineOfSightTo(vThreatPosition, _sourceent);
				pRecord->m_TimeLastSensed	= IGame::GetTime();
				pRecord->m_IsAllied			= GetClient()->IsAllied(_sourceent);

				// Update Target Info.
				pRecord->m_TargetInfo.m_LastPosition	= vThreatPosition;

				if(pRecord->m_IsShootable)
				{
					pRecord->m_TimeLastVisible    = IGame::GetTime();
				}
			}
		}
	}

	bool SensoryMemory::UpdateRecord(MemoryRecord &_record)
	{
		Prof(UpdateRecord);

		if(_record.GetAge() <= 0)
			return true;			

		_record.MarkUpdated();

		TargetInfo &ti = _record.m_TargetInfo;

		const bool bIsStatic = ti.m_EntityCategory.CheckFlag(ENT_CAT_STATIC);
		const bool bShootable = ti.m_EntityCategory.CheckFlag(ENT_CAT_SHOOTABLE);

		GameEntity ent = _record.GetEntity();

		//////////////////////////////////////////////////////////////////////////
		// Some entity flags are persistent and won't be overwritten until the subject is perceived again.
		const BitFlag64 bfPersistantMask
			( ((obint64)1<<ENT_FLAG_DEAD)
			| ((obint64)1<<ENT_FLAG_DISABLED));

		BitFlag64 bfPersistantFlags = bfPersistantMask&ti.m_EntityFlags;
		//////////////////////////////////////////////////////////////////////////

		// clear targeting info 
		if(ti.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD) || ti.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED)) 
		{ 
			_record.m_IsShootable		= false; 
			_record.m_InFOV				= false; 
			_record.m_TimeLastSensed	= -1; 
		}

		// Update data that changes.
		ti.m_EntityFlags.ClearAll();
		ti.m_EntityPowerups.ClearAll();
		ti.m_EntityClass = InterfaceFuncs::GetEntityClass(ent);

		Vector3f vNewPosition;
		if(!InterfaceFuncs::GetEntityFlags(ent, ti.m_EntityFlags))
			return false;
		if(!InterfaceFuncs::GetEntityPowerUps(ent, ti.m_EntityPowerups))
			return false;
		if(!EngineFuncs::EntityPosition(ent, vNewPosition))
			return false;
		if(!InterfaceFuncs::GetEntityCategory(ent, ti.m_EntityCategory))
			return false;

		//////////////////////////////////////////////////////////////////////////
		Vector3f vTracePosition = vNewPosition;
		if(m_pfnGetTraceOffset)
			vTracePosition.z += m_pfnGetTraceOffset(ti.m_EntityClass, ti.m_EntityFlags);

		if(bIsStatic || 
			(GetClient()->IsWithinViewDistance(vTracePosition) && 
			GetClient()->InFieldOfView(vTracePosition)))
		{
			const float DistanceToEntity = Length(vTracePosition, GetClient()->GetEyePosition());
			float EntityViewDistance = Utils::FloatMax;
			if(m_pfnGetVisDistance)
				m_pfnGetVisDistance(EntityViewDistance, ti, GetClient());

			if(DebugDrawingEnabled() && m_DebugFlags.CheckFlag(Dbg_ShowPerception))
			{
				Utils::DrawLine(GetClient()->GetEyePosition(),vTracePosition,COLOR::YELLOW,0.2f);
			}

			if(bIsStatic || 
				(DistanceToEntity < EntityViewDistance &&
				(!ti.m_EntityFlags.CheckFlag(ENT_FLAG_VISTEST) || 
				GetClient()->HasLineOfSightTo(vTracePosition, ent))))
			{
				//const bool bNewlySeen = (IGame::GetTime() - _record.GetTimeLastSensed()) > m_MemorySpan;

				// Still do the raycast on shootable statics
				if(bIsStatic)
				{
					//const bool bWasShootable = _record.m_IsShootable;
					//const bool bWasInFov = _record.m_InFOV;

					_record.m_IsShootable = false;					
					if(bShootable)
					{
						if(!GetClient()->HasLineOfSightTo(vTracePosition, ent))
						{
							_record.m_IsShootable		= false;
							_record.m_InFOV				= false;
							return true;
						}
					}
				}

				_record.m_IsShootable				= true;
				_record.m_TimeLastSensed			= IGame::GetTime();
				_record.m_TimeLastVisible			= IGame::GetTime();
				_record.m_IsAllied					= GetClient()->IsAllied(ent);

				_record.m_TargetInfo.m_CurrentWeapon = InterfaceFuncs::GetEquippedWeapon(ent).m_WeaponId;

				if(!_record.IsShootable() && !bIsStatic)
				{
					// restore the old flags
					ti.m_EntityFlags &= ~bfPersistantMask;
					ti.m_EntityFlags|=bfPersistantFlags;
				}
				//////////////////////////////////////////////////////////////////////////
				// Update Target Info.
				ti.m_LastPosition = vNewPosition;
				if(m_pfnGetAimOffset)
					ti.m_LastPosition.z += m_pfnGetAimOffset(ti.m_EntityClass, ti.m_EntityFlags);

				if(DebugDrawingEnabled() && m_DebugFlags.CheckFlag(Dbg_ShowPerception))
				{
					Utils::DrawLine(GetClient()->GetEyePosition(),ti.m_LastPosition,COLOR::YELLOW,0.2f);
				}

				_record.m_TargetInfo.m_DistanceTo = Length(ti.m_LastPosition, GetClient()->GetEyePosition());
				if(!bIsStatic)
				{
					EngineFuncs::EntityOrientation(ent, ti.m_LastFacing, 0, 0);
					EngineFuncs::EntityVelocity(ent, ti.m_LastVelocity);
				}
				//////////////////////////////////////////////////////////////////////////

				if(_record.m_InFOV == false)
				{
					_record.m_InFOV					= true;
					_record.m_TimeBecameVisible		= IGame::GetTime();
				}

				/*if(bNewlySeen)
				{
					Event_EntitySensed d;
					d.m_EntityClass = ti.m_EntityClass;
					d.m_Entity = ent;
					GetClient()->SendEvent(MessageHelper(PERCEPT_SENSE_ENTITY, &d, sizeof(d)));
				}*/
			} 
			else
			{
				// restore the old flags
				ti.m_EntityFlags &= ~bfPersistantMask;
				ti.m_EntityFlags|=bfPersistantFlags;

				// Can't see him.
				_record.m_IsShootable		= false;
				_record.m_InFOV				= false;
			}
		}
		return true;
	}

	void SensoryMemory::QueryMemory(FilterSensory &_filter)
	{
		Prof(QueryMemory);
		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
				_filter.Check(i, m_Records[i]);
		}
		_filter.PostQuery();
	}

	const TargetInfo *SensoryMemory::GetTargetInfo(const GameEntity _ent)
	{
		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid() && m_Records[i].GetEntity() == _ent)
			{
				return &m_Records[i].m_TargetInfo;
			}
		}
		return NULL;
	}

	int SensoryMemory::CheckTargetsInRadius(const Vector3f &_pos, float _radius, 
		SensoryMemory::Type _type, const BitFlag64 &_category)
	{
		int iNumInRadius = 0;
		/*float fRadSq = _radius * _radius;

		EntityList entList;

		FilterAllType all(entList, m_Client, _type, _category, BitFlag64(0));
		all.SetSortType(FilterSensory::Sort_NearToFar);
		m_Client->GetSensoryMemory()->QueryMemory(all);

		Vector3f vPos;
		for(obuint32 i = 0; i < entList.size(); ++i)
		{
		if(GetLastRecordedPosition(entList[i], vPos))
		{
		if((vPos - _pos).SquaredLength() <= fRadSq)
		{
		++iNumInRadius;
		}
		}
		}*/
		return iNumInRadius;
	}

	MemoryRecord *SensoryMemory::GetMemoryRecord(GameEntity _ent, bool _add, bool _update)
	{
		MemoryRecord *pRecord = 0;

		int iFreeSlot = -1;
		for(int i = 0; i < NumRecords; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
			{
				if(m_Records[i].GetEntity() == _ent)
				{
					pRecord = &m_Records[i];
					break;
				}
			}
			else if(iFreeSlot == -1 && i >= 64)
			{
				iFreeSlot = i;
			}
		}

		if(!pRecord && _add && iFreeSlot!=-1)
		{
			pRecord = &m_Records[iFreeSlot];
			pRecord->Reset(_ent);
		}

		if(_update && pRecord)
			UpdateRecord(*pRecord);

		return pRecord;
	}

	MemoryRecord *SensoryMemory::GetMemoryRecord(const RecordHandle &_hndl)
	{
		MemoryRecord *pRecord = 0;

		if(_hndl.IsValid())
		{
			int ix = (int)_hndl.GetIndex();
			if(InRangeT<int>(ix, 0, NumRecords-1) &&
				(m_Records[_hndl.GetIndex()].m_Serial == _hndl.GetSerial()))
			{
				pRecord = &m_Records[_hndl.GetIndex()];
			}
		}
		return pRecord;
	}

	void SensoryMemory::GetRecordInfo(const MemoryRecords &_hndls, Vector3List *_pos, Vector3List *_vel)
	{
		for(obuint32 i = 0; i < _hndls.size(); ++i)
		{
			const MemoryRecord *pRec = GetMemoryRecord(_hndls[i]);
			if(pRec)
			{
				if(_pos) _pos->push_back(pRec->GetLastSensedPosition());
				if(_vel) _vel->push_back(pRec->GetLastSensedVelocity());
			}
		}
	}

	int SensoryMemory::FindEntityByCategoryInRadius(float radius, BitFlag32 category, RecordHandle hndls[], int maxHandles)
	{
		int numRecords = 0;
		for(int i = 0; i < NumRecords && numRecords < maxHandles; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
			{
				if(!m_Records[i].m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
				{
					if((m_Records[i].m_TargetInfo.m_EntityCategory & category).AnyFlagSet())
					{
						if(m_Records[i].m_TargetInfo.m_DistanceTo <= radius && m_Records[i].GetTimeLastSensed() >= 0)
						{
							hndls[numRecords++] = RecordHandle((obint16 )i,m_Records[i].GetSerial());
						}
					}
				}
			}
		}
		return numRecords;
	}

	int SensoryMemory::FindEntityByCategoryInRadius(float radius, BitFlag32 category, GameEntity ents[], int maxEnts)
	{
		int numRecords = 0;
		for(int i = 0; i < NumRecords && numRecords < maxEnts; ++i)
		{
			if(m_Records[i].GetEntity().IsValid())
			{
				if(!m_Records[i].m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
				{
					if((m_Records[i].m_TargetInfo.m_EntityCategory & category).AnyFlagSet())
					{
						if(m_Records[i].m_TargetInfo.m_DistanceTo <= radius && m_Records[i].GetTimeLastSensed() >= 0)
						{
							ents[numRecords++] = m_Records[i].GetEntity();
						}					
					}
				}
			}
		}
		return numRecords;
	}

	bool SensoryMemory::HasLineOfSightTo(const MemoryRecord &mr, int customTraceMask)
	{
		Vector3f vTracePosition = mr.m_TargetInfo.m_LastPosition;
		if(m_pfnGetTraceOffset)
			vTracePosition.z += m_pfnGetTraceOffset(mr.m_TargetInfo.m_EntityClass, mr.m_TargetInfo.m_EntityFlags);
		return GetClient()->HasLineOfSightTo(vTracePosition, mr.GetEntity(), customTraceMask);
	}

	//////////////////////////////////////////////////////////////////////////
	// State stuff

	State::StateStatus SensoryMemory::Update(float fDt)
	{
		Prof(SensoryMemory);
		
		UpdateEntities();
		UpdateSight();
		//UpdateSound();
		//UpdateSmell();

		return State_Busy;
	}
}
