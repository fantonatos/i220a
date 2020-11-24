

#include "ysim.h"
#include <stdio.h>
#include "errors.h"

typedef enum {
  HALT_CODE, NOP_CODE, CMOVxx_CODE, IRMOVQ_CODE, RMMOVQ_CODE, MRMOVQ_CODE,
  OP1_CODE, Jxx_CODE, CALL_CODE, RET_CODE,
  PUSHQ_CODE, POPQ_CODE } BaseOpCode;

/************************** Utility Routines ****************************/

/** Return nybble from op (pos 0: least-significant; pos 1:
 *  most-significant)
 */
static Byte
get_nybble(Byte op, int pos) {
  return (op >> (pos * 4)) & 0xF;
}


static Word
get_argument(Word instruction)
{
  Byte opcode = get_nybble(instruction, 1), offset = 0;
  
  switch(opcode)
  {
    case CALL_CODE:
    case Jxx_CODE:
      offset = 1;
      break;
    case IRMOVQ_CODE:
    case RMMOVQ_CODE:
    case MRMOVQ_CODE:
      offset = 2;
      break;
  }
  
  Word argument = 0x00000000;
  for (int byte = offset + 4; byte > offset; byte--)
  {
    argument |= get_nybble(instruction, byte) << byte * 4;
  }
  
  return argument;
}
/************************** Condition Codes ****************************/

/** Conditions used in instructions */
typedef enum {
  ALWAYS_COND, LE_COND, LT_COND, EQ_COND, NE_COND, GE_COND, GT_COND
} Condition;

/** accessing condition code flags */
static inline bool get_cc_flag(Byte cc, unsigned flagBitIndex) {
  return !!(cc & (1 << flagBitIndex));
}
static inline bool get_zf(Byte cc) { return get_cc_flag(cc, ZF_CC); }
static inline bool get_sf(Byte cc) { return get_cc_flag(cc, SF_CC); }
static inline bool get_of(Byte cc) { return get_cc_flag(cc, OF_CC); }

/** Return true iff the condition specified in the least-significant
 *  nybble of op holds in y86.  Encoding of Figure 3.15 of Bryant's
 *  CompSys3e.
 */
bool
check_cc(const Y86 *y86, Byte op)
{
  bool ret = false;
  Condition condition = get_nybble(op, 0);
  Byte cc = read_cc_y86(y86);
  switch (condition) {
    case ALWAYS_COND:
      ret = true;
      break;
    case LE_COND:
      ret = (get_sf(cc) ^ get_of(cc)) | get_zf(cc);
      break;
    //@TODO add other cases
    default: {
      Address pc = read_pc_y86(y86);
      fatal("%08lx: bad condition code %d\n", pc, condition);
      break;
      }
  }
  return ret;
}

/** return true iff word has its sign bit set */
static inline bool
isLt0(Word word) {
  return (word & (1UL << (sizeof(Word)*CHAR_BIT - 1))) != 0;
}

/** Set condition codes for addition operation with operands opA, opB
 *  and result with result == opA + opB.
 */
static void
set_add_arith_cc(Y86 *y86, Word opA, Word opB, Word result)
{
  // Set Zero, Sign, and Overflow flags
  if (result == 0) write_cc_y86(y86, ZF_CC);
  if (((int)result) < 0) write_cc_y86(y86, SF_CC);
  
  
  if ( ((int)opA > 0 && (int)opB > 0 && (int)result < 0) || ((int)opA < 0 && (int)opB < 0 && (int)result >= 0) ) write_cc_y86(y86, OF_CC);
  
}

/** Set condition codes for subtraction operation with operands opA, opB
 *  and result with result == opA - opB.
 */
static void
set_sub_arith_cc(Y86 *y86, Word opA, Word opB, Word result)
{
  if (result == 0) write_cc_y86(y86, ZF_CC);
  if (((int)result) < 0) write_cc_y86(y86, SF_CC);
  if ( ((int)opA > 0 && (int)opB > 0 && (int)result < 0) || ((int)opA < 0 && (int)opB < 0 && (int)result >= 0) ) write_cc_y86(y86, OF_CC);
}

