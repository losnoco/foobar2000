#ifndef _RSP_H_
#define _RSP_H_


#include <windows.h>
#include <stdint.h>
#include "types.h"

#define CPU_Message(...)


#pragma pack(push,1)

typedef struct tagOPCODE {
	union {

		uint32_t Hex;
		uint8_t Ascii[4];

		struct {
			unsigned offset : 16;
			unsigned rt : 5;
			unsigned rs : 5;
			unsigned op : 6;
		};

		struct {
			unsigned immediate : 16;
			unsigned : 5;
			unsigned base : 5;
			unsigned : 6;
		};

		struct {
			unsigned target : 26;
			unsigned : 6;
		};

		struct {
			unsigned funct : 6;
			unsigned sa : 5;
			unsigned rd : 5;
			unsigned : 5;
			unsigned : 5;
			unsigned : 6;
		};

		struct {
			signed   voffset : 7;
			unsigned del	: 4;
			unsigned : 5;
			unsigned dest   : 5;
			unsigned : 5;
			unsigned : 6;
		};

	};
} RSPOPCODE;

#pragma pack(pop)

#define REGISTER32	MIPS_WORD
#define REGISTER	MIPS_DWORD


//RSP OpCodes
#define	RSP_SPECIAL				 0
#define	RSP_REGIMM				 1
#define RSP_J					 2
#define RSP_JAL					 3
#define RSP_BEQ					 4
#define RSP_BNE					 5
#define RSP_BLEZ				 6
#define RSP_BGTZ				 7
#define RSP_ADDI				 8
#define RSP_ADDIU				 9
#define RSP_SLTI				10
#define RSP_SLTIU				11
#define RSP_ANDI				12
#define RSP_ORI					13
#define RSP_XORI				14
#define RSP_LUI					15
#define	RSP_CP0					16
#define	RSP_CP2					18
#define RSP_LB					32
#define RSP_LH					33
#define RSP_LW					35
#define RSP_LBU					36
#define RSP_LHU					37
#define RSP_SB					40
#define RSP_SH					41
#define RSP_SW					43
#define RSP_LC2					50
#define RSP_SC2					58

/* RSP Special opcodes */
#define RSP_SPECIAL_SLL			 0
#define RSP_SPECIAL_SRL			 2
#define RSP_SPECIAL_SRA			 3
#define RSP_SPECIAL_SLLV		 4
#define RSP_SPECIAL_SRLV		 6
#define RSP_SPECIAL_SRAV		 7
#define RSP_SPECIAL_JR			 8
#define RSP_SPECIAL_JALR		 9
#define RSP_SPECIAL_BREAK		13
#define RSP_SPECIAL_ADD			32
#define RSP_SPECIAL_ADDU		33
#define RSP_SPECIAL_SUB			34
#define RSP_SPECIAL_SUBU		35
#define RSP_SPECIAL_AND			36
#define RSP_SPECIAL_OR			37
#define RSP_SPECIAL_XOR			38
#define RSP_SPECIAL_NOR			39
#define RSP_SPECIAL_SLT			42
#define RSP_SPECIAL_SLTU		43

/* RSP RegImm opcodes */
#define RSP_REGIMM_BLTZ			 0
#define RSP_REGIMM_BGEZ			 1
#define RSP_REGIMM_BLTZAL		16
#define RSP_REGIMM_BGEZAL		17

/* RSP COP0 opcodes */
#define	RSP_COP0_MF				 0
#define	RSP_COP0_MT				 4

/* RSP COP2 opcodes */
#define	RSP_COP2_MF				 0
#define	RSP_COP2_CF				 2
#define	RSP_COP2_MT				 4
#define	RSP_COP2_CT				 6

