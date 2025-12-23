extern cvar_t prvm_garbagecollection_enable;
prvm_uint_t addr, ofs;
prvm_eval_t *src;
// NEED to reset startst after calling this! startst may or may not be clobbered!
#define ADVANCE_PROFILE_BEFORE_JUMP() \
	prog->xfunction->profile += (st - startst); \
	if (prvm_statementprofiling.integer || (prvm_coverage.integer & 4)) { \
		/* All statements from startst+1 to st have been hit. */ \
		while (++startst <= st) { \
			if (prog->statement_profile[startst - cached_statements]++ == 0 && (prvm_coverage.integer & 4)) \
				PRVM_StatementCoverageEvent(prog, prog->xfunction, startst - cached_statements); \
		} \
		/* Observe: startst now is clobbered (now at st+1)! */ \
	}

#ifdef PRVMTIMEPROFILING
#define PRE_ERROR() \
	ADVANCE_PROFILE_BEFORE_JUMP(); \
	prog->xstatement = st - cached_statements; \
	tm = Sys_DirtyTime(); \
	prog->xfunction->tprofile += (tm - starttm >= 0 && tm - starttm < 1800) ? (tm - starttm) : 0; \
	startst = st; \
	starttm = tm
#else
#define PRE_ERROR() \
	ADVANCE_PROFILE_BEFORE_JUMP(); \
	prog->xstatement = st - cached_statements; \
	startst = st
#endif

// This code isn't #ifdef/#define protectable, don't try.

#if HAVE_COMPUTED_GOTOS && !(PRVMSLOWINTERPRETER || PRVMTIMEPROFILING)
  // NOTE: Due to otherwise duplicate labels, only ONE interpreter path may
  // ever hit this!
# define USE_COMPUTED_GOTOS 1
#endif

#if USE_COMPUTED_GOTOS
  // Must exactly match opcode_e enum in pr_comp.h
    static const void *dispatchtable[] = {
	&&handle_OP_DONE,
	&&handle_OP_MUL_F,
	&&handle_OP_MUL_V,
	&&handle_OP_MUL_FV,
	&&handle_OP_MUL_VF,
	&&handle_OP_DIV_F,
	&&handle_OP_ADD_F,
	&&handle_OP_ADD_V,
	&&handle_OP_SUB_F,
	&&handle_OP_SUB_V,

	&&handle_OP_EQ_F,
	&&handle_OP_EQ_V,
	&&handle_OP_EQ_S,
	&&handle_OP_EQ_E,
	&&handle_OP_EQ_FNC,

	&&handle_OP_NE_F,
	&&handle_OP_NE_V,
	&&handle_OP_NE_S,
	&&handle_OP_NE_E,
	&&handle_OP_NE_FNC,

	&&handle_OP_LE_F,
	&&handle_OP_GE_F,
	&&handle_OP_LT_F,
	&&handle_OP_GT_F,

	&&handle_OP_LOAD_F,
	&&handle_OP_LOAD_V,
	&&handle_OP_LOAD_S,
	&&handle_OP_LOAD_ENT,
	&&handle_OP_LOAD_FLD,
	&&handle_OP_LOAD_FNC,

	&&handle_OP_ADDRESS,

	&&handle_OP_STORE_F,
	&&handle_OP_STORE_V,
	&&handle_OP_STORE_S,
	&&handle_OP_STORE_ENT,
	&&handle_OP_STORE_FLD,
	&&handle_OP_STORE_FNC,

	&&handle_OP_STOREP_F,
	&&handle_OP_STOREP_V,
	&&handle_OP_STOREP_S,
	&&handle_OP_STOREP_ENT,
	&&handle_OP_STOREP_FLD,
	&&handle_OP_STOREP_FNC,

	&&handle_OP_RETURN,
	&&handle_OP_NOT_F,
	&&handle_OP_NOT_V,
	&&handle_OP_NOT_S,
	&&handle_OP_NOT_ENT,
	&&handle_OP_NOT_FNC,
	&&handle_OP_IF,
	&&handle_OP_IFNOT,
	&&handle_OP_CALL0,
	&&handle_OP_CALL1,
	&&handle_OP_CALL2,
	&&handle_OP_CALL3,
	&&handle_OP_CALL4,
	&&handle_OP_CALL5,
	&&handle_OP_CALL6,
	&&handle_OP_CALL7,
	&&handle_OP_CALL8,
	&&handle_OP_STATE,
	&&handle_OP_GOTO,
	&&handle_OP_AND_F,
	&&handle_OP_OR_F,

	&&handle_OP_BITAND_F,
	&&handle_OP_BITOR_F,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	&&handle_OP_STORE_I,

	NULL,
	NULL,

	&&handle_OP_ADD_I,
	&&handle_OP_ADD_FI,
	&&handle_OP_ADD_IF,

	&&handle_OP_SUB_I,
	&&handle_OP_SUB_FI,
	&&handle_OP_SUB_IF,
	&&handle_OP_CONV_ITOF,
	&&handle_OP_CONV_FTOI,

	NULL,
	NULL,

	&&handle_OP_LOAD_I,
	&&handle_OP_STOREP_I,

	NULL,
	NULL,

	&&handle_OP_BITAND_I,
	&&handle_OP_BITOR_I,

	&&handle_OP_MUL_I,
	&&handle_OP_DIV_I,
	&&handle_OP_EQ_I,
	&&handle_OP_NE_I,

	NULL,
	NULL,

	&&handle_OP_NOT_I,

	&&handle_OP_DIV_VF,

	NULL,
	&&handle_OP_RSHIFT_I,
	&&handle_OP_LSHIFT_I,

	&&handle_OP_GLOBALADDRESS,
	&&handle_OP_ADD_PIW,
	&&handle_OP_LOADA_F,
	&&handle_OP_LOADA_V,
	&&handle_OP_LOADA_S,
	&&handle_OP_LOADA_ENT,
	&&handle_OP_LOADA_FLD,
	&&handle_OP_LOADA_FNC,
	&&handle_OP_LOADA_I,

	&&handle_OP_STORE_P,
	&&handle_OP_LOAD_P,

	&&handle_OP_LOADP_F,
	&&handle_OP_LOADP_V,
	&&handle_OP_LOADP_S,
	&&handle_OP_LOADP_ENT,
	&&handle_OP_LOADP_FLD,
	&&handle_OP_LOADP_FNC,
	&&handle_OP_LOADP_I,

	&&handle_OP_LE_I,
	&&handle_OP_GE_I,
	&&handle_OP_LT_I,
	&&handle_OP_GT_I,

	&&handle_OP_LE_IF,
	&&handle_OP_GE_IF,
	&&handle_OP_LT_IF,
	&&handle_OP_GT_IF,

	&&handle_OP_LE_FI,
	&&handle_OP_GE_FI,
	&&handle_OP_LT_FI,
	&&handle_OP_GT_FI,

	&&handle_OP_EQ_IF,
	&&handle_OP_EQ_FI,

	NULL,
	NULL,
	NULL,
	NULL,

	&&handle_OP_MUL_IF,
	&&handle_OP_MUL_FI,
	&&handle_OP_MUL_VI,

	NULL,

	&&handle_OP_DIV_IF,
	&&handle_OP_DIV_FI,
	&&handle_OP_BITAND_IF,
	&&handle_OP_BITOR_IF,
	&&handle_OP_BITAND_FI,
	&&handle_OP_BITOR_FI,
	&&handle_OP_AND_I,
	&&handle_OP_OR_I,
	&&handle_OP_AND_IF,
	&&handle_OP_OR_IF,
	&&handle_OP_AND_FI,
	&&handle_OP_OR_FI,
	&&handle_OP_NE_IF,
	&&handle_OP_NE_FI,

	&&handle_OP_GSTOREP_I,
	&&handle_OP_GSTOREP_F,
	&&handle_OP_GSTOREP_ENT,
	&&handle_OP_GSTOREP_FLD,
	&&handle_OP_GSTOREP_S,
	&&handle_OP_GSTOREP_FNC,
	&&handle_OP_GSTOREP_V,
	&&handle_OP_GLOAD_I,
	&&handle_OP_GLOAD_F,
	&&handle_OP_GLOAD_FLD,
	&&handle_OP_GLOAD_ENT,
	&&handle_OP_GLOAD_S,
	&&handle_OP_GLOAD_FNC,
	&&handle_OP_BOUNDCHECK,

	NULL,
	NULL,
	NULL,
	NULL,

	&&handle_OP_GLOAD_V,

	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,


	NULL,
	NULL,


	&&handle_OP_LE_U,
	&&handle_OP_LT_U,
	&&handle_OP_DIV_U,
	&&handle_OP_RSHIFT_U,
	    };
