
static void* dispatch_table[OP_COUNT] = {
	&& L_MOV_II_VV,
	&& L_MOV_II_VC,

	&& L_MOV_IN_ARG,
	&& L_MOV_IN_ARG_CONST,

	&& L_MOV_OUT_ARG,

	&& L_ADD_III_VVV,
	&& L_ADD_III_VVC,

	&& L_SUB_III_VVC,

	&& L_CALL,
	&& L_CALL_C,
	&& L_CALL_V,

	&& L_RET_I_V,
	&& L_RET_I_C,
	&& L_RET_ADD_II_VV,

	&& L_ALLOC,

	&& L_DEALLOC,

	&& L_IF_LE_II_VC,

	&& L_PRINT,

	&& L_GOTO,

	&& L_eOF
};