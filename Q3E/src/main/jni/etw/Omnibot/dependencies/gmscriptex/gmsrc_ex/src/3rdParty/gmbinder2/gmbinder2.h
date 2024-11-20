//////////////////////////////////////////////////////////////////////////
// gmbinder - by Jeremy Swigart
//
// This class provides a simple way to bind native classes to custom script types
//
// Features:
//		Bind classes to script, control whether script can create an instance or not.
//		Bind class variables to script.
//			Supports bool,int,float,const char *,std::string,gmTableObject,gmFunctionObject,
//		Bind static functions to script(up to 9 parameters). No glue function needed.
//		Bind member functions to script(up to 9 parameters). No glue function needed.
//		Bind class constants to script. Access through an instance currently.
//
// Unsupported Features:
//		Automatic binding of overloaded functions. Overloaded functions must be bound to
//			multiple script functions.
//		Ex: If your class had: void DoSomething(int) and void DoSomething(float)
//			you could bind both functions, but using different names for each script.
//		Variable argument functions. To support variable arguments you must use one of the
//			raw binding functions.
//////////////////////////////////////////////////////////////////////////

#ifndef _GMBINDER2_H_
#define _GMBINDER2_H_

#include "gmConfig.h"
#include "gmThread.h"
#include "gmGCRoot.h"

#include "gmbinder2_functraits.h"

namespace gmBind2
{
	//////////////////////////////////////////////////////////////////////////
	template <typename ClassT> class ClassBase;
	template <typename ClassT> class Class;