#define DISPATCH_OPCODE() \
    goto *dispatchtable[(++st)->op]
#define HANDLE_OPCODE(opcode) handle_##opcode

    DISPATCH_OPCODE(); // jump to first opcode
#else // USE_COMPUTED_GOTOS
#define DISPATCH_OPCODE() break
#define HANDLE_OPCODE(opcode) case opcode

#if PRVMSLOWINTERPRETER
		{
			if (prog->watch_global_type != ev_void)
			{
				prvm_eval_t *g = PRVM_GLOBALFIELDVALUE(prog->watch_global);
				prog->xstatement = st + 1 - cached_statements;
				PRVM_Watchpoint(prog, 1, "Global watchpoint hit by engine", prog->watch_global_type, &prog->watch_global_value, g);
			}
			if (prog->watch_field_type != ev_void && prog->watch_edict < prog->max_edicts)
			{
				prvm_eval_t *g = PRVM_EDICTFIELDVALUE(prog->edicts + prog->watch_edict, prog->watch_field);
				prog->xstatement = st + 1 - cached_statements;
				PRVM_Watchpoint(prog, 1, "Entityfield watchpoint hit by engine", prog->watch_field_type, &prog->watch_edictfield_value, g);
			}
		}
#endif

		while (1)
		{
			st++;
#endif // USE_COMPUTED_GOTOS

#if !USE_COMPUTED_GOTOS

#if PRVMSLOWINTERPRETER
			if (prog->trace)
				PRVM_PrintStatement(prog, st);
			if (prog->break_statement >= 0)
				if ((st - cached_statements) == prog->break_statement)
				{
					prog->xstatement = st - cached_statements;
					PRVM_Breakpoint(prog, prog->break_stack_index, "Breakpoint hit");
				}
#endif
			switch (st->op)
			{
#endif
			HANDLE_OPCODE(OP_ADD_F):
				OPC->_float = OPA->_float + OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_ADD_V):
				OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
				OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
				OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_SUB_F):
				OPC->_float = OPA->_float - OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_SUB_V):
				OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
				OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
				OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_F):
				OPC->_float = OPA->_float * OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_V):
				OPC->_float = OPA->vector[0]*OPB->vector[0] + OPA->vector[1]*OPB->vector[1] + OPA->vector[2]*OPB->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_FV):
				tempfloat = OPA->_float;
				OPC->vector[0] = tempfloat * OPB->vector[0];
				OPC->vector[1] = tempfloat * OPB->vector[1];
				OPC->vector[2] = tempfloat * OPB->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_VF):
				tempfloat = OPB->_float;
				OPC->vector[0] = tempfloat * OPA->vector[0];
				OPC->vector[1] = tempfloat * OPA->vector[1];
				OPC->vector[2] = tempfloat * OPA->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_F):
				if( OPB->_float == 0.0f )
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of %f by zero\n", OPA->_float);
					if (prvm_gameplayfix_div0is0.integer)
					{
						OPC->_float = 0;
						DISPATCH_OPCODE();
					}
				}
				OPC->_float = OPA->_float / OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITAND_F):
				OPC->_float = (prvm_int_t)OPA->_float & (prvm_int_t)OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITOR_F):
				OPC->_float = (prvm_int_t)OPA->_float | (prvm_int_t)OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GE_F):
				OPC->_float = OPA->_float >= OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LE_F):
				OPC->_float = OPA->_float <= OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GT_F):
				OPC->_float = OPA->_float > OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LT_F):
				OPC->_float = OPA->_float < OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_AND_F):
				OPC->_float = PRVM_FLOAT_IS_TRUE_FOR_INT(OPA->_int) && PRVM_FLOAT_IS_TRUE_FOR_INT(OPB->_int); // TODO change this back to float, and add AND_I to be used by fteqcc for anything not a float
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_OR_F):
				OPC->_float = PRVM_FLOAT_IS_TRUE_FOR_INT(OPA->_int) || PRVM_FLOAT_IS_TRUE_FOR_INT(OPB->_int); // TODO change this back to float, and add OR_I to be used by fteqcc for anything not a float
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_F):
				OPC->_float = !PRVM_FLOAT_IS_TRUE_FOR_INT(OPA->_int);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_V):
				OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_S):
				OPC->_float = !OPA->string || !*PRVM_GetString(prog, OPA->string);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_FNC):
				OPC->_float = !OPA->function;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_ENT):
				OPC->_float = (OPA->edict == 0);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_F):
				OPC->_float = OPA->_float == OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_V):
				OPC->_float = (OPA->vector[0] == OPB->vector[0]) && (OPA->vector[1] == OPB->vector[1]) && (OPA->vector[2] == OPB->vector[2]);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_S):
				OPC->_float = !strcmp(PRVM_GetString(prog, OPA->string),PRVM_GetString(prog, OPB->string));
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_E):
				OPC->_float = OPA->_int == OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_FNC):
				OPC->_float = OPA->function == OPB->function;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_F):
				OPC->_float = OPA->_float != OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_V):
				OPC->_float = (OPA->vector[0] != OPB->vector[0]) || (OPA->vector[1] != OPB->vector[1]) || (OPA->vector[2] != OPB->vector[2]);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_S):
				OPC->_float = strcmp(PRVM_GetString(prog, OPA->string),PRVM_GetString(prog, OPB->string));
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_E):
				OPC->_float = OPA->_int != OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_FNC):
				OPC->_float = OPA->function != OPB->function;
				DISPATCH_OPCODE();

		//==================
			HANDLE_OPCODE(OP_STORE_F):
			HANDLE_OPCODE(OP_STORE_ENT):
			HANDLE_OPCODE(OP_STORE_FLD):		// integers
			HANDLE_OPCODE(OP_STORE_FNC):		// pointers
				OPB->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STORE_S):
				// refresh the garbage collection on the string - this guards
				// against a certain sort of repeated migration to earlier
				// points in the scan that could otherwise result in the string
				// being freed for being unused
				if(prvm_garbagecollection_enable.integer)
					PRVM_GetString(prog, OPA->_int);
				OPB->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STORE_V):
				OPB->ivector[0] = OPA->ivector[0];
				OPB->ivector[1] = OPA->ivector[1];
				OPB->ivector[2] = OPA->ivector[2];
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_STOREP_F):
			HANDLE_OPCODE(OP_STOREP_ENT):
			HANDLE_OPCODE(OP_STOREP_FLD):		// integers
			HANDLE_OPCODE(OP_STOREP_FNC):		// pointers
				addr = OPB->_uint + OPC->_uint;
				if ((ofs = addr - cached_vmentity1start) < cached_entityfieldsarea_entityfields)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields_entity1 + ofs);
				}
				else if ((ofs = addr - cached_vmglobal1) < cached_vmglobals_1)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(global1 + ofs);
				}
				else if ((ofs = addr - cached_vmentity0start) < cached_entityfields)
				{
					if (!cached_allowworldwrites)
					{
						PRE_ERROR();
						VM_Warning(prog, "Attempted assignment to world.%s (edictnum 0 field %"PRVM_PRIi"+%"PRVM_PRIi")\n", PRVM_GetString(prog, PRVM_ED_FieldAtOfs(prog, ofs)->s_name), OPB->_int, OPC->_int);
						// Perform entity write anyway.
					}
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an out of bounds address %"PRVM_PRIu"+%"PRVM_PRIi"", prog->name, OPB->_uint, OPC->_int);
					goto cleanup;
				}
				ptr->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STOREP_S):
				addr = OPB->_uint + OPC->_uint;
				if ((ofs = addr - cached_vmentity1start) < cached_entityfieldsarea_entityfields)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields_entity1 + ofs);
				}
				else if ((ofs = addr - cached_vmglobal1) < cached_vmglobals_1)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(global1 + ofs);
				}
				else if ((ofs = addr - cached_vmentity0start) < cached_entityfields)
				{
					if (!cached_allowworldwrites)
					{
						PRE_ERROR();
						VM_Warning(prog, "Attempted assignment to world.%s (edictnum 0 field %"PRVM_PRIi"+%"PRVM_PRIi")\n", PRVM_GetString(prog, PRVM_ED_FieldAtOfs(prog, ofs)->s_name), OPB->_int, OPC->_int);
						// Perform entity write anyway.
					}
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an out of bounds address %"PRVM_PRIu"+%"PRVM_PRIi"", prog->name, OPB->_uint, OPC->_int);
					goto cleanup;
				}
				// refresh the garbage collection on the string - this guards
				// against a certain sort of repeated migration to earlier
				// points in the scan that could otherwise result in the string
				// being freed for being unused
				if(prvm_garbagecollection_enable.integer)
					PRVM_GetString(prog, OPA->_int);
				ptr->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STOREP_V):
				addr = OPB->_uint + OPC->_uint;
				if ((ofs = addr - cached_vmentity1start) < cached_entityfieldsarea_entityfields_2)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields_entity1 + ofs);
				}
				else if ((ofs = addr - cached_vmglobal1) < cached_vmglobals_3)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(global1 + ofs);
				}
				else if ((ofs = addr - cached_vmentity0start) < cached_entityfields_2)
				{
					if (!cached_allowworldwrites)
					{
						PRE_ERROR();
						VM_Warning(prog, "Attempted assignment to world.%s (edictnum 0 field %"PRVM_PRIi"+%"PRVM_PRIi")\n", PRVM_GetString(prog, PRVM_ED_FieldAtOfs(prog, ofs)->s_name), OPB->_int, OPC->_int);
						// Perform entity write anyway.
					}
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an out of bounds address %"PRVM_PRIu"+%"PRVM_PRIi"", prog->name, OPB->_uint, OPC->_int);
					goto cleanup;
				}
				ptr->ivector[0] = OPA->ivector[0];
				ptr->ivector[1] = OPA->ivector[1];
				ptr->ivector[2] = OPA->ivector[2];
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_ADDRESS):
				if ((prvm_uint_t)OPA->edict >= cached_max_edicts)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to address an out of bounds edict number", prog->name);
					goto cleanup;
				}
				if (OPB->_uint >= cached_entityfields)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to address an invalid field (%"PRVM_PRIu") in an edict", prog->name, OPB->_uint);
					goto cleanup;
				}
