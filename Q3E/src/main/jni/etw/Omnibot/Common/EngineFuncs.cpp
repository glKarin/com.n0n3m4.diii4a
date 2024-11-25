#include "PrecompCommon.h"
#include "EngineFuncs.h"
#include "Utilities.h"

namespace EngineFuncs
{
	Prof_Define(EngineFuncs);

	bool TraceLine(obTraceResult &_tr, const Vector3f &_start, const Vector3f &_end, 
		const AABB *_aabb, int _mask, int _user, obBool _usepvs)
	{
		//Prof_Scope(EngineFuncs);
		Prof(TraceLine);

		static bool bDraw = false;
		{
			if(bDraw)
			{
				Utils::DrawLine(_start, _end, COLOR::BLUE, 0.02f);
			}
		}

		_tr.m_Endpos[0] = _start.x;
		_tr.m_Endpos[1] = _start.y;
		_tr.m_Endpos[2] = _start.z;
		return SUCCESS(g_EngineFuncs->TraceLine(_tr, _start, _end, _aabb, _mask, _user, _usepvs));
	}	
	String EntityName(const GameEntity _ent, const char *_default)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityName);

			const char *pName = g_EngineFuncs->GetEntityName(_ent);
			if(!_default) _default = "";
			return pName ? pName : _default;
		}
	}
	GameEntity EntityOwner(const GameEntity _ent)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityOwner);

			return g_EngineFuncs->GetEntityOwner(_ent);
		}
	}
	bool EntityPosition(const GameEntity _ent, Vector3f &_pos)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityPosition);

			return SUCCESS(g_EngineFuncs->GetEntityPosition(_ent, _pos));
		}
	}
	bool EntityEyePosition(const GameEntity _ent, Vector3f &_pos)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityEyePosition);

			return SUCCESS(g_EngineFuncs->GetEntityEyePosition(_ent, _pos));
		}
	}
	bool EntityBonePosition(const GameEntity _ent, int _boneid, Vector3f &_pos)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityBonePosition);

			return SUCCESS(g_EngineFuncs->GetEntityBonePosition(_ent, _boneid, _pos));
		}
	}
	bool EntityOrientation(const GameEntity _ent, float _fwd[3], float _right[3], float _up[3])
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityOrientation);

			return SUCCESS(g_EngineFuncs->GetEntityOrientation(_ent, _fwd, _right, _up));
		}
	}
	bool EntityOrientation(const GameEntity _ent, Matrix3f & mat)
	{
		Vector3f fwd, right, up;
		if (SUCCESS(g_EngineFuncs->GetEntityOrientation(_ent, fwd, right, up)))
		{
			mat = Matrix3f(fwd, right, up, true);
			return true;
		}
		return false;
	}
	bool EntityVelocity(const GameEntity _ent, Vector3f &_vel)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityVelocity);

			return SUCCESS(g_EngineFuncs->GetEntityVelocity(_ent, _vel));
		}
	}
	bool EntityLocalAABB(const GameEntity _ent, AABB &_bounds)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityWorldAABB);

			return SUCCESS(g_EngineFuncs->GetEntityLocalAABB(_ent, _bounds));
		}
	}
	bool EntityWorldAABB(const GameEntity _ent, AABB &_bounds)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityWorldAABB);

			return SUCCESS(g_EngineFuncs->GetEntityWorldAABB(_ent, _bounds));
		}
	}
	bool EntityWorldOBB(const GameEntity _ent, Box3f &_bounds)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityWorldOBB);
			if(SUCCESS(g_EngineFuncs->GetEntityWorldOBB(_ent, 
				_bounds.Center,
				_bounds.Axis[0],
				_bounds.Axis[1],
				_bounds.Axis[2],
				_bounds.Extent))) {
				return true;
			}
			return false;		
		}
	}
	bool EntityGroundEntity(const GameEntity _ent, GameEntity &moveent)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(EntityGroundEntity);

			return SUCCESS(g_EngineFuncs->GetEntityGroundEntity(_ent, moveent));
		}
	}
	bool IsInPvs(const Vector3f &_source,const Vector3f &_target)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(IsInPvs);

			return g_EngineFuncs->IsInPVS(_source, _target)==True;
		}
	}
	bool GroundPosition(const Vector3f &_pos, Vector3f &_outPosition, Vector3f &_outNormal, float _offset)
	{
		//Prof_Scope(EngineFuncs);
		{
			Prof(GroundPosition);

			obTraceResult tr;
			EngineFuncs::TraceLine(tr, 
				_pos.AddZ(_offset), 
				_pos.AddZ(-2048.f),
				NULL,
				TR_MASK_FLOODFILL, -1, False);
			if(tr.m_Fraction < 1.f)
			{
				_outPosition = Vector3f(tr.m_Endpos[0],tr.m_Endpos[1],tr.m_Endpos[2]);
				_outNormal = Vector3f(tr.m_Normal[0],tr.m_Normal[1],tr.m_Normal[2]);
				return true;
			}
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	//StringList			g_ThreadMessages;
	//StringList			g_ThreadErrors;

	//TryMutex			g_MessageMutex;

	void ThreadAddMessage(const String &_msg)
	{
		//TryMutex::scoped_lock sl(g_MessageMutex);
		//g_ThreadMessages.push_back(_msg);
	}

	void ThreadAddError(const String &_msg)
	{
		//TryMutex::scoped_lock sl(g_MessageMutex);
		//g_ThreadErrors.push_back(_msg);
	}

	void FlushAsyncMessages()
	{
		/*TryMutex::scoped_try_lock sl(g_MessageMutex,boost::defer_lock);
		if(sl.try_lock())
		{
			while(!g_ThreadErrors.empty())
			{
				g_EngineFuncs->PrintError(g_ThreadErrors.front().c_str());
				g_ThreadErrors.pop_front();
			}
			while(!g_ThreadMessages.empty())
			{
				g_EngineFuncs->PrintMessage(g_ThreadMessages.front().c_str());
				g_ThreadMessages.pop_front();
			}
			sl.unlock();
		}*/
	}

	//////////////////////////////////////////////////////////////////////////

	void ConsoleMessage(const char* _msg)
	{
		g_EngineFuncs->PrintMessage(_msg);
		//ThreadAddMessage(_msg);
	}

	void ConsoleError(const char* _msg)
	{
		g_EngineFuncs->PrintError(_msg);
		//ThreadAddError(_msg);
	}
}



obResult IEngineInterface71wrapper::TraceLine(obTraceResult &_result, const float _start[3], const float _end[3], const AABB *_pBBox, int _mask, int _user, obBool _bUsePVS)
{
	if(_mask&TR_MASK_FLOODFILLENT) _mask|=TR_MASK_PLAYERCLIP; //grates in map cortex_who
	_mask = (_mask&0x3f)|((_mask&~0xff)>>2);
	return base->TraceLine(_result, _start, _end, _pBBox, _mask, _user, _bUsePVS);
}

void IEngineInterface71wrapper::FixEntityCategory(BitFlag32& category)
{
	obint32 c = category.GetRawFlags();
	c = (c&0x1f)|((c&~0x1f)<<5);
	c = (c&0xffff)|((c&~0xffff)<<1);
	c = (c&0x3ffff)|((c&~0x3ffff)<<1);
	category = BitFlag32(c);
}

obResult IEngineInterface71wrapper::GetEntityCategory(const GameEntity _ent, BitFlag32 &_category)
{
	obResult result = base->GetEntityCategory(_ent, _category);
	if(SUCCESS(result))
	{
		FixEntityCategory(_category);
		if(_category.CheckFlag(ENT_CAT_PICKUP))
		{
			int entClass=GetEntityClass(_ent);
			if(entClass==ENT_CLASS_GENERIC_HEALTH)
				_category.SetFlag(ENT_CAT_PICKUP_HEALTH);
			else if(entClass==ENT_CLASS_GENERIC_AMMO)
				_category.SetFlag(ENT_CAT_PICKUP_AMMO);
			else if(entClass==ENT_CLASS_GENERIC_WEAPON)
				_category.SetFlag(ENT_CAT_PICKUP_WEAPON);
		}
	}
	return result;
}

obResult IEngineInterface71wrapper::GetEntityLocalAABB(const GameEntity _ent, AABB &_aabb)
{
	obResult result = base->GetEntityWorldAABB(_ent, _aabb);
	if(SUCCESS(result))
	{
		float pos[3];
		result = base->GetEntityPosition(_ent, pos);
		if(SUCCESS(result))
		{
			_aabb.UnTranslate(pos);
		}
	}
	return result;
}

obResult IEngineInterface71wrapper::GetEntityWorldOBB(const GameEntity _ent, float *_center, float *_axis0, float *_axis1, float *_axis2, float *_extents)
{
	AABB aabb;
	obResult result = base->GetEntityWorldAABB(_ent, aabb);
	if(SUCCESS(result))
	{
		aabb.CenterPoint(_center);
		for(int i=0; i<3; i++)
		{
			_axis0[i]=Vector3<float>::UNIT_X[i];
			_axis1[i]=Vector3<float>::UNIT_Y[i];
			_axis2[i]=Vector3<float>::UNIT_Z[i];
			_extents[i] = aabb.GetAxisLength(i)/2;
		}
	}
	return result;
}

obResult IEngineInterface71wrapper::InterfaceSendMessage(const MessageHelper &_data, const GameEntity _ent)
{ 
	if(_data.GetMessageId()>43) _data.m_MessageId++;
	if(_data.GetMessageId()==GEN_MSG_GETCONTROLLINGTEAM) _data.m_MessageId=44;
	return base->InterfaceSendMessage(_data, _ent);
}