	typedef int (*RawFunctionType)(gmThread*);
	//////////////////////////////////////////////////////////////////////////
	template <typename ClassT>
	struct BoundObject
	{
		ClassT		*m_NativeObj;
		gmTableObject	*m_Table;
		bool IsNative() const { return m_Native; }
		void SetNative(bool _b) { m_Native = _b; }
		BoundObject(ClassT *o) : m_NativeObj(o), m_Table(0), m_Native(false)  {}
	private:
		bool		m_Native;
	};
	//////////////////////////////////////////////////////////////////////////
#define CHECKTYPE_ARG(argnum) \
	typedef typename FunctionTraits<Fn>::arg##argnum##_type trait_arg##argnum##type; \
	typedef typename Meta::Argify<trait_arg##argnum##type>::type arg##argnum##_type; \
	arg##argnum##_type arg##argnum; \
	if(GetFromGMType<typename Meta::Strip<trait_arg##argnum##type>::type>(a_thread, argnum, arg##argnum)==GM_EXCEPTION) \
		return GM_EXCEPTION;
	//////////////////////////////////////////////////////////////////////////
#define CHECKTHIS \
	typedef typename FunctionTraits<Fn>::Class_Type Class_Type; \
	Class_Type *obj = NULL; \
	if(GetThisGMType<Class_Type>(a_thread, obj)==GM_EXCEPTION) \
		return GM_EXCEPTION;
	//////////////////////////////////////////////////////////////////////////
#define CHECKTHISOP \
	typedef typename FunctionTraits<Fn>::Class_Type Class_Type; \
	Class_Type *obj = NULL; \
	if(GetFromGMType(a_thread, 0, obj)==GM_EXCEPTION) \
	return GM_EXCEPTION;
	//////////////////////////////////////////////////////////////////////////
#define CHECKOPERAND \
	typedef typename Meta::Strip<typename FunctionTraits<Fn>::arg0_type>::type operand_type; \
	operand_type operand; \
	if(GetFromGMType<operand_type>(a_thread, 1, operand)==GM_EXCEPTION) \
	return GM_EXCEPTION;
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	int GetThisGMType(gmThread *a_thread, T *&a_var)
	{
		gmType paramType = ClassBase<T>::ClassType();

		const gmVariable *pThis = a_thread->GetThis();
		if(pThis->m_type != ClassBase<T>::ClassType())
		{
			gmType parent = a_thread->GetMachine()->GetTypeParent(pThis->m_type);
			while(parent && parent != ClassBase<T>::ClassType())
				parent = a_thread->GetMachine()->GetTypeParent(parent);
			if(parent)
				paramType = pThis->m_type;
			else
				GM_ASSERT(paramType == ClassBase<T>::ClassType());
		}
		BoundObject<T> *bo = static_cast<BoundObject<T>*>(pThis->GetUserSafe(paramType));
		GM_ASSERT(paramType != GM_NULL && bo && "Invalid Parameter type");
		if(!bo || !bo->m_NativeObj)
		{
			GM_EXCEPTION_MSG("Script function on null %s object", ClassBase<T>::ClassName());
			return GM_EXCEPTION;
		}
		a_var = bo->m_NativeObj;
		return GM_OK;
	}
	//////////////////////////////////////////////////////////////////////////
	// Paramater decoding
	template<typename T>
	inline int GetFromGMType(gmThread *a_thread, int idx, T &a_var)
	{
		if(a_thread->ParamType(idx) == ClassBase<T>::ClassType())
		{
			BoundObject<T> *bo = static_cast<BoundObject<T>*>(a_thread->ParamUser(idx));
			a_var = *bo->m_NativeObj;
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// Trace up the hierarchy to see if the cast is for a base type.
		gmMachine *m = a_thread->GetMachine();
		gmType baseType = m->GetTypeParent(a_thread->ParamType(idx));
		while(baseType)
		{
			if(baseType == ClassBase<T>::ClassType())
			{
				BoundObject<T> *bo = static_cast<BoundObject<T>*>(a_thread->ParamUser(idx));
				a_var = *bo->m_NativeObj; // COPY!
				return GM_OK;
			}
			baseType = m->GetTypeParent(baseType);
		}
		//////////////////////////////////////////////////////////////////////////
		GM_EXCEPTION_MSG("expecting param %d as %s, got %s",
			idx,
			ClassBase<T>::ClassName(),
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(idx)));
		return GM_EXCEPTION;
	}
	template<typename T>
	inline int GetFromGMType(gmThread *a_thread, int idx, T *&a_var)
	{
		if(a_thread->ParamType(idx) == ClassBase<T>::ClassType())
		{
			BoundObject<T> *bo = static_cast<BoundObject<T>*>(a_thread->ParamUser(idx));
			a_var = bo->m_NativeObj;
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// Trace up the hierarchy to see if the cast is for a base type.
		gmMachine *m = a_thread->GetMachine();
		gmType baseType = m->GetTypeParent(a_thread->ParamType(idx));
		while(baseType)
		{
			if(baseType == Class<T>::ClassType())
			{
				BoundObject<T> *bo = static_cast<BoundObject<T>*>(a_thread->ParamUser(idx));
				a_var = bo->m_NativeObj;
				return GM_OK;
			}
			baseType = m->GetTypeParent(baseType);
		}
		//////////////////////////////////////////////////////////////////////////
		GM_EXCEPTION_MSG("expecting param %d as %s, got %s",
			idx,
			ClassBase<T>::ClassName(),
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(idx)));
		return GM_EXCEPTION;
	}
	template<>
	inline int GetFromGMType<bool>(gmThread *a_thread, int idx, bool &a_var)
	{
		GM_CHECK_INT_PARAM(v,idx);
		a_var = v!=0;
		return GM_OK;
	}
	template<>
	inline int GetFromGMType<int>(gmThread *a_thread, int idx, int &a_var)
	{
		GM_CHECK_INT_PARAM(v,idx);
		a_var = v;
		return GM_OK;
	}
	template<>
	inline int GetFromGMType<float>(gmThread *a_thread, int idx, float &a_var)
	{
		GM_CHECK_FLOAT_OR_INT_PARAM(v,idx);
		a_var = v;
		return GM_OK;
	}
	template<>
	inline int GetFromGMType<const char*>(gmThread *a_thread, int idx, const char *&a_var)
	{
		GM_CHECK_STRING_PARAM(v,idx);
		a_var = v;
		return GM_OK;
	}
	template<>
	inline int GetFromGMType<std::string>(gmThread *a_thread, int idx, std::string &a_var)
	{
		GM_CHECK_STRING_PARAM(v,idx);
		a_var = v;
		return GM_OK;
	}
#if(GM_USE_VECTOR3_STACK)
	template<>
	inline int GetFromGMType<float*>(gmThread *a_thread, int idx, float *&a_var)
	{
		GM_CHECK_VECTOR_PARAM(v,idx);
		a_var[0] = v.x;
		a_var[1] = v.y;
		a_var[2] = v.z;
		return GM_OK;
	}
	template<>
	inline int GetFromGMType<Vec3>(gmThread *a_thread, int idx, Vec3 &a_var)
	{
		GM_CHECK_VECTOR_PARAM(v,idx);
		a_var[0] = v.x;
		a_var[1] = v.y;
		a_var[2] = v.z;
		return GM_OK;
	}
#endif
	// and so forth for all the types you want to handle
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
#ifdef _WIN32
	__declspec( noreturn ) inline gmVariable ToGmVar( T& ); /* LINKER ERROR! */
#else
#if 0 //karin: clang mark all overload functions as noreturn
    __attribute__((noreturn))
#endif
    inline gmVariable ToGmVar(T&) { throw std::logic_error("[[ noreturn ]] gmVariable ToGmVar(T&)"); }
#endif
	template <>
	inline gmVariable ToGmVar<bool>(bool &a_var)
	{
		return gmVariable(a_var?1:0);
	}
	template <>
	inline gmVariable ToGmVar<short>( short &a_var )
	{
		return gmVariable( (int)a_var );
	}
	template <>
	inline gmVariable ToGmVar<unsigned short>( unsigned short &a_var )
	{
		return gmVariable( (int)a_var );
	}
	template <>
	inline gmVariable ToGmVar<int>(int &a_var)
	{
		return gmVariable(a_var);
	}
	template <>
	inline gmVariable ToGmVar<unsigned int>( unsigned int &a_var )
	{
		return gmVariable( (int)a_var );
	}
	template <>
	inline gmVariable ToGmVar<float>(float &a_var)
	{
		return gmVariable(a_var);
	}
#if(GM_USE_VECTOR3_STACK)
	template <>
	inline gmVariable ToGmVar<float*>(float *&a_var)
	{
		gmVec3Data v3 = { a_var[0],a_var[1],a_var[2] };
		return gmVariable(v3);
	}
#endif
	//////////////////////////////////////////////////////////////////////////
	// Return values
	template<typename T>
	inline int PushReturnToGM(gmThread *a_thread, T arg)
	{
		if(ClassBase<T>::ClassType() != GM_NULL)
		{
			BoundObject<T> *bo = new BoundObject<T>(new T(arg));
			a_thread->PushNewUser(bo, ClassBase<T>::ClassType());
			return GM_OK;
		}
		else
		{
			const char *ArgTypeName = TypeName<T>();
			GM_EXCEPTION_MSG("Unknown Return Type: %s",ArgTypeName?ArgTypeName:"unknown");
			return GM_EXCEPTION;
		}
	}
	template<>
	inline int PushReturnToGM<bool>(gmThread *a_thread, bool arg)
	{
		a_thread->PushInt(arg?1:0);
		return GM_OK;
	}
	template<>
	inline int PushReturnToGM<int>(gmThread *a_thread, int arg)
	{
		a_thread->PushInt(arg);
		return GM_OK;
	}
	template<>
	inline int PushReturnToGM<float>(gmThread *a_thread, float arg)
	{
		a_thread->PushFloat(arg);
		return GM_OK;
	}
	template<>
	inline int PushReturnToGM<const char*>(gmThread *a_thread, const char* arg)
	{
		a_thread->PushNewString(arg);
		return GM_OK;
	}
	template<>
	inline int PushReturnToGM<std::string>(gmThread *a_thread, std::string arg)
	{
		a_thread->PushNewString(arg.c_str());
		return GM_OK;
	}
#if(GM_USE_VECTOR3_STACK)
	template<>
	inline int PushReturnToGM<Vec3>(gmThread *a_thread, Vec3 arg)
	{
		a_thread->PushVector(arg);
		return GM_OK;
	}
	template<>
	inline int PushReturnToGM<float*>(gmThread *a_thread, float *arg)
	{
		a_thread->PushVector(arg);
		return GM_OK;
	}
#endif
	//////////////////////////////////////////////////////////////////////////
	template<typename Fn, int>
	struct GMExportStruct {};

	template<typename Fn>
	struct GMExportStruct<Fn, 0>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			fn();
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn();
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			(obj->*fn)();
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)();
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportStruct<Fn, 1>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			fn(arg0);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn(arg0);
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			(obj->*fn)(arg0);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(arg0);
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportStruct<Fn, 2>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			fn(arg0, arg1);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn(arg0, arg1);
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			(obj->*fn)(arg0, arg1);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(arg0, arg1);
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportStruct<Fn, 3>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			fn(arg0, arg1, arg2);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn(arg0, arg1, arg2);
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			(obj->*fn)(arg0, arg1, arg2);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(arg0, arg1, arg2);
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportStruct<Fn, 4>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			fn(arg0, arg1, arg2, arg3);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn(arg0, arg1, arg2, arg3);
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			(obj->*fn)(arg0, arg1, arg2, arg3);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(arg0, arg1, arg2, arg3);
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportStruct<Fn, 5>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			CHECKTYPE_ARG(4);
			fn(arg0, arg1, arg2, arg3, arg4);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			CHECKTYPE_ARG(4);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn(arg0, arg1, arg2, arg3, arg4);
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			CHECKTYPE_ARG(4);
			(obj->*fn)(arg0, arg1, arg2, arg3, arg4);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHIS;
			CHECKTYPE_ARG(0);
			CHECKTYPE_ARG(1);
			CHECKTYPE_ARG(2);
			CHECKTYPE_ARG(3);
			CHECKTYPE_ARG(4);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(arg0, arg1, arg2, arg3, arg4);
			return PushReturnToGM(a_thread, ret);
		}
	};
	//...
	//////////////////////////////////////////////////////////////////////////
	template<typename Fn, int>
	struct GMExportOpStruct {};
	template<typename Fn>
	struct GMExportOpStruct<Fn, 0>
	{
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			fn();
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<false>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = fn();
			return PushReturnToGM(a_thread, ret);
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHISOP;
			(obj->*fn)();
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHISOP;
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)();
			return PushReturnToGM(a_thread, ret);
		}
	};
	template<typename Fn>
	struct GMExportOpStruct<Fn, 1>
	{
		//////////////////////////////////////////////////////////////////////////
		// member function, no return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<true>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHISOP;
			CHECKOPERAND;
			(obj->*fn)(operand);
			return GM_OK;
		}
		//////////////////////////////////////////////////////////////////////////
		// member function, one return value
		static int Call(gmThread *a_thread, Fn fn, Meta::BoolToType<false>, Meta::BoolToType<true>)
		{
			GM_CHECK_NUM_PARAMS(FunctionTraits<Fn>::Arity);
			CHECKTHISOP;
			CHECKOPERAND;
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			ret_type ret = (obj->*fn)(operand);
			return PushReturnToGM(a_thread, ret);
		}
	};
	//////////////////////////////////////////////////////////////////////////
	namespace GMProperty
	{
		template<typename T>
		static int Get(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			T *var = a_static ? (T*)a_offset : (T*)((char*)p + a_offset);
			a_operands[0].Set(a_thread->GetMachine(),*var);
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Get<gmTableObject*>(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			gmTableObject **var = a_static ? (gmTableObject**)a_offset : (gmTableObject**)((char*)p + a_offset);
			if(*var)
				a_operands[0].Set(a_thread->GetMachine(),*var);
			else
				a_operands[0].Nullify();
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Get<std::string>(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			const std::string *str = a_static ? (std::string*)a_offset : (const std::string*)((char*)p + a_offset);
			a_operands[0].SetString(a_thread->GetMachine(), str->c_str());
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Get< gmGCRoot<gmStringObject> >(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			gmGCRoot<gmStringObject> *str = a_static ? (gmGCRoot<gmStringObject>*)a_offset : (gmGCRoot<gmStringObject>*)((char*)p + a_offset);
			if(str && *str)
				a_operands[0].SetString(*str);
			else
				a_operands[0].Nullify();
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline int Set(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			T *var = a_static ? (T*)a_offset : (T*)((char*)p + a_offset);
			a_operands[1].Get(a_thread->GetMachine(),*var);
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Set<gmTableObject*>(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			gmTableObject **tbl = a_static ? (gmTableObject**)a_offset : (gmTableObject**)((char*)p + a_offset);
			*tbl = a_operands[1].GetTableObjectSafe();
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Set<std::string>(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			std::string *str = a_static ? (std::string*)a_offset : (std::string *)((char*)p + a_offset);
			*str = a_operands[1].GetCStringSafe();
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline int Set< gmGCRoot<gmStringObject> >(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			gmGCRoot<gmStringObject> *str = a_static ? (gmGCRoot<gmStringObject>*)a_offset : (gmGCRoot<gmStringObject> *)((char*)p + a_offset);
			str->Set(a_operands[1].GetStringObjectSafe(),a_thread->GetMachine());
			return 1;
		}
		//////////////////////////////////////////////////////////////////////////
		template<typename T>
		static void TraceProperty(void *p, gmMachine *a_machine, gmGarbageCollector*a_gc, size_t a_offset, bool a_static)
		{
		}
		//////////////////////////////////////////////////////////////////////////
		template<>
		inline void TraceProperty<gmTableObject*>(void *p, gmMachine *a_machine, gmGarbageCollector*a_gc, size_t a_offset, bool a_static)
		{
			gmTableObject **var = a_static ? (gmTableObject**)a_offset : (gmTableObject**)((char*)p + a_offset);
			if(*var)
				a_gc->GetNextObject(*var);
		}
		//////////////////////////////////////////////////////////////////////////
		// Bitfield stuff.
		template<typename T>
		static int GetBitField(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			T *var = a_static ? (T*)a_offset : (T*)((char*)p + a_offset);
			a_operands[0].SetInt((*var)&(1<<a_bit)?1:0);
			return 1;
		}
		template<typename T>
		static int SetBitField(void *p, gmThread *a_thread, gmVariable *a_operands, size_t a_offset, size_t a_bit, bool a_static)
		{
			T *var = a_static ? (T*)a_offset : (T*)((char*)p + a_offset);
			const int bitfield = a_operands[1].GetInt();
			if(bitfield)
				*var |= (1<<a_bit);
			else
				*var &= ~(1<<a_bit);
			return 1;
		}
	};
	//////////////////////////////////////////////////////////////////////////
	template <typename Fn>
	struct GMExportFunctor : public gmObjFunctor
	{
		int operator()(gmThread *a_thread)
		{
			using namespace Meta;
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			return GMExportStruct<Fn, FunctionTraits<Fn>::Arity>::Call(
				a_thread,
				m_Function,
				BoolToType<IsSame<ret_type, void>::value>(), // void return
				BoolToType<IsMemberFunction<Fn>::value>()); // member function
		}
		GMExportFunctor(Fn a_fn) : m_Function(a_fn) {}
	private:
		Fn				m_Function;
	};
	//////////////////////////////////////////////////////////////////////////
	template <typename Fn>
	struct GMExportOpFunctor : public gmObjFunctor
	{
		int operator()(gmThread *a_thread)
		{
			using namespace Meta;
			typedef typename FunctionTraits<Fn>::return_type ret_type;
			return GMExportOpStruct<Fn, FunctionTraits<Fn>::Arity>::Call(
				a_thread,
				m_Function,
				BoolToType<IsSame<ret_type, void>::value>(), // void return
				BoolToType<IsMemberFunction<Fn>::value>()); // member function
		}
		GMExportOpFunctor(Fn a_fn) : m_Function(a_fn) {}
	private:
		Fn				m_Function;
	};
	//////////////////////////////////////////////////////////////////////////
	template <typename Fn>
	void RegisterFunction(gmMachine *a_machine, Fn a_fn, const char *a_funcname, const char *a_asTable = NULL)
	{
		gmFunctionEntry fn = {0,0,0};
		fn.m_name = a_funcname;
		fn.m_function = 0;
		fn.m_functor = new GMExportFunctor<Fn>(a_fn);
		a_machine->RegisterLibrary(&fn, 1, a_asTable, false);
	}
	//////////////////////////////////////////////////////////////////////////
	class Global
	{
	public:
		template <typename Fn>
		Global &func(Fn a_fn, const char *a_funcname)
		{
			gmFunctionObject* funcObj = m_Machine->AllocFunctionObject();
			funcObj->m_cFunction = 0;
			funcObj->m_cFunctor = new GMExportFunctor<Fn>(a_fn);
			m_Table->Set(m_Machine,a_funcname,gmVariable(funcObj));
			return *this;
		}
		Global &func(RawFunctionType a_fn, const char *a_funcname)
		{
			gmFunctionObject* funcObj = m_Machine->AllocFunctionObject();
			funcObj->m_cFunction = a_fn;
			funcObj->m_cFunctor = 0;
			m_Table->Set(m_Machine,a_funcname,gmVariable(funcObj));
			return *this;
		}
		template <typename T>
		Global &var(T a_var, const char *a_varname)
		{
			m_Table->Set(m_Machine,a_varname,ToGmVar(a_var));
			return *this;
		}
		template <typename T>
		Global &var(T *a_var, const char *a_varname)
		{
			GM_ASSERT(ClassBase<T>::ClassType() != GM_NULL);
			if(ClassBase<T>::ClassType() != GM_NULL)
			{
				gmGCRoot<gmUserObject> r =Class<T>::WrapObject(m_Machine,a_var,false);
				m_Table->Set(m_Machine,a_varname,gmVariable(r));
			}
			return *this;
		}
		Global(gmMachine *a_machine, const char *_tablename = 0)
			: m_Machine(a_machine)
			, m_Table(0)
			, m_TableName(_tablename)
		{
			if(m_TableName)
			{
				m_Table = m_Machine->GetGlobals()->Get(m_Machine,m_TableName).GetTableObjectSafe();
				if(!m_Table)
				{
					DisableGCInScope gcEn(m_Machine);

					m_Table = m_Machine->AllocTableObject();
					m_Machine->GetGlobals()->Set(m_Machine,m_TableName,gmVariable(m_Table));
				}
			}
			else
				m_Table = m_Machine->GetGlobals();
		}
	private:
		gmMachine		*m_Machine;
		gmTableObject	*m_Table;
		const char		*m_TableName;
	};
	//////////////////////////////////////////////////////////////////////////

	struct Null {};
	struct Call {};
	struct CallAsync {};

	class Function
	{
	public:
		Function(gmMachine *_machine, const char *_funcname, const gmVariable &_this = gmVariable::s_null)
			: m_Machine(_machine)
			, m_Function(NULL)
			, m_Thread(NULL)
			, m_ParamCount(0)
			, m_This(_this)
			, m_ReturnVal(gmVariable::s_null)
			, m_ThreadId(GM_INVALID_THREAD)
		{
			gmVariable v = _machine->Lookup(_funcname);
			m_Function = v.GetFunctionObjectSafe();

			if(m_Function && m_Machine)
			{
				m_Thread = m_Machine->CreateThread();   // Create thread for func to run on
				m_Thread->Push(m_This);					// this
				m_Thread->PushFunction(m_Function);		// function
			}
		}

		gmVariable operator<<(const Call& run)
		{
			return _Call(false);
		}
		void operator<<(const CallAsync& run)
		{
			_Call(true);
		}

	//private:
		gmMachine			*m_Machine;
		gmFunctionObject	*m_Function;
		gmThread			*m_Thread;
		int					m_ParamCount;
		gmVariable			m_This;
		gmVariable			m_ReturnVal;
		int					m_ThreadId;
	private:
		gmVariable _Call(bool _async = false)
		{
			if(m_Thread)
			{
				int state = m_Thread->PushStackFrame(m_ParamCount);
				if(state != gmThread::KILLED) // Can be killed immedialy if it was a C function
				{
					if(_async)
					{
						state = m_Thread->GetState();
					}
					else
					{
						state = m_Thread->Sys_Execute(&m_ReturnVal);
					}
				}
				else
				{
					// Was a C function call, grab return var off top of stack
					m_ReturnVal = *(m_Thread->GetTop() - 1);
					m_Machine->Sys_SwitchState(m_Thread, gmThread::KILLED);
				}

				// If we requested a thread Id
				if(state != gmThread::KILLED)
					m_ThreadId = m_Thread->GetId();
				else
					m_ThreadId = GM_INVALID_THREAD;

				if(state == gmThread::KILLED)
				{
					m_Thread = NULL; // Thread has exited, no need to remember it.
				}
			}
			return m_ReturnVal;
		}
	};

	template <typename T>
	inline Function& operator<<(Function& call, T &value)
	{
		if(call.m_Thread)
		{
			GM_ASSERT(ClassBase<T>::ClassType() != GM_NULL);
			if(ClassBase<T>::ClassType() != GM_NULL)
			{
				Class<T>::PushObject(call.m_Thread,value,true);
				++call.m_ParamCount;
			}
		}
	}
	inline Function& operator<<(Function& call, Null&)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushNull();
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, float value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushFloat(value);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, bool value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushInt(value?1:0);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, int value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushInt(value);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, const char* value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushNewString(value);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, gmVariable value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->Push(value);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, gmFunctionObject *value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushFunction(value);
			++call.m_ParamCount;
		}
		return call;
	}
	inline Function& operator<<(Function& call, gmTableObject *value)
	{
		if(call.m_Thread)
		{
			call.m_Thread->PushTable(value);
			++call.m_ParamCount;
		}
		return call;
	}
	//////////////////////////////////////////////////////////////////////////
	class TableConstructor
	{
	public:
		gmGCRoot<gmTableObject> Top()
		{
			return m_TableStack[m_StackTop];
		}
		gmGCRoot<gmTableObject> Root()
		{
			return m_TableStack[0];
		}
		void Push(const char *_tablename)
		{
			GM_ASSERT(m_StackTop < TableStackSize);
			if ( m_StackTop < TableStackSize ) {
				m_StackTop++;
				m_TableStack[m_StackTop].Set(m_Machine->AllocTableObject(),m_Machine);
				m_TableStack[m_StackTop-1]->Set(m_Machine,_tablename,gmVariable(m_TableStack[m_StackTop]));
			}
		}
		void Pop()
		{
			if(m_StackTop>0)
				--m_StackTop;
		}
		TableConstructor(gmMachine *a_machine)
			: m_Machine(a_machine)
			, m_StackTop(0)
		{
			m_TableStack[m_StackTop].Set(m_Machine->AllocTableObject(),m_Machine);
		}
	private:
		gmMachine *m_Machine;

		enum { TableStackSize = 64 };
		gmGCRoot<gmTableObject> m_TableStack[TableStackSize];

		int m_StackTop;
	};
};

#include "gmbinder2_class.h"

//////////////////////////////////////////////////////////////////////////
// autoexp.dat - Copy and paste the following(minus the comment block its in),
// under the [Visualizer] section of the file
// C:\Program Files\Microsoft Visual Studio 8\Common7\Packages\Debugger\autoexp.dat
/*

;------------------------------------------------------------------------------
;  gmTableNode
;------------------------------------------------------------------------------
gmTableNode{
	preview
		(
#(
		"(",
		$e.m_key,
		",",
		$e.m_value ,
		")"
		)
		)
		children
		(
#(
Key		: $e.m_key,
Value	: $e.m_value,
		  )
		  )
}

;------------------------------------------------------------------------------
;  gmTableObject
;------------------------------------------------------------------------------
gmTableObject{
	preview
		(
#(
		"table[", $c.m_slotsUsed, "]",
		)
		)
		children
		(
#(
#array
		(
expr: $c.m_nodes[$i],
size: $c.m_tableSize,
	  ) : #(
#array (
expr: $e,
size: $e.m_key.m_type != 0
	  ) : $e
	  )
	  )
	  )
}

;------------------------------------------------------------------------------
;  gmFunctionObject
;------------------------------------------------------------------------------
gmFunctionObject{
	preview
		(
#if( $c.m_cFunction != 0 )
		(
#("function(native) = ", $c.m_cFunction )
		)
#elif( $c.m_debugInfo != 0 )
		(
#("function(script) = ", [ $c.m_debugInfo->m_debugName,s ] )
		)
#else
		(
#("function = <no debug name, use gmMachine::SetDebugMode(true)>")
		)
		)
		children
		(
#(
		[actual members]			: [$e,!]
	)
		)
}

;------------------------------------------------------------------------------
;  gmStringObject
;------------------------------------------------------------------------------
gmStringObject{
	preview
		(
#("string = ", $c.m_string )
		)
		children
		(
#(
		[actual members]			: [$e,!]
	)
		)
}

;------------------------------------------------------------------------------
;  gmVariable
;------------------------------------------------------------------------------
gmVariable{
	preview
		(
#switch ($c.m_type)
#case 0
		(
#("<null>")
		)
#case 1
		(
#("int = ", $c.m_value.m_int)
		)
#case 2
		(
#("float = ", $c.m_value.m_float)
		)
#case 3
		(
#( "vector = (", $c.m_value.m_vec3.x ,$c.m_value.m_vec3.y ,$c.m_value.m_vec3.z, ")" )
		)
#case 4
		(
#("entity = ", $c.m_value.m_hndl)
		)
#case 5
		(
#( "string = ", [ ((gmStringObject *)$c.m_value.m_ref)->m_string,s ] )
		)
#case 6
		(
#( "table[", ((gmTableObject*)$c.m_value.m_ref)->m_slotsUsed, "]" )
		)
#case 7
		(
#if( ((gmFunctionObject*)$c.m_value.m_ref)->m_cFunction != 0 )
		(
#( "function(native) = ", ((gmFunctionObject*)$c.m_value.m_ref)->m_cFunction )
		)
#elif( ((gmFunctionObject*)$c.m_value.m_ref)->m_debugInfo != 0 )
		(
#( "function(script) = ", [ ((gmFunctionObject*)$c.m_value.m_ref)->m_debugInfo->m_debugName,s ] )
		)
#else
		(
#("function = <no debug name, use gmMachine::SetDebugMode(true)>")
		)
		)
#default
		(
#("user = ", $c.m_type )
		)

		)

		children
		(
#switch ($c.m_type)
#case 0
		(
#(
		[actual members]		: [$e,!]
	)
		)
#case 1
		(
#(
		[actual members]		: [$e,!],
		int						: $c.m_value.m_int

		)
		)
#case 2
		(
#(
		[actual members]		: [$e,!],
		float					: $c.m_value.m_float
		)
		)
#case 3
		(
#(
		[actual members]		: [$e,!],
vector					: $c.m_value.m_vec3
						  )
						  )
#case 4
						  (
#(
						  [actual members]		: [$e,!],
entity					: $c.m_value.m_hndl
						  )
						  )
#case 5
						  (
#(
						  [actual members]		: [$e,!],
						  [string]				: [((gmStringObject *)$c.m_value.m_ref)->m_string,s]
	)
		)
#case 6
		(
#(
		[actual members]		: [$e,!],
		[table]					: ((gmTableObject *)$c.m_value.m_ref)
		)
		)
#case 7
		(
#(
		[actual members]		: [$e,!],
		[function]				: ((gmFunctionObject *)$c.m_value.m_ref)
		)
		)
#default
		(
#(
		[actual members]		: [$e,!],
		[user type]				: $c.m_type
		)
		)
		)
}

;------------------------------------------------------------------------------
;  gmThread
;------------------------------------------------------------------------------
gmThread{
	preview
		(
#( "this = (", $c.m_stack[$c.m_base - 2], ")" )
		)
		children
		(
#(
		[actual members]		: [$e,!],
#array
		(
expr :	($c.m_stack)[$c.m_base + $i],
size :  $c.m_numParameters
		)
		)
		)
}

;------------------------------------------------------------------------------
;  gmMachine
;------------------------------------------------------------------------------
gmMachine{
	preview
		(
#( "CurrentMemoryUsage = (", $c.m_currentMemoryUsage, ")" )
		)
		children
		(
#(
		[actual members]		: [$e,!],
		[CurrentMemoryUsage]	: [ $c.m_currentMemoryUsage ],
		[HardMemoryLimit]		: [ $c.m_desiredByteMemoryUsageHard ],
		[SoftMemoryLimit]		: [ $c.m_desiredByteMemoryUsageSoft ],
		[AutoSizeMemory]		: [ $c.m_autoMem ],
		[GC_FullCollects]		: [ $c.m_statsGCFullCollect ],
		[GC_IncrementalCollects]: [ $c.m_statsGCIncCollect ],
		[GC_Warnings]			: [ $c.m_statsGCWarnings ],
		)
		)
}

*/
//////////////////////////////////////////////////////////////////////////

#endif // _GMBINDER_H_
