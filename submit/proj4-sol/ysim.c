

#include "ysim.h"
#include <stdio.h>
#include "errors.h"

typedef enum {
  HALT_CODE, NOP_CODE, CMOVxx_CODE, IRMOVQ_CODE, RMMOVQ_CODE, MRMOVQ_CODE,
  OP1_CODE, Jxx_CODE, CALL_CODE, RET_CODE,
  PUSHQ_CODE, POPQ_CODE } BaseOpCode;

/************************** Utility Routines ****************************/

// Prints a Word bit by bit to the screen
void print_word(Word word)
{
  for(int index = 1; index <= sizeof(word)*8; index++)
  {
    printf("%u", (word >> index - 1) & 1lu);
  }
  printf("(%lu)\n", word);
}

/** Return nybble from op (pos 0: least-significant; pos 1:
 *  most-significant)
 */
static Byte
get_nybble(Byte op, int pos) {
  return (op >> (pos * 4)) & 0xF;
}


// Given an entire instruction, returns that instruction's argument
// i.e. the jump address for jump
// i.e. the return adress for irmovq's immediate
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
  // Use our new method to get the instruction
  

  // Get this step's instuction, opcode, and increment program counter
  Address counter = read_pc_y86(y86);
  Byte instruction = read_memory_byte_y86(y86, counter);
  Byte opcode = get_nybble(instruction, 1);
  
  // Stop execution if there are problems
  if (read_status_y86(y86) != STATUS_AOK) return;
  
  //print_word(instruction);
  //print_word(opcode);
  //printf("opcode: 0x%X\n", opcode);
 
  /*
   * Is there a situation? If so, determine which situation.
   * If there is a situation, handle the situation
   * in a manner apropriate for that situation.
   * If there was not a situation, move on to the next situation.
   */
  Word addr = 0, data = 0, argument = 0;
  Address dest = 0;
  Register a = 0, b = 0;
  switch(opcode)
  {
    case HALT_CODE:
      //printf("halt\n");
      write_status_y86(y86, STATUS_HLT);
      return;
    case NOP_CODE:
      //printf("nop\n");
      write_pc_y86(y86, counter+sizeof(Byte));
      // Do nothing
      break;
    case CALL_CODE:	// dynamic instr fix+
      //printf("call\n");
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      	// Write return address to stack
      write_memory_word_y86(y86, addr-sizeof(Word), counter + sizeof(Byte) + sizeof(Word));
      write_register_y86(y86, REG_RSP, (Word)addr-sizeof(Word));     // decrement stack pointer
      dest = read_memory_word_y86(y86, counter + 1);	  // Callee is in word after opcode byte instr
      //printf("jumping to %x from %x\n", dest, counter);
      write_pc_y86(y86, dest);                            // Set Program Counter to destination (jump)
      return;
    case RET_CODE:
      addr = read_register_y86(y86, REG_RSP);             // Get Stack Pointer
      write_register_y86(y86, REG_RSP, (Word)addr+sizeof(Word));     // Increment stack pointer
      dest = read_memory_word_y86(y86, addr);           // Read return address from stack
      //printf("ret to %x\n", dest);
      write_pc_y86(y86, dest);                            // Set Program Counter to returning destination (jump)
      break;
    case POPQ_CODE:
      a = get_nybble(read_memory_byte_y86(y86, counter+1), 1);   // Get Dest Reg (next byte instr)
      addr = read_register_y86(y86, REG_RSP);                    // Get Stack Pointer
      data = read_memory_word_y86(y86, addr+sizeof(Word));       // Read data from stack
      write_register_y86(y86, REG_RSP, (Word)addr+sizeof(Word)); // increment stack pointer
      write_register_y86(y86, a, data);                          // Write data to dest reg
      write_pc_y86(y86, counter+(2*sizeof(Byte)));
      break;
    case PUSHQ_CODE:
      a = get_nybble(read_memory_byte_y86(y86, counter+1), 1);    // Get Src Register (next byte)
      addr = read_register_y86(y86, REG_RSP);                     // Get Stack Pointer
      write_register_y86(y86, REG_RSP, (Word)addr-sizeof(Word));  // decrement stack pointer
      data = read_register_y86(y86, a);                     // Read data from src reg
      write_memory_word_y86(y86, addr, data);        // Write data to stack
      write_pc_y86(y86, counter+(2*sizeof(Byte)));
      break;
    case OP1_CODE:
      op1(y86, instruction,
        get_nybble(instruction, 3),
        get_nybble(instruction, 4));
      write_pc_y86(y86, counter+(2*sizeof(Byte)));
      break;
    // ECCO! BEHOLD! LOOK NO FURTHER! MOV INSTRUCTIONS GO.. YES, in THIS very spot.............!
    case CMOVxx_CODE:
      // RRMOVQ
      if (get_nybble(opcode, 1) == 0)
      {
	      // register to register
        a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1); // low nibble in next byte
	      b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0); // high nibble in next byte
	      write_register_y86(y86, b, read_register_y86(y86, a));	// copy contents A to B
      }
      write_pc_y86(y86, counter + 2*sizeof(Byte));
      break;
    case IRMOVQ_CODE:
      //printf("irmovq\n");
      b = get_nybble(read_memory_byte_y86(y86, read_pc_y86(y86) + 1), 0);
      data = read_memory_word_y86(y86, read_pc_y86(y86) + 2);
      write_register_y86(y86, b, data);
      write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      break;
    case RMMOVQ_CODE:
      // register to memory
      // get registers
      a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1);
	    b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0);
	    // a contains integer value. b is pointer
	    write_memory_word_y86(y86, read_register_y86(y86, b), read_register_y86(y86, a)); // in memory location b, store a
	    write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      break;
    case MRMOVQ_CODE:
      // move from memory to register
      a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1); // a is a pointer to value
	    b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0); // will be moved into b
	    
	      // read the value from the pointer stored in RegA, and write it to RegB
	    write_register_y86(y86, b, read_memory_word_y86(y86, read_register_y86(y86, a)));
	    
	    write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      break;
    //END OF MOV INSTRUCTIONs........................................................
    default:
      // TODO Change CPU Status on Unrecognized Instruction
      write_pc_y86(y86, counter+1); // program counter next byte
      break;
  }
  
  // Change Status to OK, Increment Program Counter
  write_status_y86(y86, STATUS_AOK);
  //write_pc_y86(y86, counter+1);
  //printf("Complete Instruction opcode: 0x%X, counter was %d, now is %d\n", opcode, counter, counter+1);
}