#if 0
				if (OPA->edict == 0 && !cached_allowworldwrites)
				{
					PRE_ERROR();
					prog->error_cmd("Forbidden assignment to world (edictnum 0) in %s", prog->name);
					goto cleanup;
				}
#endif
				OPC->_int = cached_vmentity0start + OPA->edict * cached_entityfields + OPB->_int;
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_LOAD_F):
			HANDLE_OPCODE(OP_LOAD_FLD):
			HANDLE_OPCODE(OP_LOAD_ENT):
			HANDLE_OPCODE(OP_LOAD_FNC):
			HANDLE_OPCODE(OP_LOAD_P):
				if ((prvm_uint_t)OPA->edict >= cached_max_edicts)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an out of bounds edict number", prog->name);
					goto cleanup;
				}
				if (OPB->_uint >= cached_entityfields)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid field in an edict (%"PRVM_PRIu")", prog->name, OPB->_uint);
					goto cleanup;
				}
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = ((prvm_eval_t *)(ed->fields.ip + OPB->_int))->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOAD_S):
				if ((prvm_uint_t)OPA->edict >= cached_max_edicts)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an out of bounds edict number", prog->name);
					goto cleanup;
				}
				if (OPB->_uint >= cached_entityfields)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid field in an edict (%"PRVM_PRIu")", prog->name, OPB->_uint);
					goto cleanup;
				}
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = ((prvm_eval_t *)(ed->fields.ip + OPB->_int))->_int;
				// refresh the garbage collection on the string - this guards
				// against a certain sort of repeated migration to earlier
				// points in the scan that could otherwise result in the string
				// being freed for being unused
				if(prvm_garbagecollection_enable.integer)
					PRVM_GetString(prog, OPC->_int);
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_LOAD_V):
				if ((prvm_uint_t)OPA->edict >= cached_max_edicts)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an out of bounds edict number", prog->name);
					goto cleanup;
				}
				if (OPB->_uint >= cached_entityfields_2)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid field in an edict (%"PRVM_PRIu")", prog->name, OPB->_uint);
					goto cleanup;
				}
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				ptr = (prvm_eval_t *)(ed->fields.ip + OPB->_int);
				OPC->ivector[0] = ptr->ivector[0];
				OPC->ivector[1] = ptr->ivector[1];
				OPC->ivector[2] = ptr->ivector[2];
				DISPATCH_OPCODE();

		//==================

			HANDLE_OPCODE(OP_IFNOT):
				//spike FIXME -- dp redefined IFNOT[_I] as IFNOT_F, which breaks if(0x80000000)
				//spike FIXME -- you should add separate IFNOT_I/IFNOT_F opcodes and remap IFNOT_I to ITNOT_F in v6 progs for compat.
				if(!FLOAT_IS_TRUE_FOR_INT(OPA->_int))
				// TODO add an "int-if", and change this one to OPA->_float
				// although mostly unneeded, thanks to the only float being false being 0x0 and 0x80000000 (negative zero)
				// and entity, string, field values can never have that value
				{
					ADVANCE_PROFILE_BEFORE_JUMP();
					st += st->operand[1] - 1;	// offset the st++
					startst = st;
					// no bounds check needed, it is done when loading progs
					if (++jumpcount == 10000000 && prvm_runawaycheck)
					{
						prog->xstatement = st - cached_statements;
						PRVM_Profile(prog, 1<<30, 1000000, 0);
						prog->error_cmd("%s runaway loop counter hit limit of %d jumps\ntip: read above for list of most-executed functions", prog->name, jumpcount);
					}
				}
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_IF):
				//spike FIXME -- dp redefined IF[_I] as IF_F
				// TODO: plan is:
				// - Add IF_F, IFNOT_F.
				// - Rename this to IF_I, IFNOT_I.
				// - Add sv_gameplayfix variable to remap IF_I and IFNOT_I to IF_F and IFNOT_F on progs load.
				// - Define this fix for Nexuiz.
				// - Add generation of IF_F, IFNOT_F instructions to gmqcc.
				// - Move Xonotic to that.
				if(FLOAT_IS_TRUE_FOR_INT(OPA->_int))
				// TODO add an "int-if", and change this one, as well as the FLOAT_IS_TRUE_FOR_INT usages, to OPA->_float
				// although mostly unneeded, thanks to the only float being false being 0x0 and 0x80000000 (negative zero)
				// and entity, string, field values can never have that value
				{
					ADVANCE_PROFILE_BEFORE_JUMP();
					st += st->operand[1] - 1;	// offset the st++
					startst = st;
					// no bounds check needed, it is done when loading progs
					if (++jumpcount == 10000000 && prvm_runawaycheck)
					{
						prog->xstatement = st - cached_statements;
						PRVM_Profile(prog, 1<<30, 0.01, 0);
						prog->error_cmd("%s runaway loop counter hit limit of %d jumps\ntip: read above for list of most-executed functions", prog->name, jumpcount);
					}
				}
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_GOTO):
				ADVANCE_PROFILE_BEFORE_JUMP();
				st += st->operand[0] - 1;	// offset the st++
				startst = st;
				// no bounds check needed, it is done when loading progs
				if (++jumpcount == 10000000 && prvm_runawaycheck)
				{
					prog->xstatement = st - cached_statements;
					PRVM_Profile(prog, 1<<30, 0.01, 0);
					prog->error_cmd("%s runaway loop counter hit limit of %d jumps\ntip: read above for list of most-executed functions", prog->name, jumpcount);
				}
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_CALL0):
			HANDLE_OPCODE(OP_CALL1):
			HANDLE_OPCODE(OP_CALL2):
			HANDLE_OPCODE(OP_CALL3):
			HANDLE_OPCODE(OP_CALL4):
			HANDLE_OPCODE(OP_CALL5):
			HANDLE_OPCODE(OP_CALL6):
			HANDLE_OPCODE(OP_CALL7):
			HANDLE_OPCODE(OP_CALL8):