/* RSP Vector opcodes */
#define	RSP_VECTOR_VMULF		 0
#define	RSP_VECTOR_VMULU		 1
#define	RSP_VECTOR_VRNDP		 2
#define	RSP_VECTOR_VMULQ		 3
#define	RSP_VECTOR_VMUDL		 4
#define	RSP_VECTOR_VMUDM		 5
#define	RSP_VECTOR_VMUDN		 6
#define	RSP_VECTOR_VMUDH		 7
#define	RSP_VECTOR_VMACF		 8
#define	RSP_VECTOR_VMACU		 9
#define	RSP_VECTOR_VRNDN		10
#define	RSP_VECTOR_VMACQ		11
#define	RSP_VECTOR_VMADL		12
#define	RSP_VECTOR_VMADM		13
#define	RSP_VECTOR_VMADN		14
#define	RSP_VECTOR_VMADH		15
#define	RSP_VECTOR_VADD			16
#define	RSP_VECTOR_VSUB			17
#define	RSP_VECTOR_VABS			19
#define	RSP_VECTOR_VADDC		20
#define	RSP_VECTOR_VSUBC		21
#define	RSP_VECTOR_VSAW			29
#define	RSP_VECTOR_VLT			32
#define	RSP_VECTOR_VEQ			33
#define	RSP_VECTOR_VNE			34
#define	RSP_VECTOR_VGE			35
#define	RSP_VECTOR_VCL			36
#define	RSP_VECTOR_VCH			37
#define	RSP_VECTOR_VCR			38
#define	RSP_VECTOR_VMRG			39
#define	RSP_VECTOR_VAND			40
#define	RSP_VECTOR_VNAND		41
#define	RSP_VECTOR_VOR			42
#define	RSP_VECTOR_VNOR			43
#define	RSP_VECTOR_VXOR			44
#define	RSP_VECTOR_VNXOR		45
#define	RSP_VECTOR_VRCP			48
#define	RSP_VECTOR_VRCPL		49
#define	RSP_VECTOR_VRCPH		50
#define	RSP_VECTOR_VMOV			51
#define	RSP_VECTOR_VRSQ			52
#define	RSP_VECTOR_VRSQL		53
#define	RSP_VECTOR_VRSQH		54
#define	RSP_VECTOR_VNOOP		55

/* RSP LSC2 opcodes */
#define RSP_LSC2_BV				 0
#define RSP_LSC2_SV				 1
#define RSP_LSC2_LV				 2
#define RSP_LSC2_DV				 3
#define RSP_LSC2_QV				 4
#define RSP_LSC2_RV				 5
#define RSP_LSC2_PV				 6
#define RSP_LSC2_UV				 7
#define RSP_LSC2_HV				 8
#define RSP_LSC2_FV				 9
#define RSP_LSC2_WV				10
#define	RSP_LSC2_TV				11

#define MaxMaps	32


//extern uint8_t * RSPRecompPos, * RSPRecompCode, * RSPRecompCodeSecondary;
//extern uint8_t * RSPJumpTable;*/


#define NORMAL					0
#define DO_DELAY_SLOT			1
#define DO_END_DELAY_SLOT		2
#define DELAY_SLOT				3
#define END_DELAY_SLOT			4
#define LIKELY_DELAY_SLOT		5
#define JUMP	 				6
#define DELAY_SLOT_DONE			7
#define LIKELY_DELAY_SLOT_DONE	8
#define END_BLOCK 				9
#define FINISH_BLOCK			10 // from RSP Recompiler CPU
#define FINISH_SUB_BLOCK		11 // from RSP Recompiler CPU

#define RSPCALL _cdecl

#ifdef __cplusplus

#include "rsp_x86.h"

typedef struct
{
	union {
		uint32_t DestReg;
		uint32_t StoredReg;
	};
	union {
		uint32_t SourceReg0;
		uint32_t IndexReg;
	};
	uint32_t SourceReg1;
	uint32_t flags;
} OPCODE_INFO;


#define CALL_RSPFUNC(x) \
	RSPPushImm32((uint32_t)this); \
	RSPCall_Direct(GetAddress(0, x)); \
	RSPAddConstToX86Reg(x86_ESP, 4); \

#include "rsp_recompiler_cpu.h"

class Rsp : public RSPX86
{
	public:
		Rsp(class Cpu & mCpu);
		~Rsp();
		void InitRsp(void);
		int32_t Run(uint32_t cycles);

		REGISTER32 *RSP_GPR, RSP_Flags[4];
		REGISTER * RSP_ACCUM;
		VECTOR * RSP_Vect;
		uint32_t NoOfMaps, MapsCRC[MaxMaps], Table;
		uint8_t * RSPRecompCode, * RSPRecompCodeSecondary, *RSPJumpTables;
		void ** RSPJumpTable;
		RSP_COMPILER Compiler;
		REGISTER EleSpec[32], Indx[32];
		RSPOPCODE RSPOpC;
		uint32_t *PrgCount, RSPNextInstruction;
		REGISTER32 Recp, RecpResult, SQroot, SQrootResult;
		uint32_t ConditionalMove;

		uint32_t RSPCompilePC, RSPBlockID;
		uint32_t dwBuffer;

		RSP_BLOCK RSPCurrentBlock;
		RSP_CODE RspCode;

		uint8_t * pLastSecondary, * pLastPrimary;


		uint32_t ESP_RegSave, EBP_RegSave;
		uint32_t RSPBranchCompare;