static void
set_logic_op_cc(Y86 *y86, Word result)
{
  // Set Zero and Sign flags
  if (result == 0) write_cc_y86(y86, ZF_CC);
  if (result < 0) write_cc_y86(y86, SF_CC);
}

/**************************** Operations *******************************/
// Determine the math operation we are doing, and do it.
static void
op1(Y86 *y86, Byte op, Register regA, Register regB)
{
  enum {ADDL_FN, SUBL_FN, ANDL_FN, XORL_FN };
  Word result = 0, numA = 0, numB = 0;
  
  // Get our numbers from the registers
  numA = read_register_y86(y86, regA);
  numB = read_register_y86(y86, regB);
  
  
  // Determine which function (Add, subtract, AND, XOR)
  // Nibble 2 contains this
  Byte function = get_nybble(op, 2);
  switch(function)
  {
    case ADDL_FN:
      result = numA + numB;
      set_add_arith_cc(y86, numA, numB, result);
      break;
    case SUBL_FN:
      result = numA - numB;
      set_sub_arith_cc(y86, numA, numB, result);
      break;
    case ANDL_FN:
      result = numA & numB;
      break;
    case XORL_FN:
      result = numA ^ numB;
      break;
    default:
      // abbiamo un problema
      break;
  }
  
  // Save our result in the B register
  write_register_y86(y86, regB, result);
}

/*********************** Single Instruction Step ***********************/

/** Execute the next instruction of y86. Must change status of
 *  y86 to STATUS_HLT on halt, STATUS_ADR or STATUS_INS on
 *  bad address or instruction.
 */
void
step_ysim(Y86 *y86)
{
  //printf("start \n");
  
  // Get this step's instuction, opcode, and increment program counter
  Address counter = read_pc_y86(y86);
  Word instruction = read_memory_word_y86(y86, counter);
  Byte opcode = get_nybble((Byte)instruction, 1);
  
  
  //printf("opcode: 0x%X\n", opcode);
 
  /*
   * Is there a situation? If so, determine which situation.
   * If there is a situation, handle the situation
   * in a manner apropriate for that situation.
   * If there was not a situation, move on to the next situation.
   */
  Word addr = 0, data = 0;
  Address dest = 0;
  Register a = 0, b = 0;
  switch(opcode)
  {
    case HALT_CODE:
      write_status_y86(y86, STATUS_HLT);
      return;
    case NOP_CODE:
      // Do nothing
      break;
    case CALL_CODE:
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      write_memory_word_y86(y86, addr, (Word)counter+1);  // Write return address to stack
      write_register_y86(y86, REG_RSP, (Word)addr+1);     // Increment stack pointer
      dest = get_argument(instruction);
      write_pc_y86(y86, dest);                            // Set Program Counter to destination (jump)
      return;
    case RET_CODE:
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      write_register_y86(y86, REG_RSP, (Word)addr-1);     // Decrement stack pointer
      dest = read_memory_word_y86(y86, addr-1);           // Read return address from stack
      write_pc_y86(y86, dest);                            // Set Program Counter to returning destination (jump)
      break;
    case POPQ_CODE:
      a = get_nybble(instruction, 3);                     // Get Destination Register
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      data = read_memory_word_y86(y86, addr);             // Read data from stack
      write_register_y86(y86, REG_RSP, (Word)addr-1);     // Decrement stack pointer
      write_register_y86(y86, a, data);                   // Write data to destination register
      break;
    case PUSHQ_CODE:
      a = get_nybble(instruction, 3);                     // Get Source Register
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      write_register_y86(y86, REG_RSP, (Word)addr+1);     // Increment stack pointer
      data = read_register_y86(y86, REG_RSP);             // Read data from source register
      write_memory_word_y86(y86, addr+1, data);           // Write data to stack
      break;
    case OP1_CODE:
      op1(y86, instruction,
        get_nybble(instruction, 3),
        get_nybble(instruction, 4));
      break;
    default:
      // TODO Change CPU Status on Unrecognized Instruction
      break;
  }
  //printf("conclude \n");
  
  // Change Status to OK, Increment Program Counter
  write_status_y86(y86, STATUS_AOK);
  write_pc_y86(y86, counter+1);
  //printf("Complete Instruction opcode: 0x%X, counter was %d, now is %d\n", opcode, counter, counter+1);
}

