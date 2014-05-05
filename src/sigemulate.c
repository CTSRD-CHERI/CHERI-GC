#include "gc_config.h"
#ifdef GC_EPHEMERAL_TRAP_SUPPORT

#include <sys/types.h>
#include <machine/cheri.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "ephemeral.h"

#ifdef NDEBUG
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...) 
#endif

/**
 * Opcodes for MIPS branch instructions.
 */
enum branch_ops
{
	// The real opcode is stored in the second register field
	MIPS_BRANCH_REGIMM= 0x1,
	// The real opcode is stored in the first register field
	MIPS_BRANCH_CHERI = 0x12,

	MIPS_BRANCH_J     = 0x2,
	MIPS_BRANCH_JAL   = 0x3,

	MIPS_BRANCH_JR    = 0x8,
	MIPS_BRANCH_JALR  = 0x9,
	MIPS_BRANCH_BEQ   = 0x4,
	MIPS_BRANCH_BNE   = 0x5,
	MIPS_BRANCH_BLEZ  = 0x6,
	MIPS_BRANCH_BGTZ  = 0x7,
	MIPS_BRANCH_BEQL  = 0x14,
	MIPS_BRANCH_BNEL  = 0x15,
	MIPS_BRANCH_BLEZL = 0x16,
	MIPS_BRANCH_BGTZL = 0x17
};
/**
 * For some branches, the opcode is REGIMM, but the real opcode is stored in
 * the second register operand slot (bits 20--16).
 */
enum regimm_branch_ops
{
	MIPS_BRANCH_BLTZ    = 0x0,
	MIPS_BRANCH_BGEZ    = 0x1,
	MIPS_BRANCH_BLTZAL  = 0x10,
	MIPS_BRANCH_BGEZAL  = 0x11,
	MIPS_BRANCH_BLTZL   = 0x2,
	MIPS_BRANCH_BGEZL   = 0x3,
	MIPS_BRANCH_BLTZALL = 0x12,
	MIPS_BRANCH_BGEZALL = 0x13
};

/**
 * For CHERI branch instructions, the opcode is CP2OP and the real opcode is
 * stored in the first register operand slot (bits 25--21).
 */
enum cheri_branch_ops
{
	CHERI_BRANCH_CBTU   = 0x09, 
	CHERI_BRANCH_CBTS   = 0x0a,
	CHERI_BRANCH_CJR    = 0x07,
	CHERI_BRANCH_CJALR  = 0x08
};


/**
 * Returns the CHERI signal frame information associated with a context.
 */
static inline struct cheri_frame *
getCHERIFrame(mcontext_t *context)
{
	assert(context->mc_cp2state_len == sizeof(struct cheri_frame));
	return ((struct cheri_frame *)context->mc_cp2state);
}

/**
 * Sign extend a value to the size of a `register_t`, assuming that it is a
 * signed value of the low `size` bits of the `val` argument.
 */
static inline register_t
signExtend(register_t val, int size)
{
	int shift = ((sizeof(register_t)*8) - size);
	int64_t ext = val << shift;

	return (ext >> shift);
}


/**
 * Reads an immediate value from an instruction.
 */
static inline register_t
getImm(uint32_t instr, int start, int len)
{
	uint32_t mask = (0xffffffff >> (32-len)) << (start - len + 1);

	return (instr & mask) >> (start - len + 1);
}

/**
 * Loads a capability register from the context.
 */
static inline __capability void*
getCapRegAtIndex(mcontext_t *context, int idx)
{
	struct cheri_frame *frame = getCHERIFrame(context);

	_Static_assert(offsetof(struct cheri_frame, cf_c0) == 0,
			"Layout of struct cheri_frame has changed!");
	_Static_assert(offsetof(struct cheri_frame, cf_pcc) == 27*32,
			"Layout of struct cheri_frame has changed!");
	assert((idx < 26) && (idx >= 0) &&
	       "Invalid capability register index");
	return (((__capability void**)frame)[idx]);
}

static inline uint32_t *
getAdjustedPc(mcontext_t *context, register_t pc)
{
	uint64_t base = getCHERIFrame(context)->cf_pcc.c_base;
	assert((base + pc) > base);
	assert(pc < getCHERIFrame(context)->cf_pcc.c_length);
	base += (uint64_t)pc;
	return ((uint32_t*)base);
}

