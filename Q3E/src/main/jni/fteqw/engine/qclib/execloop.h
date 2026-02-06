//qc execution code.
//we have two conditions.
//one allows us to debug and trace through our code, the other doesn't.

//hopefully, the compiler will do a great job at optimising this code for us, where required.
//if it dosn't, then bum.

//the general overhead should be reduced significantly, and I would be supprised if it did run slower.

//run away loops are checked for ONLY on gotos and function calls. This might give a poorer check, but it will run faster overall.

//Appears to work fine.

#if INTSIZE == 16
#define reeval reeval16
#define pr_statements pr_statements16
#define fakeop fakeop16
#define dstatement_t dstatement16_t
#define sofs signed short
#elif INTSIZE == 32
#define reeval reeval32
#define pr_statements pr_statements32
#define fakeop fakeop32
#define dstatement_t dstatement32_t
#define sofs signed int
#elif INTSIZE == 24
#error INTSIZE should be set to 32.
#else
#error Bad cont size
#endif

#define ENGINEPOINTER(p) ((char*)(p) - progfuncs->funcs.stringtable)
#define QCPOINTER(p) (eval_t *)(p->_int+progfuncs->funcs.stringtable)
#define QCPOINTERM(p) (eval_t *)((p)+progfuncs->funcs.stringtable)
#define QCPOINTERWRITEFAIL(p,sz) ((unsigned int)(p)-1 >= prinst.addressableused-1-(sz))	//disallows null writes
#define QCPOINTERREADFAIL(p,sz) ((unsigned int)(p) >= prinst.addressableused-(sz))		//permits null reads



#define QCFAULT return (prinst.pr_xstatement=(st-pr_statements)-1),PR_HandleFault
#define EVAL_FLOATISTRUE(ev) ((ev)->_int & 0x7fffffff) //mask away sign bit. This avoids using denormalized floats.

#define A_RSHIFT_I(x,y) ((x < 0) ? ~(~(x) >> (y)) : ((x) >> (y)))	//C leaves it undefined whether signed rshift is arithmatic or logical. gcc should be smart enough to fold this to the proper signed instruction at least on x86.

#ifdef __GNUC__
#define errorif(x) if(__builtin_expect(x,0))
#else
#define errorif(x) if(x)
#endif

//rely upon just st
{
#ifdef DEBUGABLE
	s = st-pr_statements;
	s+=1;

	errorif (prinst.watch_ptr && prinst.watch_ptr->_int != prinst.watch_old._int)
	{
		//this will fire on the next instruction after the variable got changed.
		prinst.pr_xstatement = s;
		if (current_progstate->linenums)
			externs->Printf("^b^3Watch point hit in %s:%u, \"%s\" changed", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), current_progstate->linenums[s-1], prinst.watch_name);
		else
			externs->Printf("^b^3Watch point hit in %s, \"%s\" changed", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), prinst.watch_name);
		switch(prinst.watch_type)
		{
		case ev_float:
			externs->Printf(" from %g to %g", prinst.watch_old._float, prinst.watch_ptr->_float);
			break;
		case ev_vector:
			externs->Printf(" from '%g %g %g' to '%g %g %g'", prinst.watch_old._vector[0], prinst.watch_old._vector[1], prinst.watch_old._vector[2], prinst.watch_ptr->_vector[0], prinst.watch_ptr->_vector[1], prinst.watch_ptr->_vector[2]);
			break;
		default:
			externs->Printf(" from %i to %i", prinst.watch_old._int, prinst.watch_ptr->_int);
			break;
		case ev_entity:
			externs->Printf(" from %i(%s) to %i(%s)", prinst.watch_old._int, PR_GetEdictClassname(progfuncs, prinst.watch_old._int), prinst.watch_ptr->_int, PR_GetEdictClassname(progfuncs, prinst.watch_ptr->_int));
			break;
		case ev_function:
		case ev_string:
			externs->Printf(", now set to %s", PR_ValueString(progfuncs, prinst.watch_type, prinst.watch_ptr, false));
			break;
		}
		externs->Printf(".\n");
		prinst.watch_old = *prinst.watch_ptr;
//		prinst.watch_ptr = NULL;
		progfuncs->funcs.debug_trace=DEBUG_TRACE_INTO;	//this is what it's for

		s=ShowStep(progfuncs, s, "Watchpoint hit", false);
	}
	else if (progfuncs->funcs.debug_trace)
		s=ShowStep(progfuncs, s, NULL, false);
	st = pr_statements + s;
	prinst.pr_xfunction->profile+=1;

	op = (progfuncs->funcs.debug_trace?(st->op & ~0x8000):st->op);
reeval:
#else
	st++;
	op = st->op;