#ifdef PRVMTIMEPROFILING
				tm = Sys_DirtyTime();
				prog->xfunction->tprofile += (tm - starttm >= 0 && tm - starttm < 1800) ? (tm - starttm) : 0;
				starttm = tm;
#endif
				ADVANCE_PROFILE_BEFORE_JUMP();
				startst = st;
				prog->xstatement = st - cached_statements;
				prog->argc = st->op - OP_CALL0;
				if (!OPA->function)
				{
					prog->error_cmd("NULL function in %s", prog->name);
				}

				if(!OPA->function || OPA->function < 0 || OPA->function >= prog->numfunctions)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted CALL outside the program", prog->name);
					goto cleanup;
				}

				enterfunc = &prog->functions[OPA->function];
				if (enterfunc->callcount++ == 0 && (prvm_coverage.integer & 1))
					PRVM_FunctionCoverageEvent(prog, enterfunc);

				if (enterfunc->first_statement < 0)
				{
					// negative first_statement values are built in functions
					int builtinnumber = -enterfunc->first_statement;
					prog->xfunction->builtinsprofile++;
					if (builtinnumber < prog->numbuiltins && prog->builtins[builtinnumber])
					{
						prog->builtins[builtinnumber](prog);
#ifdef PRVMTIMEPROFILING
						tm = Sys_DirtyTime();
						enterfunc->tprofile += (tm - starttm >= 0 && tm - starttm < 1800) ? (tm - starttm) : 0;
						prog->xfunction->tbprofile += (tm - starttm >= 0 && tm - starttm < 1800) ? (tm - starttm) : 0;
						starttm = tm;
#endif
						// builtins may cause ED_Alloc() to be called, update cached variables
						CACHE_CHANGING(NO_DECLARE);
						// these do not change
						//cached_statements = prog->statements;
						//cached_allowworldwrites = prog->allowworldwrites;
						//cached_flag = prog->flag;
						// if prog->trace changed we need to change interpreter path
						if (prog->trace != cachedpr_trace)
							goto chooseexecprogram;
					}
					else
						prog->error_cmd("No such builtin #%i in %s. This program is corrupt or incompatible with DarkPlaces (or this version of it)", builtinnumber, prog->name);
				}
				else
					st = cached_statements + PRVM_EnterFunction(prog, enterfunc);
				startst = st;
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_DONE):
			HANDLE_OPCODE(OP_RETURN):