/**
 * Reads a register by loading the value from a register context based on an
 * opcode in an instruction.  The `opidx` parameter gives the bit index into
 * the instruction where the operand starts.
 */
static inline register_t
getReg(mcontext_t *context, uint32_t instr, int opidx)
{
	int regno = getImm(instr, opidx, 5);

	if (regno == 0)
	{
		return 0;
	}
	return (context->mc_regs[regno]);
}

/**
 * Reads a capability register by loading the value from a register context
 * based on an opcode in an instruction.  The `opidx` parameter gives the bit
 * index into the instruction where the operand starts.
 */
static inline __capability void*
getCapReg(mcontext_t *context, uint32_t instr, int opidx)
{
	int regno = getImm(instr, opidx, 5);

	return getCapRegAtIndex(context, regno);
}

/**
 * Reads a signed immediate value from an instruction.
 */
static inline register_t
getSImm(uint32_t instr, int start, int len)
{
	return signExtend(getImm(instr, start, len), len);
}

/**
 * Get the opcode field from an instruction (the first 6 bits)
 */
static inline register_t
getOpcode(uint32_t instr)
{

	return getImm(instr, 31, 6);
}

/**
 * Returns whether the current pc location is a branch delay slot.
 */
static inline bool
isInDelaySlot(mcontext_t *context)
{
	return getImm(context->cause, 31, 1);
#if 0
	// TODO: We should just get the magic number from the trap, but we can
	// cheat a bit by disassembling the previous instruction.
	if (context->mc_pc < 4)
	{
		return false;
	}
	uint32_t instr = *getAdjustedPc(context, context->mc_pc - 4);
	switch ((enum branch_ops)getOpcode(instr))
	{
		case MIPS_BRANCH_REGIMM:
		{
			switch ((enum regimm_branch_ops)getImm(instr, 20, 5))
			{
				case MIPS_BRANCH_BLTZL:
				case MIPS_BRANCH_BLTZALL:
				case MIPS_BRANCH_BLTZ:
				case MIPS_BRANCH_BLTZAL:
				case MIPS_BRANCH_BGEZL:
				case MIPS_BRANCH_BGEZ:
				case MIPS_BRANCH_BGEZAL:
				case MIPS_BRANCH_BGEZALL:
					return true;
			}
			return false;
		}
		case MIPS_BRANCH_CHERI:
		{
			switch ((enum cheri_branch_ops)getImm(instr, 25, 5))
			{
				case CHERI_BRANCH_CBTU:
				case CHERI_BRANCH_CBTS:
				case CHERI_BRANCH_CJR:
				case CHERI_BRANCH_CJALR:
					return true;
			}
			return false;
		}
		case MIPS_BRANCH_J:
		case MIPS_BRANCH_JAL:
		case MIPS_BRANCH_JR:
		case MIPS_BRANCH_JALR:
		case MIPS_BRANCH_BEQL:
		case MIPS_BRANCH_BEQ:
		case MIPS_BRANCH_BNEL:
		case MIPS_BRANCH_BNE:
		case MIPS_BRANCH_BLEZL:
		case MIPS_BRANCH_BLEZ:
		case MIPS_BRANCH_BGTZL:
		case MIPS_BRANCH_BGTZ:
			return true;
	}
	return false;
#endif
}

/**
 * If we are in a branch delay slot, work out what the next instruction will
 * be.  This is either the branch target or the instruction immediately
 * following the delay slot, depending on whether the branch should have been
 * taken.
 */