		uint32_t RSP_NextInstruction, RSP_JumpTo;


		uint32_t RSP_Running;

		int32_t bDelayAffect;
		uint32_t VXOR_DynaRegCount;

		void (RSPCALL Rsp::*RSP_RegImm[32])();
		void (RSPCALL Rsp::*RSP_Special[64])();
		void (RSPCALL Rsp::*RSP_Cop0[32])();
		void (RSPCALL Rsp::*RSP_Cop2[32])();
		void (RSPCALL Rsp::*RSP_Vector[64])();
		void (RSPCALL Rsp::*RSP_Lc2[32])();
		void (RSPCALL Rsp::*RSP_Sc2[32])();
		void (RSPCALL Rsp::*RSP_Opcode[64])();


		void RSPCALL RSP_SP_DMA_READ (void);
		void RSPCALL RSP_SP_DMA_WRITE(void);


	protected:
		class Cpu & cpu;

	private:
		int32_t RSPAllocateMemory(void);
		void InitilizeRSPRegisters (void);

		void RSPCALL RSP_LB_DMEM  ( uint32_t Addr, uint8_t * Value );
		void RSPCALL RSP_LBV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LDV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LFV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LH_DMEM  ( uint32_t Addr, uint16_t * Value );
		void RSPCALL RSP_LHV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LLV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LPV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LRV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LQV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LSV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LTV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LUV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_LW_DMEM  ( uint32_t Addr, uint32_t * Value );
		void RSPCALL RSP_LW_IMEM  ( uint32_t Addr, uint32_t * Value );
		void RSPCALL RSP_SB_DMEM  ( uint32_t Addr, uint8_t Value );
		void RSPCALL RSP_SBV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SDV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SFV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SH_DMEM  ( uint32_t Addr, uint16_t Value );
		void RSPCALL RSP_SHV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SLV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SPV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SQV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SRV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SSV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_STV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SUV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSP_SW_DMEM  ( uint32_t Addr, uint32_t Value );
		void RSPCALL RSP_SWV_DMEM ( uint32_t Addr, int32_t vect, int32_t element );
		void RSPCALL RSPSetJumpTable (void) ;

		#include "rsp_recompiler_ops.h"

		int32_t WriteToAccum (int32_t Location, int32_t PC);
		int32_t WriteToVectorDest (uint32_t DestReg, int32_t PC);
		int32_t UseRspFlags (int32_t PC);
		int32_t DelaySlotAffectBranch(uint32_t PC);
		int32_t CompareInstructions(uint32_t PC, RSPOPCODE * Top, RSPOPCODE * Bottom);
		int32_t IsOpcodeBranch(uint32_t PC, RSPOPCODE RspOp);
		int32_t IsOpcodeNop(uint32_t PC);
		int32_t IsNextInstructionMmx(uint32_t PC);
		int32_t IsRegisterConstant (uint32_t Reg, uint32_t * Constant);
		void RSPCALL RSP_Element2Mmx(int32_t MmxReg);
		void RSPCALL RSP_MultiElement2Mmx(int32_t MmxReg1, int32_t MmxReg2);

		uint32_t RunRecompilerCPU ( uint32_t Cycles );
		void BuildRecompilerCPU ( void );
		void CompilerRSPBlock ( void );
		void CompilerToggleBuffer (void);
		int32_t RSP_DoSections(void);