#endif

	safeswitch ((enum qcop_e)op)
	{
	case OP_ADD_F:
		OPC->_float = OPA->_float + OPB->_float;
		break;
	case OP_ADD_V:
		OPC->_vector[0] = OPA->_vector[0] + OPB->_vector[0];
		OPC->_vector[1] = OPA->_vector[1] + OPB->_vector[1];
		OPC->_vector[2] = OPA->_vector[2] + OPB->_vector[2];
		break;

	case OP_SUB_F:
		OPC->_float = OPA->_float - OPB->_float;
		break;
	case OP_SUB_V:
		OPC->_vector[0] = OPA->_vector[0] - OPB->_vector[0];
		OPC->_vector[1] = OPA->_vector[1] - OPB->_vector[1];
		OPC->_vector[2] = OPA->_vector[2] - OPB->_vector[2];
		break;

	case OP_MUL_F:
		OPC->_float = OPA->_float * OPB->_float;
		break;
	case OP_MUL_V:
		OPC->_float = OPA->_vector[0]*OPB->_vector[0]
				+ OPA->_vector[1]*OPB->_vector[1]
				+ OPA->_vector[2]*OPB->_vector[2];
		break;
	case OP_MUL_FV:
		tmpf = OPA->_float;
		OPC->_vector[0] = tmpf * OPB->_vector[0];
		OPC->_vector[1] = tmpf * OPB->_vector[1];
		OPC->_vector[2] = tmpf * OPB->_vector[2];
		break;
	case OP_MUL_VF:
		tmpf = OPB->_float;
		OPC->_vector[0] = tmpf * OPA->_vector[0];
		OPC->_vector[1] = tmpf * OPA->_vector[1];
		OPC->_vector[2] = tmpf * OPA->_vector[2];
		break;

	case OP_DIV_F:
/*		errorif (OPB->_float == 0)
		{
			prinst.pr_xstatement = st-pr_statements;
			externs->Printf ("Division by 0 in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
			PR_StackTrace (&progfuncs->funcs, 1);
			OPC->_float = 0.0;
		}
		else
*/			OPC->_float = OPA->_float / OPB->_float;
		break;
	case OP_DIV_VF:
		tmpf = OPB->_float;
/*		errorif (!tmpf)
		{
			prinst.pr_xstatement = st-pr_statements;
			externs->Printf ("Division by 0 in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
			PR_StackTrace (&progfuncs->funcs, 1);
		}
*/
		OPC->_vector[0] = OPA->_vector[0] / tmpf;
		OPC->_vector[1] = OPA->_vector[1] / tmpf;
		OPC->_vector[2] = OPA->_vector[2] / tmpf;
		break;

	case OP_BITAND_F:
		OPC->_float = (float)((int)OPA->_float & (int)OPB->_float);
		break;

	case OP_BITOR_F:
		OPC->_float = (float)((int)OPA->_float | (int)OPB->_float);
		break;


	case OP_GE_F:
		OPC->_float = (float)(OPA->_float >= OPB->_float);
		break;
	case OP_GE_I:
		OPC->_int = (int)(OPA->_int >= OPB->_int);
		break;
	case OP_GE_IF:
		OPC->_int = (int)(OPA->_int >= OPB->_float);
		break;
	case OP_GE_FI:
		OPC->_int = (int)(OPA->_float >= OPB->_int);
		break;

	case OP_LE_F:
		OPC->_float = (float)(OPA->_float <= OPB->_float);
		break;
	case OP_LE_I:
		OPC->_int = (int)(OPA->_int <= OPB->_int);
		break;
	case OP_LE_IF:
		OPC->_int = (int)(OPA->_int <= OPB->_float);
		break;
	case OP_LE_FI:
		OPC->_int = (int)(OPA->_float <= OPB->_int);
		break;
	case OP_LE_U:
		OPC->_int = (int)(OPA->_uint <= OPB->_uint);
		break;

	case OP_GT_F:
		OPC->_float = (float)(OPA->_float > OPB->_float);
		break;
	case OP_GT_I:
		OPC->_int = (int)(OPA->_int > OPB->_int);
		break;
	case OP_GT_IF:
		OPC->_int = (int)(OPA->_int > OPB->_float);
		break;
	case OP_GT_FI:
		OPC->_int = (int)(OPA->_float > OPB->_int);
		break;

	case OP_LT_F:
		OPC->_float = (float)(OPA->_float < OPB->_float);
		break;
	case OP_LT_I:
		OPC->_int = (int)(OPA->_int < OPB->_int);
		break;
	case OP_LT_IF:
		OPC->_int = (int)(OPA->_int < OPB->_float);
		break;
	case OP_LT_FI:
		OPC->_int = (int)(OPA->_float < OPB->_int);
		break;
	case OP_LT_U:
		OPC->_int = (OPA->_uint < OPB->_uint);
		break;

	case OP_AND_F:
		//original logic
		//OPC->_float = (float)(OPA->_float && OPB->_float);
		//deal with denormalized floats by ensuring that they're not 0 (ignoring sign bit).
		//this avoids issues where the fpu treats denormalised floats as 0, or fpus that don't support denormals.
		OPC->_float = (float)(EVAL_FLOATISTRUE(OPA) && EVAL_FLOATISTRUE(OPB));
		break;
	case OP_OR_F:
		OPC->_float = (float)(EVAL_FLOATISTRUE(OPA) || EVAL_FLOATISTRUE(OPB));
		break;

	case OP_NOT_F:
		OPC->_float = (float)(!EVAL_FLOATISTRUE(OPA));
		break;
	case OP_NOT_V:
		OPC->_float = (float)(!OPA->_vector[0] && !OPA->_vector[1] && !OPA->_vector[2]);
		break;
	case OP_NOT_S:
		OPC->_float = (float)(!(OPA->string) || !*PR_StringToNative(&progfuncs->funcs, OPA->string));
		break;
	case OP_NOT_FNC:
		OPC->_float = (float)(!(OPA->function & ~0xff000000));
		break;
	case OP_NOT_ENT:
		OPC->_float = (float)(!(OPA->edict));//(PROG_TO_EDICT(progfuncs, OPA->edict) == (edictrun_t *)sv_edicts);
		break;
	case OP_NOT_I:
		OPC->_int = !OPA->_int;
		break;

	case OP_EQ_F:
		OPC->_float = (float)(OPA->_float == OPB->_float);
		break;
	case OP_EQ_IF:
		OPC->_int = (float)(OPA->_int == OPB->_float);
		break;
	case OP_EQ_FI:
		OPC->_int = (float)(OPA->_float == OPB->_int);
		break;


	case OP_EQ_V:
		OPC->_float = (float)((OPA->_vector[0] == OPB->_vector[0]) &&
					(OPA->_vector[1] == OPB->_vector[1]) &&
					(OPA->_vector[2] == OPB->_vector[2]));
		break;
	case OP_EQ_S:
		if (OPA->string==OPB->string)
			OPC->_float = true;
		else if (!OPA->string)
		{
			if (!OPB->string || !*PR_StringToNative(&progfuncs->funcs, OPB->string))
				OPC->_float = true;
			else
				OPC->_float = false;
		}
		else if (!OPB->string)
		{
			if (!OPA->string || !*PR_StringToNative(&progfuncs->funcs, OPA->string))
				OPC->_float = true;
			else
				OPC->_float = false;
		}
		else
			OPC->_float = (float)(!strcmp(PR_StringToNative(&progfuncs->funcs, OPA->string),PR_StringToNative(&progfuncs->funcs, OPB->string)));
		break;
	case OP_EQ_E:
		OPC->_float = (float)(OPA->_int == OPB->_int);
		break;
	case OP_EQ_FNC:
		OPC->_float = (float)(OPA->function == OPB->function);
		break;


	case OP_NE_F:
		OPC->_float = (float)(OPA->_float != OPB->_float);
		break;
	case OP_NE_V:
		OPC->_float = (float)((OPA->_vector[0] != OPB->_vector[0]) ||
					(OPA->_vector[1] != OPB->_vector[1]) ||
					(OPA->_vector[2] != OPB->_vector[2]));
		break;
	case OP_NE_S:
		if (OPA->string==OPB->string)
			OPC->_float = false;
		else if (!OPA->string)
		{
			if (!OPB->string || !*(PR_StringToNative(&progfuncs->funcs, OPB->string)))
				OPC->_float = false;
			else
				OPC->_float = true;
		}
		else if (!OPB->string)
		{
			if (!OPA->string || !*PR_StringToNative(&progfuncs->funcs, OPA->string))
				OPC->_float = false;
			else
				OPC->_float = true;
		}
		else
			OPC->_float = (float)(strcmp(PR_StringToNative(&progfuncs->funcs, OPA->string),PR_StringToNative(&progfuncs->funcs, OPB->string)));		
		break;
	case OP_NE_E:
		OPC->_float = (float)(OPA->_int != OPB->_int);
		break;
	case OP_NE_FNC:
		OPC->_float = (float)(OPA->function != OPB->function);
		break;

//==================
	case OP_STORE_IF:
		OPB->_float = (float)OPA->_int;
		break;
	case OP_STORE_FI:
		OPB->_int = (int)OPA->_float;
		break;
		
	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:		// integers
	case OP_STORE_S:
	case OP_STORE_I:
	case OP_STORE_FNC:		// pointers
	case OP_STORE_P:
		OPB->_int = OPA->_int;
		break;
	case OP_STORE_V:
		OPB->_vector[0] = OPA->_vector[0];
		OPB->_vector[1] = OPA->_vector[1];
		OPB->_vector[2] = OPA->_vector[2];
		break;

	//store a value to a pointer
	case OP_STOREP_IF:
		i = OPB->_int + OPC->_int*sizeof(ptr->_float);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(ptr->_float), sizeof(ptr->_float))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
			}
		}
		else
			ptr = QCPOINTERM(i);
		ptr->_float = (float)OPA->_int;
		break;
	case OP_STOREP_FI:
		i = OPB->_int + OPC->_int*sizeof(ptr->_int);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(int)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(ptr->_int), sizeof(ptr->_int))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
			}
		}
		else
			ptr = QCPOINTERM(i);
		ptr->_int = (int)OPA->_float;
		break;
	case OP_STOREP_I:
	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:		// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:		// pointers
		i = OPB->_int + OPC->_int*sizeof(ptr->_int);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(ptr->_int)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(ptr->_int), sizeof(ptr->_int))))
			{
				if (i == -1)
					break;
				if (i == 0)
					QCFAULT(&progfuncs->funcs, "bad pointer write in %s (null pointer)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
				else
					QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		ptr->_int = OPA->_int;
		break;
	case OP_STOREP_I64:		// 64bit
		i = OPB->_int + OPC->_int*sizeof(ptr->_int);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(ptr->i64)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(ptr->_int), sizeof(ptr->i64))))
			{
				if (i == -1)
					break;
				if (i == 0)
					QCFAULT(&progfuncs->funcs, "bad pointer write in %s (null pointer)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
				else
					QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		ptr->i64 = OPA->i64;
		break;
	case OP_STOREP_V:
		i = OPB->_int + (OPC->_int*sizeof(ptr->_int));
		errorif (QCPOINTERWRITEFAIL(i, sizeof(pvec3_t)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(ptr->_int), sizeof(pvec3_t))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		ptr->_vector[0] = OPA->_vector[0];
		ptr->_vector[1] = OPA->_vector[1];
		ptr->_vector[2] = OPA->_vector[2];
		break;

	case OP_STOREP_C:	//store (float) character in a string
		i = OPB->_int + (OPC->_int)*sizeof(char);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(char)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(char), sizeof(char))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		*(unsigned char *)ptr = (char)OPA->_float;
		break;
	case OP_STOREP_I8:	//store (byte) character in a string
		i = OPB->_int + (OPC->_int)*sizeof(pbyte);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(pbyte)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(pbyte), sizeof(pbyte))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		*(pbyte *)ptr = (pbyte)OPA->_int;
		break;
	case OP_STOREP_I16:	//store short to a pointer
		i = OPB->_int + (OPC->_int)*sizeof(short);
		errorif (QCPOINTERWRITEFAIL(i, sizeof(short)))
		{
			if (!(ptr=PR_GetWriteTempStringPtr(progfuncs, OPB->_int, OPC->_int*sizeof(short), sizeof(short))))
			{
				if (i == -1)
					break;
				QCFAULT(&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, prinst.addressableused);
			}
		}
		else
			ptr = QCPOINTERM(i);
		*(short *)ptr = (short)OPA->_int;
		break;

	case OP_STOREF_F:
	case OP_STOREF_I:
	case OP_STOREF_S:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid entity in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
		errorif (!ed || ed->readonly)
		{	//boot it over to the debugger
#if INTSIZE == 16
			ddef16_t *d = ED_GlobalAtOfs16(progfuncs, st->a);
#else
			ddef32_t *d = ED_GlobalAtOfs32(progfuncs, st->a);
#endif
			fdef_t *f = ED_FieldAtOfs(progfuncs, OPB->_int + progfuncs->funcs.fieldadjust);
			if (PR_ExecRunWarning(&progfuncs->funcs, st-pr_statements, "assignment to read-only entity %i in %s (%s.%s)\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), d?PR_StringToNative(&progfuncs->funcs, d->s_name):"??", f?f->name:"??"))
				return prinst.pr_xstatement;
			break;
		}

//Whilst the next block would technically be correct, we don't use it as it breaks too many quake mods.
#ifdef NOLEGACY
		errorif (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "assignment to free entity in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
#endif

		i = OPB->_int + progfuncs->funcs.fieldadjust;
		errorif ((unsigned int)i*4 >= ed->fieldsize)	//FIXME:lazy size check
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}

		ptr = (eval_t *)(((int *)edvars(ed)) + i);
		ptr->_int = OPC->_int;
		break;
	case OP_STOREF_I64:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid entity in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
		errorif (!ed || ed->readonly)
		{	//boot it over to the debugger
#if INTSIZE == 16
			ddef16_t *d = ED_GlobalAtOfs16(progfuncs, st->a);
#else
			ddef32_t *d = ED_GlobalAtOfs32(progfuncs, st->a);
#endif
			fdef_t *f = ED_FieldAtOfs(progfuncs, OPB->_int + progfuncs->funcs.fieldadjust);
			if (PR_ExecRunWarning(&progfuncs->funcs, st-pr_statements, "assignment to read-only entity %i in %s (%s.%s)\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), d?PR_StringToNative(&progfuncs->funcs, d->s_name):"??", f?f->name:"??"))
				return prinst.pr_xstatement;
			break;
		}

//Whilst the next block would technically be correct, we don't use it as it breaks too many quake mods.
#ifdef NOLEGACY
		errorif (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "assignment to free entity in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
#endif

		i = OPB->_int + progfuncs->funcs.fieldadjust;
		errorif ((unsigned int)i*4 >= ed->fieldsize)	//FIXME:lazy size check
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}

		ptr = (eval_t *)(((int *)edvars(ed)) + i);
		ptr->i64 = OPC->i64;
		break;
	case OP_STOREF_V:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid entity in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
		errorif (!ed || ed->readonly)
		{	//boot it over to the debugger
#if INTSIZE == 16
			ddef16_t *d = ED_GlobalAtOfs16(progfuncs, st->a);
#else
			ddef32_t *d = ED_GlobalAtOfs32(progfuncs, st->a);
#endif
			fdef_t *f = ED_FieldAtOfs(progfuncs, OPB->_int + progfuncs->funcs.fieldadjust);
			if (PR_ExecRunWarning(&progfuncs->funcs, st-pr_statements, "assignment to read-only entity %i in %s (%s.%s)\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), d?PR_StringToNative(&progfuncs->funcs, d->s_name):"??", f?f->name:"??"))
				return prinst.pr_xstatement;
			break;
		}

//Whilst the next block would technically be correct, we don't use it as it breaks too many quake mods.
#ifdef NOLEGACY
		errorif (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "assignment to free entity in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
#endif

		i = OPB->_int + progfuncs->funcs.fieldadjust;
		errorif ((unsigned int)i*4 >= ed->fieldsize)	//FIXME:lazy size check
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_STOREF_? references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}

		ptr = (eval_t *)(((int *)edvars(ed)) + i);
		ptr->_vector[0] = OPC->_vector[0];
		ptr->_vector[1] = OPC->_vector[1];
		ptr->_vector[2] = OPC->_vector[2];
		break;


	//get a pointer to a field var
	case OP_ADDRESS:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_ADDRESS references invalid entity in %s\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
		errorif (!ed || ed->readonly)
		{

			//boot it over to the debugger
			{
#if INTSIZE == 16
				ddef16_t *d = ED_GlobalAtOfs16(progfuncs, st->a);
#else
				ddef32_t *d = ED_GlobalAtOfs32(progfuncs, st->a);
#endif
				fdef_t *f = ED_FieldAtOfs(progfuncs, OPB->_int + progfuncs->funcs.fieldadjust);
				if (PR_ExecRunWarning(&progfuncs->funcs, st-pr_statements, "assignment to read-only entity %i in %s (%s.%s)\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), d?PR_StringToNative(&progfuncs->funcs, d->s_name):"??", f?f->name:"??"))
					return prinst.pr_xstatement;
				OPC->_int = ~0;
				break;
			}
		}

//Whilst the next block would technically be correct, we don't use it as it breaks too many quake mods.
#ifdef NOLEGACY
		errorif (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "assignment to free entity in %s", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			break;
		}
#endif

		i = OPB->_int + progfuncs->funcs.fieldadjust;
#ifdef PARANOID
		errorif ((unsigned int)i*4 >= ed->fieldsize)	//FIXME:lazy size check
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_ADDRESS references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_int = 0;
			break;
		}
#endif

		OPC->_int = ENGINEPOINTER((((int *)edvars(ed)) + i));
		break;

	//load a field to a value
	case OP_LOAD_P:
	case OP_LOAD_I:
	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references invalid entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_int = 0;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
#ifdef NOLEGACY
		if (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references free entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_int = 0;
		}
		else
#endif
		{
			i = OPB->_int + progfuncs->funcs.fieldadjust;
			errorif ((unsigned int)(i+1)*4 > ed->fieldsize)	//FIXME:lazy size check
			{
				if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
					return prinst.pr_xstatement;
				OPC->_int = 0;
				break;
			}
			ptr = (eval_t *)(((int *)edvars(ed)) + i);
			OPC->_int = ptr->_int;
		}
		break;
	case OP_LOAD_I64:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD_V references invalid entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_vector[0] = 0;
			OPC->_vector[1] = 0;
			OPC->_vector[2] = 0;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
#ifdef NOLEGACY
		if (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references free entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_vector[0] = 0;
			OPC->_vector[1] = 0;
			OPC->_vector[2] = 0;
		}
		else
#endif
		{
			i = OPB->_int + progfuncs->funcs.fieldadjust;
			errorif ((unsigned int)(i+2)*4 > ed->fieldsize)	//FIXME:lazy size check
			{
				if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
					return prinst.pr_xstatement;
				OPC->_int = 0;
				break;
			}
			ptr = (eval_t *)(((int *)edvars(ed)) + i);
			OPC->i64 = ptr->i64;
		}
		break;
	case OP_LOAD_V:
		errorif ((unsigned)OPA->edict >= (unsigned)num_edicts)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD_V references invalid entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_vector[0] = 0;
			OPC->_vector[1] = 0;
			OPC->_vector[2] = 0;
			break;
		}
		ed = PROG_TO_EDICT_PB(progfuncs, OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
#ifdef NOLEGACY
		if (ed->ereftype == ER_FREE)
		{
			if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references free entity %i in %s\n", OPA->edict, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
				return prinst.pr_xstatement;
			OPC->_vector[0] = 0;
			OPC->_vector[1] = 0;
			OPC->_vector[2] = 0;
		}
		else
#endif
		{
			i = OPB->_int + progfuncs->funcs.fieldadjust;
			errorif ((unsigned int)(i+3)*4 > ed->fieldsize)	//FIXME:lazy size check
			{
				if (PR_ExecRunWarning (&progfuncs->funcs, st-pr_statements, "OP_LOAD references invalid field %i in %s\n", OPB->_int, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name)))
					return prinst.pr_xstatement;
				OPC->_int = 0;
				break;
			}
			ptr = (eval_t *)(((int *)edvars(ed)) + i);
			OPC->_vector[0] = ptr->_vector[0];
			OPC->_vector[1] = ptr->_vector[1];
			OPC->_vector[2] = ptr->_vector[2];
		}
		break;

//==================

	case OP_IFNOT_S:
		RUNAWAYCHECK();
		if (!OPA->string || !PR_StringToNative(&progfuncs->funcs, OPA->string))
			st += (sofs)st->b - 1;	// offset the s++
		break;

	case OP_IFNOT_F:
		RUNAWAYCHECK();
		if (!EVAL_FLOATISTRUE(OPA))
			st += (sofs)st->b - 1;	// offset the s++
		break;

	//WARNING: vanilla uses this for floats too, which results in a discrepancy with -0
	case OP_IFNOT_I:
		RUNAWAYCHECK();
		if (!OPA->_int)
			st += (sofs)st->b - 1;	// offset the s++
		break;

	case OP_IF_S:
		RUNAWAYCHECK();
		if (OPA->string && PR_StringToNative(&progfuncs->funcs, OPA->string))
			st += (sofs)st->b - 1;	// offset the s++
		break;

	case OP_IF_F:
		RUNAWAYCHECK();
		if (EVAL_FLOATISTRUE(OPA))
			st += (sofs)st->b - 1;	// offset the s++
		break;

	//WARNING: vanilla uses this for floats too, which results in a discrepancy with -0
	case OP_IF_I:
		RUNAWAYCHECK();
		if (OPA->_int)
			st += (sofs)st->b - 1;	// offset the s++
		break;

	case OP_GOTO:
		RUNAWAYCHECK();
		st += (sofs)st->a - 1;	// offset the s++
		break;

	case OP_CALL8H:
	case OP_CALL7H:
	case OP_CALL6H:
	case OP_CALL5H:
	case OP_CALL4H:
	case OP_CALL3H:
	case OP_CALL2H:
		G_VECTOR(OFS_PARM1)[0] = OPC->_vector[0];
		G_VECTOR(OFS_PARM1)[1] = OPC->_vector[1];
		G_VECTOR(OFS_PARM1)[2] = OPC->_vector[2];
	case OP_CALL1H:
		G_VECTOR(OFS_PARM0)[0] = OPB->_vector[0];
		G_VECTOR(OFS_PARM0)[1] = OPB->_vector[1];
		G_VECTOR(OFS_PARM0)[2] = OPB->_vector[2];

	case OP_CALL8:
	case OP_CALL7:
	case OP_CALL6:
	case OP_CALL5:
	case OP_CALL4:
	case OP_CALL3:
	case OP_CALL2:
	case OP_CALL1:
	case OP_CALL0:
		{
			int newpr;
			unsigned int fnum;
			RUNAWAYCHECK();
			prinst.pr_xstatement = st-pr_statements;

			if (op > OP_CALL8)
				progfuncs->funcs.callargc = op - (OP_CALL1H-1);
			else
				progfuncs->funcs.callargc = op - OP_CALL0;
			fnum = OPA->function;

			glob = NULL;	//try to derestrict it.

			progfuncs->funcs.callprogs=prinst.pr_typecurrent;			//so we can revert to the right caller.
			newpr = (fnum & 0xff000000)>>24;	//this is the progs index of the callee
			fnum &= ~0xff000000;				//the callee's function index.

			//if it's an external call, switch now (before any function pointers are used)
			errorif (!PR_SwitchProgsParms(progfuncs, newpr) || !fnum || fnum > pr_progs->numfunctions)
			{
				char *msg = fnum?"OP_CALL references invalid function in %s\n":"NULL function from qc (inside %s).\n";
				PR_SwitchProgsParms(progfuncs, progfuncs->funcs.callprogs);

				glob = pr_globals;
				if (!progfuncs->funcs.debug_trace)
					QCFAULT(&progfuncs->funcs, msg, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));

				//skip the instruction if they just try stepping over it anyway.
				PR_StackTrace(&progfuncs->funcs, 0);
				externs->Printf(msg, PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));

				pr_globals[OFS_RETURN] = 0;
				pr_globals[OFS_RETURN+1] = 0;
				pr_globals[OFS_RETURN+2] = 0;
				break;
			}

			newf = &pr_cp_functions[fnum & ~0xff000000];

			if (newf->first_statement <= 0)
			{	// negative statements are built in functions
				/*calling a builtin in another progs may affect that other progs' globals instead, is the theory anyway, so args and stuff need to move over*/
				if (prinst.pr_typecurrent != 0)
				{
					//builtins quite hackily refer to only a single global.
					//for builtins to affect the globals of other progs, we need to first switch to the progs that it will affect, so they'll be correct when we switch back
					PR_SwitchProgsParms(progfuncs, 0);
				}
				i = -newf->first_statement;
				if (i < externs->numglobalbuiltins)
				{
#ifndef QCGC
					prinst.numtempstringsstack = prinst.numtempstrings;
#endif
					(*externs->globalbuiltins[i]) (&progfuncs->funcs, (struct globalvars_s *)current_progstate->globals);

					//in case ed_alloc was called
					num_edicts = sv_num_edicts;
				}
				else
					PR_RunError (&progfuncs->funcs, "Bad builtin call number - %i", -newf->first_statement);
				PR_SwitchProgsParms(progfuncs, (progsnum_t)progfuncs->funcs.callprogs);

				//decide weather non debugger wants to start debugging.
				return prinst.pr_xstatement;
			}
			s = PR_EnterFunction (progfuncs, newf, progfuncs->funcs.callprogs);
			st = &pr_statements[s];
		}
		
		//resume at the new statement, which might be in a different progs
		return s;

	case OP_DONE:
	case OP_RETURN:

		RUNAWAYCHECK();

		glob[OFS_RETURN] = glob[st->a];
		glob[OFS_RETURN+1] = glob[st->a+1];
		glob[OFS_RETURN+2] = glob[st->a+2];

		s = PR_LeaveFunction (progfuncs);
		st = &pr_statements[s];		
		if (prinst.pr_depth == prinst.exitdepth)
		{
			prinst.pr_xstatement = s;
			return -1;		// all done
		}
		return s;
//		break;

	case OP_STATE:
		externs->stateop(&progfuncs->funcs, OPA->_float, OPB->function);
		break;

	case OP_ADD_I:		
		OPC->_int = OPA->_int + OPB->_int;
		break;
	case OP_ADD_FI:
		OPC->_float = OPA->_float + (float)OPB->_int;
		break;
	case OP_ADD_IF:
		OPC->_float = (float)OPA->_int + OPB->_float;
		break;
  
	case OP_SUB_I:
		OPC->_int = OPA->_int - OPB->_int;
		break;
	case OP_SUB_FI:
		OPC->_float = OPA->_float - (float)OPB->_int;
		break;
	case OP_SUB_IF:
		OPC->_float = (float)OPA->_int - OPB->_float;
		break;

	case OP_CONV_ITOF:
		OPC->_float = (float)OPA->_int;
		break;
	case OP_CONV_FTOI:
		OPC->_int = (int)OPA->_float;
		break;

	case OP_LOADP_ITOF:
		i = OPA->_int;
		errorif (QCPOINTERREADFAIL(i, sizeof(char)))
		{
			QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%#x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPA->_int);
		}
		ptr = QCPOINTERM(i);
		OPC->_float = (float)ptr->_int;
		break;

	case OP_LOADP_FTOI:
		i = OPA->_int;
		errorif (QCPOINTERREADFAIL(i, sizeof(char)))
		{
			QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%#x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPA->_int);
		}
		ptr = QCPOINTERM(i);
		OPC->_int = (int)ptr->_float;
		break;

	case OP_BITAND_I:
		OPC->_int = (OPA->_int & OPB->_int);
		break;
	
	case OP_BITOR_I:
		OPC->_int = (OPA->_int | OPB->_int);
		break;

	case OP_MUL_I:		
		OPC->_int = OPA->_int * OPB->_int;
		break;
	case OP_DIV_I:
		if (OPB->_int == 0)	//no division by zero allowed...
			OPC->_int = 0;
		else if (OPB->_int == -1 && OPA->_int==(int)0x80000000)
			OPC->_int = 0x7fffffff;
		else
			OPC->_int = OPA->_int / OPB->_int;
		break;
	case OP_DIV_U:
		if (OPB->_uint == 0)	//no division by zero allowed...
			OPC->_uint = 0;
		else
			OPC->_uint = OPA->_uint / OPB->_uint;
		break;
	case OP_EQ_I:
		OPC->_int = (OPA->_int == OPB->_int);
		break;
	case OP_NE_I:
		OPC->_int = (OPA->_int != OPB->_int);
		break;
	

	//array/structure reading/writing.
	case OP_GLOBALADDRESS:
		OPC->_int = ENGINEPOINTER(&OPA->_int + OPB->_int); /*pointer arithmatic*/
		break;
	case OP_ADD_PIW:	//pointer to 32 bit (remember to *3 for vectors)
		OPC->_int = OPA->_int + OPB->_int*sizeof(float);
		break;

	case OP_LOADA_I:
	case OP_LOADA_F:
	case OP_LOADA_FLD:
	case OP_LOADA_ENT:
	case OP_LOADA_S:
	case OP_LOADA_FNC:
		i = st->a + OPB->_int;
		if ((size_t)i >= (size_t)(current_progstate->globals_bytes>>2))
		{
			QCFAULT(&progfuncs->funcs, "bad array read in %s (index %i)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPB->_int);
		}
		else
			OPC->_int = ((eval_t *)&glob[i])->_int;
		break;
	case OP_LOADA_I64:
		i = st->a + OPB->_int;
		if ((size_t)i >= (size_t)(current_progstate->globals_bytes>>2)-1u)
		{
			QCFAULT(&progfuncs->funcs, "bad array read in %s (index %i)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPB->_int);
		}
		else
			OPC->i64 = ((eval_t *)&glob[i])->i64;
		break;
	case OP_LOADA_V:
		i = st->a + OPB->_int;
		if ((size_t)(i) >= (size_t)(current_progstate->globals_bytes>>2)-2u)
		{
			QCFAULT(&progfuncs->funcs, "bad array read in %s (index %i)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPB->_int);
		}
		else
		{
			OPC->_vector[0] = ((eval_t *)&glob[i])->_vector[0];
			OPC->_vector[1] = ((eval_t *)&glob[i])->_vector[1];
			OPC->_vector[2] = ((eval_t *)&glob[i])->_vector[2];
		}
		break;



	case OP_ADD_SF:	//(char*)c = (char*)a + (float)b
		OPC->_int = OPA->_int + (int)OPB->_float;
		break;
	case OP_SUB_S:	//(float)c = (char*)a - (char*)b
		OPC->_int = OPA->_int - OPB->_int;
		break;
	case OP_LOADP_C:	//load character from a string/pointer
		i = (unsigned int)OPA->_int + (int)OPB->_float;
		errorif (QCPOINTERREADFAIL(i, sizeof(char)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_float, sizeof(char))))
			{
				if (i == -1)
				{
					OPC->_float = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%i bytes into %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, ptr);
			}
		}
		else 
			ptr = QCPOINTERM(i);
		OPC->_float = *(unsigned char *)ptr;
		break;
	case OP_LOADP_U8:	//load character from a string/pointer
		i = (unsigned int)OPA->_int + (int)OPB->_int;
		errorif (QCPOINTERREADFAIL(i, sizeof(pbyte)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int, sizeof(pbyte))))
			{
				if (i == -1)
				{
					OPC->_int = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%i bytes into %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, ptr);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_int = *(pbyte *)ptr;
		break;
	case OP_LOADP_I8:	//load character from a string/pointer
		i = (unsigned int)OPA->_int + (int)OPB->_int;
		errorif (QCPOINTERREADFAIL(i, sizeof(pbyte)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int, sizeof(pbyte))))
			{
				if (i == -1)
				{
					OPC->_int = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%i bytes into %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, ptr);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_int = *(char *)ptr;
		break;
	case OP_LOADP_U16:	//load character from a string/pointer
		i = (unsigned int)OPA->_int + (int)OPB->_int*2;
		errorif (QCPOINTERREADFAIL(i, sizeof(short)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int*2, sizeof(short))))
			{
				if (i == -1)
				{
					OPC->_int = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%i bytes into %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, ptr);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_int = *(unsigned short *)ptr;
		break;
	case OP_LOADP_I16:	//load character from a string/pointer
		i = (unsigned int)OPA->_int + (int)OPB->_int*2;
		errorif (QCPOINTERREADFAIL(i, sizeof(short)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int*2, sizeof(short))))
			{
				if (i == -1)
				{
					OPC->_int = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (%i bytes into %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, ptr);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_int = *(short *)ptr;
		break;
	case OP_LOADP_I:
	case OP_LOADP_F:
	case OP_LOADP_FLD:
	case OP_LOADP_ENT:
	case OP_LOADP_S:
	case OP_LOADP_FNC:
		i = OPA->_int + OPB->_int*4;
		errorif (QCPOINTERREADFAIL(i, sizeof(int)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int*4, sizeof(int))))
			{
				if (i == -1)
				{
					OPC->_int = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (from %#x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_int = ptr->_int;
		break;

	case OP_LOADP_I64:
		i = OPA->_int + OPB->_int*4;
		errorif (QCPOINTERREADFAIL(i, sizeof(pint64_t)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int*4, sizeof(pint64_t))))
			{
				if (i == -1)
				{
					OPC->i64 = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (from %#x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->i64 = ptr->i64;
		break;

	case OP_LOADP_V:
		i = OPA->_int + OPB->_int*4;	//NOTE: inconsistant, but a bit more practical for the qcc when structs etc are involved
		errorif (QCPOINTERREADFAIL(i, sizeof(pvec3_t)))
		{
			if (!(ptr=PR_GetReadTempStringPtr(progfuncs, OPA->_int, OPB->_int*4, sizeof(pvec3_t))))
			{
				if (i == -1)
				{
					OPC->_vector[0] = 0;
					OPC->_vector[1] = 0;
					OPC->_vector[2] = 0;
					break;
				}
				QCFAULT(&progfuncs->funcs, "bad pointer read in %s (from %#x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i);
			}
		}
		else
			ptr = QCPOINTERM(i);
		OPC->_vector[0] = ptr->_vector[0];
		OPC->_vector[1] = ptr->_vector[1];
		OPC->_vector[2] = ptr->_vector[2];
		break;

	case OP_BITXOR_I:
		OPC->_int = OPA->_int ^ OPB->_int;
		break;
	case OP_RSHIFT_I:
		OPC->_int = A_RSHIFT_I(OPA->_int, OPB->_int);
		break;
	case OP_RSHIFT_U:
		OPC->_uint = OPA->_uint >> OPB->_uint;
		break;
	case OP_LSHIFT_I:
		OPC->_int = OPA->_int << OPB->_int;
		break;

	//hexen2 arrays contain a prefix global set to (arraysize-1) inserted before the actual array data
	//for vectors, this prefix is the number of vectors rather than the number of globals. this can cause issues with using OP_FETCH_GBL_V within structs.
	case OP_FETCH_GBL_F:
	case OP_FETCH_GBL_S:
	case OP_FETCH_GBL_E:
	case OP_FETCH_GBL_FNC:
		i = OPB->_float;
		errorif((unsigned)i > (unsigned)((eval_t *)&glob[st->a-1])->_int)
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError(&progfuncs->funcs, "array index out of bounds: %s[%d] (max %d)", PR_GlobalStringNoContents(progfuncs, st->a), i, ((eval_t *)&glob[st->a-1])->_int);
		}
		OPC->_int = ((eval_t *)&glob[st->a + i])->_int;
		break;
	case OP_FETCH_GBL_V:
		i = OPB->_float;
		errorif((unsigned)i > (unsigned)((eval_t *)&glob[st->a-1])->_int)
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError(&progfuncs->funcs, "array index out of bounds: %s[%d]", PR_GlobalStringNoContents(progfuncs, st->a), i);
		}
		ptr = (eval_t *)&glob[st->a + i*3];
		OPC->_vector[0] = ptr->_vector[0];
		OPC->_vector[1] = ptr->_vector[1];
		OPC->_vector[2] = ptr->_vector[2];
		break;

	case OP_CSTATE:
		externs->cstateop(&progfuncs->funcs, OPA->_float, OPB->_float, prinst.pr_xfunction - pr_cp_functions);
		break;

	case OP_CWSTATE:
		externs->cwstateop(&progfuncs->funcs, OPA->_float, OPB->_float, prinst.pr_xfunction - pr_cp_functions);
		break;

	case OP_THINKTIME:
		externs->thinktimeop(&progfuncs->funcs, (struct edict_s *)PROG_TO_EDICT_UB(progfuncs, OPA->edict), OPB->_float);
		break;

	case OP_MULSTORE_F:
		/*OPC->_float = */OPB->_float *= OPA->_float;
		break;
	case OP_MULSTORE_VF:
		tmpf = OPA->_float;	//don't break on vec*=vec_x;
		/*OPC->_vector[0] = */OPB->_vector[0] *= tmpf;
		/*OPC->_vector[1] = */OPB->_vector[1] *= tmpf;
		/*OPC->_vector[2] = */OPB->_vector[2] *= tmpf;
		break;
	case OP_MULSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_float = ptr->_float *= OPA->_float;
		break;
	case OP_MULSTOREP_VF:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(pvec3_t)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		tmpf = OPA->_float;	//don't break on vec*=vec_x;
		OPC->_vector[0] = ptr->_vector[0] *= tmpf;
		OPC->_vector[1] = ptr->_vector[1] *= tmpf;
		OPC->_vector[2] = ptr->_vector[2] *= tmpf;
		break;
	case OP_DIVSTORE_F:
		/*OPC->_float = */OPB->_float /= OPA->_float;
		break;
	case OP_DIVSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_float = ptr->_float /= OPA->_float;
		break;
	case OP_ADDSTORE_F:
		/*OPC->_float = */OPB->_float += OPA->_float;
		break;
	case OP_ADDSTORE_V:
		/*OPC->_vector[0] =*/ OPB->_vector[0] += OPA->_vector[0];
		/*OPC->_vector[1] =*/ OPB->_vector[1] += OPA->_vector[1];
		/*OPC->_vector[2] =*/ OPB->_vector[2] += OPA->_vector[2];
		break;
	case OP_ADDSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_float = ptr->_float += OPA->_float;
		break;
	case OP_ADDSTOREP_V:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(pvec3_t)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_vector[0] = ptr->_vector[0] += OPA->_vector[0];
		OPC->_vector[1] = ptr->_vector[1] += OPA->_vector[1];
		OPC->_vector[2] = ptr->_vector[2] += OPA->_vector[2];
		break;
	case OP_SUBSTORE_F:
		/*OPC->_float = */OPB->_float -= OPA->_float;
		break;
	case OP_SUBSTORE_V:
		/*OPC->_vector[0] = */OPB->_vector[0] -= OPA->_vector[0];
		/*OPC->_vector[1] = */OPB->_vector[1] -= OPA->_vector[1];
		/*OPC->_vector[2] = */OPB->_vector[2] -= OPA->_vector[2];
		break;
	case OP_SUBSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_float = ptr->_float -= OPA->_float;
		break;
	case OP_SUBSTOREP_V:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(pvec3_t)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		OPC->_vector[0] = ptr->_vector[0] -= OPA->_vector[0];
		OPC->_vector[1] = ptr->_vector[1] -= OPA->_vector[1];
		OPC->_vector[2] = ptr->_vector[2] -= OPA->_vector[2];
		break;
	case OP_BITSETSTORE_F:
		OPB->_float = (int)OPB->_float | (int)OPA->_float;
		break;
	case OP_BITSETSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		ptr->_float = (int)ptr->_float | (int)OPA->_float;
		break;
	case OP_BITCLRSTORE_F:
		OPB->_float = (int)OPB->_float & ~(int)OPA->_float;
		break;
	case OP_BITCLRSTOREP_F:
		i = OPB->_int;
		errorif (QCPOINTERWRITEFAIL(i, sizeof(float)))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad pointer write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), i, (unsigned)prinst.addressableused);
		}
		ptr = QCPOINTERM(i);
		ptr->_float = (int)ptr->_float & ~(int)OPA->_float;
		break;

	//for scaler randoms, prevent the random value from ever reaching 1
	//this avoids issues when array[random()*array.length]
	case OP_RAND0:
		OPC->_float = (rand ()&0x7fff) / ((float)0x8000);
		break;
	case OP_RAND1:
		OPC->_float = (rand ()&0x7fff) / ((float)0x8000)*OPA->_float;
		break;
	case OP_RAND2:	//backwards range shouldn't matter (except that it is b that is never reached, rather than the higher of the two)
		OPC->_float = OPA->_float + (rand ()&0x7fff) / ((float)0x8000)*(OPB->_float-OPA->_float);
		break;
	//random vectors DO result in 0 to 1 inclusive, to try to ensure a more balanced range
	case OP_RANDV0:
		OPC->_vector[0] = (rand ()&0x7fff) / ((float)0x7fff);
		OPC->_vector[1] = (rand ()&0x7fff) / ((float)0x7fff);
		OPC->_vector[2] = (rand ()&0x7fff) / ((float)0x7fff);
		break;
	case OP_RANDV1:
		OPC->_vector[0] = (rand ()&0x7fff) / ((float)0x7fff)*OPA->_vector[0];
		OPC->_vector[1] = (rand ()&0x7fff) / ((float)0x7fff)*OPA->_vector[1];
		OPC->_vector[2] = (rand ()&0x7fff) / ((float)0x7fff)*OPA->_vector[2];
		break;
	case OP_RANDV2:	//backwards range shouldn't matter
		OPC->_vector[0] = OPA->_vector[0] + (rand ()&0x7fff) / ((float)0x7fff)*(OPB->_vector[0]-OPA->_vector[0]);
		OPC->_vector[1] = OPA->_vector[1] + (rand ()&0x7fff) / ((float)0x7fff)*(OPB->_vector[1]-OPA->_vector[1]);
		OPC->_vector[2] = OPA->_vector[2] + (rand ()&0x7fff) / ((float)0x7fff)*(OPB->_vector[2]-OPA->_vector[2]);
		break;

	case OP_SWITCH_F:
	case OP_SWITCH_V:
	case OP_SWITCH_S:
	case OP_SWITCH_E:
	case OP_SWITCH_FNC:
		//the case opcodes depend upon the preceding switch.
		//otherwise the switch itself is much like a goto
		//don't embed the case/caserange checks directly into the switch so that custom caseranges can be potentially be implemented with hybrid emulation.
		switchcomparison = op - OP_SWITCH_F;
		switchref = OPA;
		RUNAWAYCHECK();
		st += (sofs)st->b - 1;	// offset the s++
		break;
	case OP_SWITCH_I:
		//the case opcodes depend upon the preceding switch.
		//otherwise the switch itself is much like a goto
		//don't embed the case/caserange checks directly into the switch so that custom caseranges can be potentially be implemented with hybrid emulation.
		switchcomparison = OP_SWITCH_E - OP_SWITCH_F;
		switchref = OPA;
		RUNAWAYCHECK();
		st += (sofs)st->b - 1;	// offset the s++
		break;
	case OP_CASE:
		//if the comparison is true, jump (back up) to the relevent code block
		if (casecmp[switchcomparison](progfuncs, switchref, OPA))
		{
			RUNAWAYCHECK();
			st += (sofs)st->b-1; // -1 to offset the s++
		}
		break;
	case OP_CASERANGE:
		//if the comparison is true, jump (back up) to the relevent code block
		if (casecmprange[switchcomparison](progfuncs, switchref, OPA, OPB))
		{
			RUNAWAYCHECK();
			st += (sofs)st->c-1; // -1 to offset the s++
		}
		break;








	case OP_BITAND_IF:
		OPC->_int = (OPA->_int & (int)OPB->_float);
		break;
	case OP_BITOR_IF:
		OPC->_int = (OPA->_int | (int)OPB->_float);
		break;
	case OP_BITAND_FI:
		OPC->_int = ((int)OPA->_float & OPB->_int);
		break;
	case OP_BITOR_FI:
		OPC->_int = ((int)OPA->_float | OPB->_int);
		break;

	case OP_MUL_IF:
		OPC->_float = (OPA->_int * OPB->_float);
		break;
	case OP_MUL_FI:
		OPC->_float = (OPA->_float * OPB->_int);
		break;

	case OP_MUL_VI:
		tmpi = OPB->_int;
		OPC->_vector[0] = OPA->_vector[0] * tmpi;
		OPC->_vector[1] = OPA->_vector[1] * tmpi;
		OPC->_vector[2] = OPA->_vector[2] * tmpi;
		break;
	case OP_MUL_IV:
		tmpi = OPA->_int;
		OPC->_vector[0] = tmpi * OPB->_vector[0];
		OPC->_vector[1] = tmpi * OPB->_vector[1];
		OPC->_vector[2] = tmpi * OPB->_vector[2];
		break;

	case OP_DIV_IF:
		OPC->_float = (OPA->_int / OPB->_float);
		break;
	case OP_DIV_FI:
		OPC->_float = (OPA->_float / OPB->_int);
		break;

	/*case OP_MOD_I:
		OPC->_int = (OPA->_int % OPB->_int);
		break;
	case OP_MOD_U:
		OPC->_uint = (OPA->_uint % OPB->_uint);
		break;
	case OP_MOD_F:
		OPC->_float = OPA->_float - OPB->_float*(int)(OPA->_float/OPB->_float);
		break;
	case OP_MOD_V:
		OPC->_vector[0] = OPA->_vector[0] - OPB->_vector[0]*(int)(OPA->_vector[0]/OPB->_vector[0]);
		OPC->_vector[1] = OPA->_vector[1] - OPB->_vector[1]*(int)(OPA->_vector[1]/OPB->_vector[1]);
		OPC->_vector[2] = OPA->_vector[2] - OPB->_vector[2]*(int)(OPA->_vector[2]/OPB->_vector[2]);
		break;*/


	case OP_AND_I:
		OPC->_int = (OPA->_int && OPB->_int);
		break;
	case OP_OR_I:
		OPC->_int = (OPA->_int || OPB->_int);
		break;

	case OP_AND_IF:
		OPC->_int = (OPA->_int && OPB->_float);
		break;
	case OP_OR_IF:
		OPC->_int = (OPA->_int || OPB->_float);
		break;

	case OP_AND_FI:
		OPC->_int = (OPA->_float && OPB->_int);
		break;
	case OP_OR_FI:
		OPC->_int = (OPA->_float || OPB->_int);
		break;

	case OP_NE_IF:
		OPC->_int = (OPA->_int != OPB->_float);
		break;
	case OP_NE_FI:
		OPC->_int = (OPA->_float != OPB->_int);
		break;

	case OP_GADDRESS: //return glob[aint+bfloat]
		//this instruction is not implemented due to the weirdness of it.
		//its theoretically a more powerful load... but untyped?
		//or is it meant to be an LEA instruction (that could simply be switched with OP_GLOAD_I)
		prinst.pr_xstatement = st-pr_statements;
		PR_RunError (&progfuncs->funcs, "OP_GADDRESS not implemented (found in %s)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
		break;
	case OP_GLOAD_I:
	case OP_GLOAD_F:
	case OP_GLOAD_FLD:
	case OP_GLOAD_ENT:
	case OP_GLOAD_S:
	case OP_GLOAD_FNC:
		errorif (OPA->_int < 0 || OPA->_int >= (current_progstate->globals_bytes>>2))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad indexed global read in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPA->_int, current_progstate->globals_bytes>>2);
		}
		ptr = ((eval_t *)&glob[OPA->_int]);
		OPC->_int = ptr->_int;
		break;
	case OP_GLOAD_V:
		errorif (OPA->_int < 0 || OPA->_int >= (current_progstate->globals_bytes>>2)-2u)
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad indexed global read in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPA->_int, current_progstate->globals_bytes>>2);
		}
		ptr = ((eval_t *)&glob[OPA->_int]);
		OPC->_vector[0] = ptr->_vector[0];
		OPC->_vector[1] = ptr->_vector[1];
		OPC->_vector[2] = ptr->_vector[2];
		break;
	case OP_GSTOREP_I:
	case OP_GSTOREP_F:
	case OP_GSTOREP_ENT:
	case OP_GSTOREP_FLD:
	case OP_GSTOREP_S:
	case OP_GSTOREP_FNC:
		errorif (OPB->_int < 0 || OPB->_int >= (current_progstate->globals_bytes>>2))
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad indexed global write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPB->_int, current_progstate->globals_bytes>>2);
		}
		ptr = ((eval_t *)&glob[OPB->_int]);
		ptr->_int = OPA->_int;
		break;
	case OP_GSTOREP_V:
		errorif (OPB->_int < 0 || OPB->_int >= (current_progstate->globals_bytes>>2)-2u)
		{
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError (&progfuncs->funcs, "bad indexed global write in %s (%x >= %x)", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name), OPB->_int, current_progstate->globals_bytes>>2);
		}
		ptr = ((eval_t *)&glob[OPB->_int]);
		ptr->_vector[0] = OPA->_vector[0];
		ptr->_vector[1] = OPA->_vector[1];
		ptr->_vector[2] = OPA->_vector[2];
		break;

	case OP_BOUNDCHECK:
		errorif ((unsigned int)OPA->_int < (unsigned int)st->c || (unsigned int)OPA->_int >= (unsigned int)st->b)
		{
			externs->Printf("Progs boundcheck failed. Value is %i. Must be %u<=value<%u\n", OPA->_int, st->c, st->b);
			QCFAULT(&progfuncs->funcs, "Progs boundcheck failed. Value is %i. Must be %u<=value<%u\n", OPA->_int, st->c, st->b);
/*			s=ShowStepf(progfuncs, st - pr_statements, "Progs boundcheck failed. Value is %i. Must be between %u and %u\n", OPA->_int, st->c, st->b);
			if (st == pr_statements + s)
				PR_RunError(&progfuncs->funcs, "unable to resume boundcheck");
			st = pr_statements + s;
			return s;
*/		}
		break;
	case OP_PUSH:	//note: OPA is words, not bytes.
		OPC->_int = ENGINEPOINTER(&prinst.localstack[prinst.localstack_used+prinst.spushed]);
		prinst.spushed += OPA->_uint;
		if (prinst.spushed + prinst.localstack_used >= LOCALSTACK_SIZE)
		{
			i = prinst.spushed;
			prinst.spushed = 0;
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError(&progfuncs->funcs, "Progs pushed too much (%i bytes, %i parents, max %i)", i, prinst.localstack_used, LOCALSTACK_SIZE);
		}
		break;
/*	case OP_POP:
		pr_spushed -= OPA->_uint;
		if (pr_spushed < 0)
		{
			pr_spushed = 0;
			prinst.pr_xstatement = st-pr_statements;
			PR_RunError(progfuncs, "Progs poped more than it pushed");
		}
		break;
*/


	//[u]int64+double opcodes
	case OP_ADD_I64:		OPC->i64     = OPA->i64  + OPB->i64;   break;
	case OP_SUB_I64:		OPC->i64     = OPA->i64  -  OPB->i64;  break;
	case OP_MUL_I64:		OPC->i64     = OPA->i64  *  OPB->i64;  break;
	case OP_DIV_I64:		OPC->i64     = OPA->i64  /  OPB->i64;  break;
	case OP_BITAND_I64:		OPC->i64     = OPA->i64  &  OPB->i64;  break;
	case OP_BITOR_I64:		OPC->i64     = OPA->i64  |  OPB->i64;  break;
	case OP_BITXOR_I64:		OPC->i64     = OPA->i64  ^  OPB->i64;  break;
	case OP_LSHIFT_I64I:	OPC->i64     = OPA->i64  << OPB->_int; break;
	case OP_RSHIFT_I64I:	OPC->i64	 = A_RSHIFT_I(OPA->i64, OPB->_int);	break;
	case OP_LT_I64:			OPC->_int    = OPA->i64  <  OPB->i64;  break;
	case OP_LE_I64:			OPC->_int    = OPA->i64  <= OPB->i64;  break;
	case OP_EQ_I64:			OPC->_int    = OPA->i64  == OPB->i64;  break;
	case OP_NE_I64:			OPC->_int    = OPA->i64  != OPB->i64;  break;
	case OP_LT_U64:			OPC->_int    = OPA->u64 <  OPB->u64;   break;
	case OP_LE_U64:			OPC->_int    = OPA->u64 <= OPB->u64;   break;
	case OP_DIV_U64:		OPC->u64     = OPA->u64 /  OPB->u64;   break;
	case OP_RSHIFT_U64I:	OPC->u64     = OPA->u64 >> OPB->_int;  break;
	case OP_STORE_I64:		OPB->i64     = OPA->i64;	 break;
	case OP_CONV_UI64:		OPC->i64     = OPA->_uint;   break;
	case OP_CONV_II64:		OPC->i64     = OPA->_int;    break;
	case OP_CONV_I64I:		OPC->_int    = OPA->i64;     break;
	case OP_CONV_FD:		OPC->_double = OPA->_float;  break;
	case OP_CONV_DF:		OPC->_float  = OPA->_double; break;
	case OP_CONV_I64F:		OPC->_float  = OPA->i64;     break;
	case OP_CONV_FI64:		OPC->i64     = OPA->_float;  break;
	case OP_CONV_I64D:		OPC->_double = OPA->i64;     break;
	case OP_CONV_DI64:		OPC->i64     = OPA->_double; break;
	case OP_CONV_U64D:		OPC->_double = OPA->u64;     break;
	case OP_CONV_DU64:		OPC->u64     = OPA->_double; break;
	case OP_CONV_U64F:		OPC->_float	 = OPA->u64;     break;
	case OP_CONV_FU64:		OPC->u64     = OPA->_float;  break;
	case OP_ADD_D:			OPC->_double = OPA->_double +  OPB->_double; break;
	case OP_SUB_D:			OPC->_double = OPA->_double -  OPB->_double; break;
	case OP_MUL_D:			OPC->_double = OPA->_double *  OPB->_double; break;
	case OP_DIV_D:			OPC->_double = OPA->_double /  OPB->_double; break;
	case OP_LT_D:			OPC->_int    = OPA->_double <  OPB->_double; break;
	case OP_LE_D:			OPC->_int    = OPA->_double <= OPB->_double; break;
	case OP_EQ_D:			OPC->_int    = OPA->_double == OPB->_double; break;
	case OP_NE_D:			OPC->_int    = OPA->_double != OPB->_double; break;

	case OP_BITEXTEND_I:	OPC->_int    = A_RSHIFT_I((  signed int)(OPA->_int  << (32-(OPB->_uint&0xff)-(OPB->_uint>>8))), (signed)(32-(OPB->_uint&0xff))); break;	//shift it up and down. should sign extend.
	case OP_BITEXTEND_U:	OPC->_uint   = (unsigned int)(OPA->_uint << (32-(OPB->_uint&0xff)-(OPB->_uint>>8))) >> (32-(OPB->_uint&0xff)); break;	//shift it up and down. should clear the bits.
	case OP_BITCOPY_I:		i=((1<<(OPB->_uint&0xff))-1);OPC->_uint=(OPC->_uint&~(i<<(OPB->_uint>>8)))|(((OPA->_uint&i)<<(OPB->_uint>>8)));break;			//replaces the specified bits (uses the same format bitextend uses to select its input to extend)
	case OP_CONV_UF:		OPC->_float  = OPA->_uint;   break;
	case OP_CONV_FU:		OPC->_uint   = OPA->_float;  break;

	case OP_UNUSED:
	case OP_POP:

#ifdef __GNUC__
	case OP_NUMREALOPS ... OP_NUMOPS:
#endif

	safedefault:
		if (op & OP_BIT_BREAKPOINT)	//break point!
		{
			op &= ~OP_BIT_BREAKPOINT;
			s = st-pr_statements;
			if (prinst.pr_xstatement != s)
			{
				prinst.pr_xstatement = s;
				externs->Printf("Break point hit in %s.\n", PR_StringToNative(&progfuncs->funcs, prinst.pr_xfunction->s_name));
				s = ShowStep(progfuncs, s, NULL, false);
				st = &pr_statements[s];	//let the user move execution
				prinst.pr_xstatement = s = st-pr_statements;
				op = st->op & ~OP_BIT_BREAKPOINT;
			}
			goto reeval;	//reexecute
		}
		prinst.pr_xstatement = st-pr_statements;
		PR_RunError (&progfuncs->funcs, "Bad opcode %i", st->op);
	}
}


#undef reeval
#undef st
#undef pr_statements
#undef fakeop
#undef dstatement_t
#undef sofs
#undef OPCODE

#undef ENGINEPOINTER
#undef QCPOINTER
#undef QCPOINTERM

