#pragma once
enum : uint8_t {

	MOV_II_VV, 
	MOV_II_VC, 

	MOV_IN_ARG,
	MOV_IN_ARG_CONST,

	MOV_OUT_ARG,

	ADD_III_VVV, 
	ADD_III_VVC, 

	SUB_III_VVC, 

	CALL, 
	CALL_C, 
	CALL_V, 

	RET_I_V,
	RET_I_C,

	RET_ADD_II_VV, 
	

	ALLOC, 
	DEALLOC, 

	IF_LE_II_VC,

	PRINT, 

	GOTO,

	eOF, 
	OP_COUNT
};
enum : uint8_t {
	VAR_I,
	VAR_S,
	LIT_I,
	LIT_S,


};


struct Command {
	int16_t op;
	intptr_t mem1;
	intptr_t mem2;
	intptr_t mem3;
};



Command  pushcodetest[] =
{

{LIT_S,0,0,0},
{VAR_I,1,0,0},
{LIT_S,1,0,0},
{eOF,0,0,0},

};




Command  bytecodetest[] =
{

	{ALLOC,10,0,0},
	{MOV_II_VC,9,0,0},
	{IF_LE_II_VC,8,9,30},


	{CALL_V,10,10,9},
	{MOV_OUT_ARG,1,1,0},


	{PRINT,0,0,0},

	{ADD_III_VVC,9,9,1},
	{GOTO,2,0,0},



	{DEALLOC,10,0,0},
	{eOF,0,0,0},

		

	{MOV_OUT_ARG,1,1,0},

	
	{IF_LE_II_VC,13,1,1},
	{RET_I_V,10,1,0},

	
	{SUB_III_VVC,2,1,1},

	{CALL_V,10,10,2},
	{MOV_OUT_ARG,4,1,0},

	{SUB_III_VVC,3,1,2},

	{CALL_V,10,10,3},
	{MOV_OUT_ARG,5,1,0},

	{RET_ADD_II_VV,10,4,5},


};



std::vector<std::string> str = {
"Result: ",
"\n",


};

std::vector<Command> bytecode;
std::vector<Command> pushcode;