		/************************* OpCode functions *************************/
		void RSPCALL RSP_Opcode_SPECIAL ( void );
		void RSPCALL RSP_Opcode_REGIMM  ( void );
		void RSPCALL RSP_Opcode_J	   ( void );
		void RSPCALL RSP_Opcode_JAL	 ( void );
		void RSPCALL RSP_Opcode_BEQ	 ( void );
		void RSPCALL RSP_Opcode_BNE	 ( void );
		void RSPCALL RSP_Opcode_BLEZ	( void );
		void RSPCALL RSP_Opcode_BGTZ	( void );
		void RSPCALL RSP_Opcode_ADDI	( void );
		void RSPCALL RSP_Opcode_ADDIU   ( void );
		void RSPCALL RSP_Opcode_SLTI	( void );
		void RSPCALL RSP_Opcode_SLTIU   ( void );
		void RSPCALL RSP_Opcode_ANDI	( void );
		void RSPCALL RSP_Opcode_ORI	 ( void );
		void RSPCALL RSP_Opcode_XORI	( void );
		void RSPCALL RSP_Opcode_LUI	 ( void );
		void RSPCALL RSP_Opcode_COP0	( void );
		void RSPCALL RSP_Opcode_COP2	( void );
		void RSPCALL RSP_Opcode_LB	  ( void );
		void RSPCALL RSP_Opcode_LH	  ( void );
		void RSPCALL RSP_Opcode_LW	  ( void );
		void RSPCALL RSP_Opcode_LBU	 ( void );
		void RSPCALL RSP_Opcode_LHU	 ( void );
		void RSPCALL RSP_Opcode_SB	  ( void );
		void RSPCALL RSP_Opcode_SH	  ( void );
		void RSPCALL RSP_Opcode_SW	  ( void );
		void RSPCALL RSP_Opcode_LC2	 ( void );
		void RSPCALL RSP_Opcode_SC2	 ( void );
		/********************** R4300i OpCodes: Special **********************/
		void RSPCALL RSP_Special_SLL	( void );
		void RSPCALL RSP_Special_SRL	( void );
		void RSPCALL RSP_Special_SRA	( void );
		void RSPCALL RSP_Special_SLLV   ( void );
		void RSPCALL RSP_Special_SRLV   ( void );
		void RSPCALL RSP_Special_SRAV   ( void );
		void RSPCALL RSP_Special_JR	 ( void );
		void RSPCALL RSP_Special_JALR   ( void );
		void RSPCALL RSP_Special_BREAK  ( void );
		void RSPCALL RSP_Special_ADD	( void );
		void RSPCALL RSP_Special_ADDU   ( void );
		void RSPCALL RSP_Special_SUB	( void );
		void RSPCALL RSP_Special_SUBU   ( void );
		void RSPCALL RSP_Special_AND	( void );
		void RSPCALL RSP_Special_OR	 ( void );
		void RSPCALL RSP_Special_XOR	( void );
		void RSPCALL RSP_Special_NOR	( void );
		void RSPCALL RSP_Special_SLT	( void );
		void RSPCALL RSP_Special_SLTU   ( void );
		/********************** R4300i OpCodes: RegImm **********************/
		void RSPCALL RSP_Opcode_BLTZ	( void );
		void RSPCALL RSP_Opcode_BGEZ	( void );
		void RSPCALL RSP_Opcode_BLTZAL  ( void );
		void RSPCALL RSP_Opcode_BGEZAL  ( void );
		/************************** Cop0 functions *************************/
		void RSPCALL RSP_Cop0_MF		( void );
		void RSPCALL RSP_Cop0_MT		( void );
		/************************** Cop2 functions *************************/
		void RSPCALL RSP_Cop2_MF		( void );
		void RSPCALL RSP_Cop2_CF		( void );
		void RSPCALL RSP_Cop2_MT		( void );
		void RSPCALL RSP_Cop2_CT		( void );
		void RSPCALL RSP_COP2_VECTOR	( void );
		/************************** Vect functions **************************/
		void RSPCALL RSP_Vector_VMULF   ( void );
		void RSPCALL RSP_Vector_VMULU	( void );
		void RSPCALL RSP_Vector_VMUDL   ( void );
		void RSPCALL RSP_Vector_VMUDM   ( void );
		void RSPCALL RSP_Vector_VMUDN   ( void );
		void RSPCALL RSP_Vector_VMUDH   ( void );
		void RSPCALL RSP_Vector_VMACF   ( void );
		void RSPCALL RSP_Vector_VMACU   ( void );
		void RSPCALL RSP_Vector_VMACQ   ( void );
		void RSPCALL RSP_Vector_VMADL   ( void );
		void RSPCALL RSP_Vector_VMADM   ( void );
		void RSPCALL RSP_Vector_VMADN   ( void );
		void RSPCALL RSP_Vector_VMADH   ( void );
		void RSPCALL RSP_Vector_VADD	( void );
		void RSPCALL RSP_Vector_VSUB	( void );
		void RSPCALL RSP_Vector_VABS	( void );
		void RSPCALL RSP_Vector_VADDC   ( void );
		void RSPCALL RSP_Vector_VSUBC   ( void );
		void RSPCALL RSP_Vector_VSAW	( void );
		void RSPCALL RSP_Vector_VLT	 ( void );
		void RSPCALL RSP_Vector_VEQ	 ( void );
		void RSPCALL RSP_Vector_VNE	 ( void );
		void RSPCALL RSP_Vector_VGE	 ( void );
		void RSPCALL RSP_Vector_VCL	 ( void );
		void RSPCALL RSP_Vector_VCH	 ( void );
		void RSPCALL RSP_Vector_VCR	 ( void );
		void RSPCALL RSP_Vector_VMRG	( void );
		void RSPCALL RSP_Vector_VAND	( void );
		void RSPCALL RSP_Vector_VNAND   ( void );
		void RSPCALL RSP_Vector_VOR	 ( void );
		void RSPCALL RSP_Vector_VNOR	( void );
		void RSPCALL RSP_Vector_VXOR	( void );
		void RSPCALL RSP_Vector_VNXOR   ( void );
		void RSPCALL RSP_Vector_VRCP	( void );
		void RSPCALL RSP_Vector_VRCPL   ( void );
		void RSPCALL RSP_Vector_VRCPH   ( void );
		void RSPCALL RSP_Vector_VMOV	( void );
		void RSPCALL RSP_Vector_VRSQ	( void );
		void RSPCALL RSP_Vector_VRSQL   ( void );
		void RSPCALL RSP_Vector_VRSQH   ( void );
		void RSPCALL RSP_Vector_VNOOP   ( void );
		/************************** lc2 functions **************************/
		void RSPCALL RSP_Opcode_LBV	 ( void );
		void RSPCALL RSP_Opcode_LSV	 ( void );
		void RSPCALL RSP_Opcode_LLV	 ( void );
		void RSPCALL RSP_Opcode_LDV	 ( void );
		void RSPCALL RSP_Opcode_LQV	 ( void );
		void RSPCALL RSP_Opcode_LRV	 ( void );
		void RSPCALL RSP_Opcode_LPV	 ( void );
		void RSPCALL RSP_Opcode_LUV	 ( void );
		void RSPCALL RSP_Opcode_LHV	 ( void );
		void RSPCALL RSP_Opcode_LFV	 ( void );
		void RSPCALL RSP_Opcode_LTV	 ( void );
		/************************** lc2 functions **************************/
		void RSPCALL RSP_Opcode_SBV	 ( void );
		void RSPCALL RSP_Opcode_SSV	 ( void );
		void RSPCALL RSP_Opcode_SLV	 ( void );
		void RSPCALL RSP_Opcode_SDV	 ( void );
		void RSPCALL RSP_Opcode_SQV	 ( void );
		void RSPCALL RSP_Opcode_SRV	 ( void );
		void RSPCALL RSP_Opcode_SPV	 ( void );
		void RSPCALL RSP_Opcode_SUV	 ( void );
		void RSPCALL RSP_Opcode_SHV	 ( void );
		void RSPCALL RSP_Opcode_SFV	 ( void );
		void RSPCALL RSP_Opcode_STV	 ( void );
		void RSPCALL RSP_Opcode_SWV	 ( void );
		/************************** Other functions **************************/
		void RSPCALL rsp_UnknownOpcode  ( void );