#ifdef PRVMTIMEPROFILING
				tm = Sys_DirtyTime();
				prog->xfunction->tprofile += (tm - starttm >= 0 && tm - starttm < 1800) ? (tm - starttm) : 0;
				starttm = tm;
#endif
				ADVANCE_PROFILE_BEFORE_JUMP();
				prog->xstatement = st - cached_statements;

				prog->globals.ip[OFS_RETURN  ] = prog->globals.ip[st->operand[0]  ];
				prog->globals.ip[OFS_RETURN+1] = prog->globals.ip[st->operand[0]+1];
				prog->globals.ip[OFS_RETURN+2] = prog->globals.ip[st->operand[0]+2];

				st = cached_statements + PRVM_LeaveFunction(prog);
				startst = st;
				if (prog->depth <= exitdepth)
					goto cleanup; // all done
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_STATE):
				if(cached_flag & PRVM_OP_STATE)
				{
					ed = PRVM_PROG_TO_EDICT(PRVM_gameglobaledict(self));
					PRVM_gameedictfloat(ed,nextthink) = PRVM_gameglobalfloat(time) + 0.1;
					PRVM_gameedictfloat(ed,frame) = OPA->_float;
					PRVM_gameedictfunction(ed,think) = OPB->function;
				}
				else
				{
					PRE_ERROR();
					prog->xstatement = st - cached_statements;
					prog->error_cmd("OP_STATE not supported by %s", prog->name);
				}
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_ADD_I):
				OPC->_int = OPA->_int + OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_ADD_IF):
				OPC->_float = ((prvm_vec_t) OPA->_int) + OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_ADD_FI):
				OPC->_float = OPA->_float + (prvm_vec_t) OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_SUB_I):
				OPC->_int = OPA->_int - OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_SUB_IF):
				OPC->_float = ((prvm_vec_t) OPA->_int) - OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_SUB_FI):
				OPC->_float = OPA->_float - (prvm_vec_t) OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_I):
				OPC->_int = OPA->_int * OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_IF):
				OPC->_float = ((prvm_vec_t) OPA->_int) * OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_FI):
				OPC->_float = OPA->_float * (prvm_vec_t) OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_MUL_VI):
				OPC->vector[0] = (prvm_vec_t) OPB->_int * OPA->vector[0];
				OPC->vector[1] = (prvm_vec_t) OPB->_int * OPA->vector[1];
				OPC->vector[2] = (prvm_vec_t) OPB->_int * OPA->vector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_VF):
				if( OPB->_float == 0.0f )
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of '%f %f %f' by zero\n", OPA->vector[0], OPA->vector[1], OPA->vector[2]);
					if (prvm_gameplayfix_div0is0.integer)
					{
						OPC->vector[0] = 0;
						OPC->vector[1] = 0;
						OPC->vector[2] = 0;
						DISPATCH_OPCODE();
					}
				}
				tempfloat = OPB->_float;
				OPC->vector[0] = OPA->vector[0] / tempfloat;
				OPC->vector[1] = OPA->vector[1] / tempfloat;
				OPC->vector[2] = OPA->vector[2] / tempfloat;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_I):
				// NOTE: This also catches the second kind of division that can trap, namely, -2147483648 / -1,
				// whose result is not representable as int32_t and raises the same CPU exception.
				if( OPB->_int != 0 && (OPB->_int != -1 || OPA->_int != PRVM_INT_MIN) )
				{
					OPC->_int = OPA->_int / OPB->_int;
				}
				else
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of %"PRVM_PRIi" by %"PRVM_PRIi"\n", OPA->_int, OPB->_int);
					OPC->_int = 0;
				}
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_IF):
				if( OPB->_float == 0.0f )
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of %"PRVM_PRIi" by zero\n", OPA->_int);
					if (prvm_gameplayfix_div0is0.integer)
					{
						OPC->_float = 0;
						DISPATCH_OPCODE();
					}
				}
				OPC->_float = ((prvm_vec_t) OPA->_int) / OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_FI):
				if( OPB->_int == 0 )
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of %f by zero\n", OPA->_float);
					if (prvm_gameplayfix_div0is0.integer)
					{
						OPC->_float = 0;
						DISPATCH_OPCODE();
					}
				}
				OPC->_float = OPA->_float / (prvm_vec_t) OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_CONV_ITOF):
				OPC->_float = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_CONV_FTOI):
				OPC->_int = OPA->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITAND_I):
				OPC->_int = OPA->_int & OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITOR_I):
				OPC->_int = OPA->_int | OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITAND_IF):
				OPC->_int = OPA->_int & (prvm_int_t)OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITOR_IF):
				OPC->_int = OPA->_int | (prvm_int_t)OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITAND_FI):
				OPC->_int = (prvm_int_t)OPA->_float & OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_BITOR_FI):
				OPC->_int = (prvm_int_t)OPA->_float | OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GE_I):
				OPC->_int = OPA->_int >= OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LE_I):
				OPC->_int = OPA->_int <= OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GT_I):
				OPC->_int = OPA->_int > OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LT_I):
				OPC->_int = OPA->_int < OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_AND_I):
				OPC->_int = OPA->_int && OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_OR_I):
				OPC->_int = OPA->_int || OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GE_IF):
				OPC->_int = (prvm_vec_t)OPA->_int >= OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LE_IF):
				OPC->_int = (prvm_vec_t)OPA->_int <= OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GT_IF):
				OPC->_int = (prvm_vec_t)OPA->_int > OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LT_IF):
				OPC->_int = (prvm_vec_t)OPA->_int < OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_AND_IF):
				OPC->_int = (prvm_vec_t)OPA->_int && OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_OR_IF):
				OPC->_int = (prvm_vec_t)OPA->_int || OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GE_FI):
				OPC->_int = OPA->_float >= (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LE_FI):
				OPC->_int = OPA->_float <= (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GT_FI):
				OPC->_int = OPA->_float > (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LT_FI):
				OPC->_int = OPA->_float < (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_AND_FI):
				OPC->_int = OPA->_float && (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_OR_FI):
				OPC->_int = OPA->_float || (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NOT_I):
				OPC->_int = !OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_I):
				OPC->_int = OPA->_int == OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_IF):
				OPC->_int = (prvm_vec_t)OPA->_int == OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_EQ_FI):
				OPC->_int = OPA->_float == (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_I):
				OPC->_int = OPA->_int != OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_IF):
				OPC->_int = (prvm_vec_t)OPA->_int != OPB->_float;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_NE_FI):
				OPC->_int = OPA->_float != (prvm_vec_t)OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STORE_I):
			HANDLE_OPCODE(OP_STORE_P):
				OPB->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_STOREP_I):