static bool 
emulateBranch(mcontext_t *context, register_t pc)
{
	uint32_t instr = *getAdjustedPc(context, pc);
	// If the instruction isn't a branch, the following two will be nonsense
	// values, but we'll only use them in cases where they make sense and
	// computing them has no side effects so it simplifies the code to compute
	// them here.
	int64_t offset = getSImm(instr, 15, 16) << 2;
	// The destination for the branch, if it's a PC-relative branch with
	// immediate offset.
	register_t branchPc = ((int64_t)pc) + offset + 4;
	// The instruction immediately following the delay slot.
	register_t normalPc = pc + 8;

	// Similarly, the next two may be meaningless values, but again we're just
	// loading data from a structure, so it's safe to do the work redundantly.
	// The first register operand, if this is a two-GPR-operand instruction
	int64_t regVal = getReg(context, instr, 25);
	// The second register operand, if this is a two-GPR-operand instruction
	int64_t regVal2 = getReg(context, instr, 20);

	switch ((enum branch_ops)getOpcode(instr))
	{
		case MIPS_BRANCH_REGIMM:
		{
			switch ((enum regimm_branch_ops)getImm(instr, 20, 5))
			{
				case MIPS_BRANCH_BLTZL:
				case MIPS_BRANCH_BLTZALL:
					assert((regVal < 0) &&
					       "In delay slot for not-taken likely branch!");
				case MIPS_BRANCH_BLTZ:
				case MIPS_BRANCH_BLTZAL:
					context->mc_pc = ((regVal < 0) ? branchPc : normalPc);
					return true;
				case MIPS_BRANCH_BGEZL:
				case MIPS_BRANCH_BGEZ:
					assert((regVal < 0) &&
					       "In delay slot for not-taken likely branch!");
				case MIPS_BRANCH_BGEZAL:
				case MIPS_BRANCH_BGEZALL:
					context->mc_pc = ((regVal >= 0) ? branchPc : normalPc);
					return true;
			}
			break;
		}
		case MIPS_BRANCH_J:
		case MIPS_BRANCH_JAL:
			context->mc_pc = (getImm(instr, 25, 0)<<2) & ((pc >> 28) << 28);
			return true;
		case MIPS_BRANCH_JR:
		case MIPS_BRANCH_JALR:
			context->mc_pc = getReg(context, instr, 25);
			return true;
		case MIPS_BRANCH_BEQL:
		case MIPS_BRANCH_BEQ:
			context->mc_pc = ((regVal == regVal2) ? branchPc : normalPc);
			return true;
		case MIPS_BRANCH_BNEL:
		case MIPS_BRANCH_BNE:
			context->mc_pc = ((regVal != regVal2) ? branchPc : normalPc);
			return true;
		case MIPS_BRANCH_BLEZL:
		case MIPS_BRANCH_BLEZ:
			context->mc_pc = ((regVal <= 0) ? branchPc : normalPc);
			return true;
		case MIPS_BRANCH_BGTZL:
		case MIPS_BRANCH_BGTZ:
			context->mc_pc = ((regVal > 0) ? branchPc : normalPc);
			return true;
		case MIPS_BRANCH_CHERI:
		{
			switch ((enum cheri_branch_ops)getImm(instr, 25, 5))
			{
				case CHERI_BRANCH_CBTU:
				{
					__capability void *cap = getCapReg(context, instr, 20);
					bool tag = __builtin_cheri_get_cap_tag(cap);
					context->mc_pc = (!tag ? branchPc : normalPc);
					return true;
				}
				case CHERI_BRANCH_CBTS:
				{
					__capability void *cap = getCapReg(context, instr, 20);
					bool tag = __builtin_cheri_get_cap_tag(cap);
					context->mc_pc = (tag ? branchPc : normalPc);
					return true;
				}
				case CHERI_BRANCH_CJR:
				case CHERI_BRANCH_CJALR:
				{
					context->mc_pc = getReg(context, instr, 10);
					// FIXME: This is very ugly, but to fix it we need to
					// define a new structure to replace cheri_frame.
					struct cheri_frame *frame = getCHERIFrame(context);
					// Note: The /32 is safe here because if this is not
					// aligned then the load will fail anyway...
					int regno = offsetof(struct cheri_frame, cf_pcc) / 32;
					(((__capability void**)frame)[regno]) =
						getCapReg(context, instr, 15);
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Helper function to construct a struct capstore.
 */
static inline struct capstore
make_capstore(__capability void *src, __capability void **dst)
{
	struct capstore s = {src, dst};

	return (s);
}

/**
 * Emulate the CHERI store instruction.  This works on csc instructions that
 * attempt to store an ephemeral capability through a capability that does not
 * havIFrame(context)-> permissions.
 */
static struct capstore
emulateCHERIStore(mcontext_t *context, register_t pc)
{
	uint16_t cause = (uint16_t)getCHERIFrame(context)->cf_capcause;
	register_t reg = cause & 0x1f;
	uint8_t exccode = (cause >> 8) & 0xff;
	uint32_t instr = *getAdjustedPc(context, pc);

	// We only care about capability stores that failed because of a lack
	// of permission to store ephemeral capabilities.
	if (exccode != 0x16)
	{
		return make_capstore(0, 0);
	}
	// If this is one of the reserved registers, give up
	if (reg > 26)
	{
		LOG("Reserved register: %ld\n", reg);
		return make_capstore(0, 0);
	}
	if (reg != getImm(instr, 20, 5))
	{
		LOG("Register from capcause: %ld.  From instruction: %ld.  pc: %lx\n",
			reg, getImm(instr, 20, 5), context->mc_pc);
	}
	// The register that cause the trap should be the same one as the one
	// in the instruction!
	assert((reg == getImm(instr, 20, 5)) &&
	       "Capability register mismatch!");
	// If we're failing because we don't have a capability store permission, we should have an extra 
	assert((getOpcode(instr) == 0x3e) && "Unexpected instruction");
	// At this point, we know that the tag on cap is valid, because if it
	// were not then the error would be a tag failure.
	__capability void *cb = getCapRegAtIndex(context, reg);
	__capability void *cs = getCapReg(context, instr, 25);
	int64_t offset = getReg(context, instr, 15);
	offset += getSImm(instr, 10, 11);
	// Negative offset is not allowed
	if (offset < 0)
	{
		return make_capstore(0, 0);
	}
	// Out of bounds error
	if (offset + sizeof(__capability void*) > __builtin_cheri_get_cap_length(cb))
	{
		return make_capstore(0, 0);
	}
	__capability void **realBase =
		(void*)(__builtin_cheri_get_cap_base(cb) + offset);
	*realBase = cs;
	return make_capstore(cs, realBase);
}

/**
 * The callback that will be used to handle the result of the emulated
 * instruction.
 */
static cheri_store_handler callback;
/**
 * The original action for SIGPROT, which will be used if we don't handle it.
 */
static struct sigaction oaction;

/**
 * The SIGPROT handler.  Attempts to emulate the CHERI store instruction and
 * invokes the callback if it can.  It then advances the pc, including
 * emulating branch instructions, if required.
 */
static void
capsighandler(int signo, siginfo_t *info, ucontext_t *uap)
{
	mcontext_t *context = &uap->uc_mcontext;
	bool isDelaySlot = isInDelaySlot(context);
	register_t pc = context->mc_pc;
	assert(signo == SIGPROT);

	struct capstore cs = emulateCHERIStore(context, isDelaySlot ? pc + 4 : pc);
	// If we were able to emulate the store, then track the remembered set
	if (cs.dst != 0)
	{
		callback(cs);
		// If we're in a delay slot, then emulate the branch.  Otherwise, just
		// skip to the next instruction.
		if (isDelaySlot)
		{
			assert((pc > 4) && "Invalid delay slot PC!");
			if (!emulateBranch(context, pc))
			{
#ifndef NDEBUG
				fprintf(stderr, "Failed to emulate branch instruction: 0x%x",
						*getAdjustedPc(context, pc));
				abort();
#endif
			}
		}
		else
		{
			context->mc_pc += 4;
		}
		return;
	}
	// Abort if we haven't handled this branch and there are no other handlers
	// registered before us.
	if (oaction.sa_handler == 0)
	{
		abort();
	}
	// If we've failed to handle this, fall back to the original implementation
	if ((oaction.sa_flags & SA_SIGINFO) == SA_SIGINFO)
	{
		oaction.sa_sigaction(signo, info, uap);
	}
	else
	{
		oaction.sa_handler(signo);
	}
}

int setupEphemeralEmulation(cheri_store_handler handler)
{
	struct  sigaction action;

	action.sa_sigaction =
		(void (*)(int, struct __siginfo *, void *))capsighandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGPROT, &action, &oaction))
	{
		return errno;
	}
	callback = handler;
	return 0;
}

#endif // GC_EPHEMERAL_TRAP_SUPPORT