		void RSPBranch_AddRef(uint32_t Target, uint32_t * X86Loc);
		void RSPCheat_r4300iOpcode ( void (RSPCALL Rsp:: *FunctAddress)()) ;
		void RSPCheat_r4300iOpcodeNoMessage( void (RSPCALL Rsp:: *FunctAddress)());

		uint32_t WriteToAccum2 (int32_t Location, int32_t PC, int32_t RecursiveCall);
		int32_t WriteToVectorDest2 (uint32_t DestReg, int32_t PC, int32_t RecursiveCall);
		void GetInstructionInfo(uint32_t PC, RSPOPCODE * RspOp, OPCODE_INFO * info);
		void DetectGPRConstants(RSP_CODE * code);
		void ClearAllx86Code (void);
		void LinkBranches(RSP_BLOCK * Block);


		int32_t RSPCompile_Vector_VMULF_MMX ( void );
		int32_t RSPCompile_Vector_VMUDL_MMX ( void );
		int32_t RSPCompile_Vector_VMUDM_MMX ( void );
		int32_t RSPCompile_Vector_VMUDN_MMX ( void );
		int32_t RSPCompile_Vector_VMUDH_MMX ( void );
		int32_t RSPCompile_Vector_VADD_MMX ( void );
		int32_t RSPCompile_Vector_VGE_MMX(void);
		int32_t RSPCompile_Vector_VAND_MMX ( void );
		int32_t RSPCompile_Vector_VOR_MMX ( void );
		int32_t RSPCompile_Vector_VXOR_MMX ( void );

		void ReOrderInstructions(uint32_t StartPC, uint32_t EndPC);
		void ReOrderSubBlock(RSP_BLOCK * Block);
		void BuildBranchLabels(void);
		void CompilerLinkBlocks(void);
		int32_t IsJumpLabel(uint32_t PC);

};


#endif


#endif