#if PRBOUNDSCHECK
				if (OPB->_int < 0 || OPB->_int + 4 > pr_edictareasize)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an out of bounds edict", prog->name);
					goto cleanup;
				}
#endif
				ptr = (prvm_eval_t *)(prog->edictsfields.ip + OPB->_int);
				ptr->_int = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOAD_I):
#if PRBOUNDSCHECK
				if (OPA->edict < 0 || OPA->edict >= prog->max_edicts)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an out of bounds edict number", prog->name);
					goto cleanup;
				}
				if (OPB->_int < 0 || OPB->_int >= progs->entityfields.ip)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid field in an edict", prog->name);
					goto cleanup;
				}
#endif
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = ((prvm_eval_t *)((int *)ed->fields.ip + OPB->_int))->_int;
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_GSTOREP_I):
			HANDLE_OPCODE(OP_GSTOREP_F):
			HANDLE_OPCODE(OP_GSTOREP_ENT):
			HANDLE_OPCODE(OP_GSTOREP_FLD):		// integers
			HANDLE_OPCODE(OP_GSTOREP_S):
			HANDLE_OPCODE(OP_GSTOREP_FNC):		// pointers
				if (OPB->_int <= 0 || OPB->_int >= prog->numglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an invalid indexed global", prog->name);
					goto cleanup;
				}
				prog->globals.ip[OPB->_int] = OPA->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_GSTOREP_V):
				if (OPB->_int <= 0 || OPB->_int + 2 >= prog->numglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to write to an invalid indexed global", prog->name);
					goto cleanup;
				}
				prog->globals.ip[OPB->_int  ] = OPA->ivector[0];
				prog->globals.ip[OPB->_int+1] = OPA->ivector[1];
				prog->globals.ip[OPB->_int+2] = OPA->ivector[2];
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_GLOAD_I):
			HANDLE_OPCODE(OP_GLOAD_F):
			HANDLE_OPCODE(OP_GLOAD_FLD):
			HANDLE_OPCODE(OP_GLOAD_ENT):
			HANDLE_OPCODE(OP_GLOAD_S):
			HANDLE_OPCODE(OP_GLOAD_FNC):
				if (OPA->_int < 0 || OPA->_int >= prog->numglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid indexed global", prog->name);
					goto cleanup;
				}
				OPC->_int = prog->globals.ip[OPA->_int];
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_GLOAD_V):
				if (OPA->_int < 0 || OPA->_int + 2 >= prog->numglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read an invalid indexed global", prog->name);
					goto cleanup;
				}
				OPC->ivector[0] = prog->globals.ip[OPA->_int  ];
				OPC->ivector[1] = prog->globals.ip[OPA->_int+1];
				OPC->ivector[2] = prog->globals.ip[OPA->_int+2];
				DISPATCH_OPCODE();

			HANDLE_OPCODE(OP_BOUNDCHECK):
				if ((unsigned int)OPA->_int < (unsigned int)st->operand[2] || (unsigned int)OPA->_int >= (unsigned int)st->operand[1])
				{
					PRE_ERROR();
					prog->error_cmd("Progs boundcheck failed in %s, value is < %"PRVM_PRIi" or >= %"PRVM_PRIi"", prog->name, OPC->_int, OPB->_int);
					goto cleanup;
				}
				DISPATCH_OPCODE();

			// FTEQW pointer instructions.
			HANDLE_OPCODE(OP_GLOBALADDRESS):
				OPC->_int = PRVM_GLOBALSBASE + st->operand[0] + 1 * OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_ADD_PIW):
				OPC->_int = OPA->_int + 1 * OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADA_F):
			HANDLE_OPCODE(OP_LOADA_ENT):
			HANDLE_OPCODE(OP_LOADA_FLD):
			HANDLE_OPCODE(OP_LOADA_FNC):
			HANDLE_OPCODE(OP_LOADA_I):
				ofs = st->operand[0] + OPB->_int;
				if (ofs >= cached_vmglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)st->operand[0], OPB->_int);
					goto cleanup;
				}
				src = (prvm_eval_t *)&globals[ofs];
				OPC->_int = src->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADA_S):
				ofs = st->operand[0] + OPB->_int;
				if (ofs >= cached_vmglobals)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)st->operand[0], OPB->_int);
					goto cleanup;
				}
				src = (prvm_eval_t *)&globals[ofs];
				// refresh the garbage collection on the string - this guards
				// against a certain sort of repeated migration to earlier
				// points in the scan that could otherwise result in the string
				// being freed for being unused
				if(prvm_garbagecollection_enable.integer)
					PRVM_GetString(prog, src->_int);
				OPC->_int = src->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADA_V):
				ofs = st->operand[0] + OPB->_int;
				if (ofs >= cached_vmglobals_2)
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)st->operand[0], OPB->_int);
					goto cleanup;
				}
				src = (prvm_eval_t *)&globals[ofs];
				OPC->ivector[0] = src->ivector[0];
				OPC->ivector[1] = src->ivector[1];
				OPC->ivector[2] = src->ivector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADP_F):
			HANDLE_OPCODE(OP_LOADP_ENT):
			HANDLE_OPCODE(OP_LOADP_FLD):		// integers
			HANDLE_OPCODE(OP_LOADP_FNC):		// pointers
			HANDLE_OPCODE(OP_LOADP_I):
				addr = OPA->_uint + OPB->_uint;
				if ((ofs = addr - cached_vmentity0start) < cached_entityfieldsarea)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else if ((ofs = addr - PRVM_GLOBALSBASE) < cached_vmglobals)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(globals + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)OPA->_int, OPB->_int);
					goto cleanup;
				}
				OPC->_int = ptr->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADP_S):
				addr = OPA->_uint + OPB->_uint;
				if ((ofs = addr - cached_vmentity0start) < cached_entityfieldsarea)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else if ((ofs = addr - PRVM_GLOBALSBASE) < cached_vmglobals)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(globals + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)OPA->_int, OPB->_int);
					goto cleanup;
				}
				if(prvm_garbagecollection_enable.integer)
					PRVM_GetString(prog, ptr->_int);
				OPC->_int = ptr->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LOADP_V):
				addr = OPA->_uint + OPB->_uint;
				if ((ofs = addr - cached_vmentity0start) < cached_entityfieldsarea_2)
				{
					// OK entity write.
					ptr = (prvm_eval_t *)(cached_edictsfields + ofs);
				}
				else if ((ofs = addr - PRVM_GLOBALSBASE) < cached_vmglobals_2)
				{
					// OK global write.
					ptr = (prvm_eval_t *)(globals + ofs);
				}
				else
				{
					PRE_ERROR();
					prog->error_cmd("%s attempted to read from an out of bounds address %u+%"PRVM_PRIi"", prog->name, (unsigned int)OPA->_int, OPB->_int);
					goto cleanup;
				}
				OPC->ivector[0] = ptr->ivector[0];
				OPC->ivector[1] = ptr->ivector[1];
				OPC->ivector[2] = ptr->ivector[2];
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_RSHIFT_I):
				OPC->_int = OPA->_int >> OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LSHIFT_I):
				OPC->_int = OPA->_int << OPB->_int;
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LE_U):
				OPC->_int = (OPA->_uint <= OPB->_uint);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_LT_U):
				OPC->_int = (OPA->_uint < OPB->_uint);
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_DIV_U):
				if( OPB->_uint != 0 )
				{
					OPC->_uint = OPA->_uint / OPB->_uint;
				}
				else
				{
					PRE_ERROR();
					VM_Warning(prog, "Attempted division of %"PRVM_PRIu" by zero\n", OPA->_uint);
					OPC->_uint = 0;
				}
				DISPATCH_OPCODE();
			HANDLE_OPCODE(OP_RSHIFT_U):
				OPC->_uint = OPA->_uint >> OPB->_uint;
				DISPATCH_OPCODE();

#if !USE_COMPUTED_GOTOS
			default:
				PRE_ERROR();
				prog->error_cmd("Bad opcode %i in %s. This program is corrupt or incompatible with DarkPlaces (or this version of it)", st->op, prog->name);
				goto cleanup;
			}
#if PRVMSLOWINTERPRETER
			{
				if (prog->watch_global_type != ev_void)
				{
					prvm_eval_t *g = PRVM_GLOBALFIELDVALUE(prog->watch_global);
					prog->xstatement = st - cached_statements;
					PRVM_Watchpoint(prog, 0, "Global watchpoint hit", prog->watch_global_type, &prog->watch_global_value, g);
				}
				if (prog->watch_field_type != ev_void && prog->watch_edict < prog->max_edicts)
				{
					prvm_eval_t *g = PRVM_EDICTFIELDVALUE(prog->edicts + prog->watch_edict, prog->watch_field);
					prog->xstatement = st - cached_statements;
					PRVM_Watchpoint(prog, 0, "Entityfield watchpoint hit", prog->watch_field_type, &prog->watch_edictfield_value, g);
				}
			}
#endif
		}
#endif // !USE_COMPUTED_GOTOS

#undef DISPATCH_OPCODE
#undef HANDLE_OPCODE
#undef USE_COMPUTED_GOTOS
#undef PRE_ERROR
#undef ADVANCE_PROFILE_BEFORE_JUMP
