

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

/************************** Condition Codes ****************************/

/** Conditions used in instructions */
typedef enum {
  ALWAYS_COND, LE_COND, LT_COND, EQ_COND, NE_COND, GE_COND, GT_COND
} Condition;

/** Function to Construct a Flag Register Byte **/
static inline Byte
set_cc_flags(unsigned zf, unsigned sf, unsigned of)
{
  return ((zf<<ZF_CC) + (sf<<SF_CC) + (of<<OF_CC));
}

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
      //printf("always");
      break;
    case LE_COND:
      ret = (get_sf(cc) ^ get_of(cc)) | get_zf(cc);
      //printf("le");
      break;
    case LT_COND:
      ret = (get_sf(cc) ^ get_of(cc));
      //printf("lt");
      break;
    case EQ_COND:
      ret = get_zf(cc);
     // printf("eq");
      break;
    case NE_COND:
      ret = !(get_zf(cc));
     // printf("ne !(%d) = %d ", get_zf(cc), !(get_zf(cc)));
      break;
    case GE_COND:
      ret = !(get_sf(cc) ^ get_of(cc));
      //printf("ge");
      break;
    case GT_COND:
      ret = !(get_sf(cc) ^ get_of(cc)) & !get_zf(cc);
    //  printf("gt");
      break;
    default: {
      Address pc = read_pc_y86(y86);
      fatal("%08lx: bad condition code %d\n", pc, condition);
      break;
      }
  }

  //printf(" ret=%d\n", ret);
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
  signed long long R = (signed)result; // Let's be civilized, please.

  Byte flags = 0;

  if (R < 0) flags = set_cc_flags(0, 1, 0);
  if (R == 0) flags = set_cc_flags(1, get_sf(flags), 0);
  if ((opA>0 && opB>0 && result<0) || (opA<0 && opB<0 && result>0)) 
    { flags = set_cc_flags(get_zf(flags), get_sf(flags), 1); }

  write_cc_y86(y86, flags);
}

/** Set condition codes for subtraction operation with operands opA, opB
 *  and result with result == opA - opB.
 */
static void
set_sub_arith_cc(Y86 *y86, Word opA, Word opB, Word result)
{
  signed long long R = (signed)result;
  
  Byte flags = 0;

  if (opA > opB || R < 0) flags = set_cc_flags(0, 1, 0);
  if (result == 0) flags = set_cc_flags(1, get_sf(flags), 0);
  if ((opB>0 && opA<0 && result>0) || (opB<0 && opA>0 && result<0))
    { flags = set_cc_flags(get_zf(flags), get_sf(flags), 1); }

  write_cc_y86(y86, flags);
}

static void
set_logic_op_cc(Y86 *y86, Word result)
{
  signed long long R = (signed)result;
  Byte flags = 0;

  if (R < 0) flags = set_cc_flags(0, 1, 0);
  if (result == 0) flags = set_cc_flags(1, get_sf(flags), 0);
  flags = set_cc_flags(get_zf(flags), get_sf(flags), 0);

  write_cc_y86(y86, flags);
}

/********************** Conditional Operations *************************/
// Conditional Branches, and Moves

static void jmp (Y86* y86, Byte op)
{
  Byte function = get_nybble (op, 0);
  
  Address dest = read_memory_word_y86(y86, read_pc_y86(y86)+1);
  
//  printf("Jump func=%d dest=0x%X\n", function, dest);
  
  if (check_cc(y86, op)) write_pc_y86(y86, dest);
  else write_pc_y86(y86, read_pc_y86(y86) + sizeof(Byte) + sizeof(Word));
}

static void cmov (Y86* y86, Byte op)
{
  Byte function = get_nybble (op, 0);
  Address pcounter = read_pc_y86 (y86);

  // Are we doing the move?
  if (!check_cc(y86, op)) return;

  // The move is confirmed. Do it.
  Byte
  a = get_nybble(read_memory_byte_y86(y86, pcounter+1), 1),
  b = get_nybble(read_memory_byte_y86(y86, pcounter+1), 0);

  write_register_y86(y86, b, read_register_y86(y86, a));
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
  Byte function = get_nybble(op, 0);
  switch(function)
  {
    case ADDL_FN:
      result = numB + numA;
      set_add_arith_cc(y86, numA, numB, result);
      //printf("Adding %d + %d = %d\n", numB, numA, result);
      break;
    case SUBL_FN:
      result = numB - numA;
      set_sub_arith_cc(y86, numA, numB, result);
      //printf("Subtracting %d - %d = %d\n", numB, numA, result);
      break;
    case ANDL_FN:
      result = numB & numA;
      set_logic_op_cc(y86, result);
      //printf("AND %d & %d = %d\n", numB, numA, result);
      break;
    case XORL_FN:
      result = numB ^ numA;
      set_logic_op_cc(y86, result);
      //printf("XOR %d ^ %d = %d\n", numB, numA, result);
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
  // Get this step's instuction, opcode, and increment program counter
  Address counter = read_pc_y86(y86);
  Byte instruction = read_memory_byte_y86(y86, counter);
  Byte opcode = get_nybble(instruction, 1);
  
  if (read_status_y86(y86) != STATUS_AOK) return;
  
  /*
   * Is there a situation? If so, determine which situation.
   * If there is a situation, handle the situation
   * in a manner apropriate for that situation.
   * If there was not a situation, move on to the next situation.
   */
  Word addr = 0, data = 0, argument = 0;
  Address dest = 0, rsp = read_register_y86(y86, REG_RSP);
  Register a = 0, b = 0;
  switch(opcode)
  {
    case HALT_CODE:
      write_status_y86(y86, STATUS_HLT);
      return;
    case NOP_CODE:
      write_pc_y86(y86, counter+sizeof(Byte));
      break;


/** Stack Modifing Instructions **/
    case CALL_CODE:
      addr = read_register_y86(y86, REG_RSP);
      write_memory_word_y86(y86, addr-sizeof(Word), counter + sizeof(Byte) + sizeof(Word));
      write_register_y86(y86, REG_RSP, (Word)addr-sizeof(Word));
      dest = read_memory_word_y86(y86, counter + 1);
      write_pc_y86(y86, dest); 
      return;
    case RET_CODE:
      addr = read_register_y86(y86, REG_RSP);
      write_register_y86(y86, REG_RSP, (Word)addr+sizeof(Word));
      dest = read_memory_word_y86(y86, addr);
      write_pc_y86(y86, dest);
      break;
    case POPQ_CODE:
      a = get_nybble(read_memory_byte_y86(y86, counter+1), 1);  // Get Dest Reg (next byte instr)
      addr = read_register_y86(y86, REG_RSP);                   // Get Stack Pointer
      data = read_memory_word_y86(y86, addr);                   // Read data from stack
      write_register_y86(y86, REG_RSP, addr+sizeof(Word));      // rsp++
      write_register_y86(y86, a, data);                         // Write data to dest reg
//      write_register_y86(y86, REG_RSP, addr+sizeof(Word));      // Increment stack pointer
      write_pc_y86(y86, counter+(2*sizeof(Byte)));
      break;
    case PUSHQ_CODE:
      a = get_nybble(read_memory_byte_y86(y86, counter+1), 1);  // Get Src Register (next byte)
      data = read_register_y86(y86, a);                         // Read data from src Reg
      addr = read_register_y86(y86, REG_RSP);                   // Get Stack Pointer
      write_register_y86(y86, REG_RSP, addr-sizeof(Word));      // decrement stack pointer
      write_memory_word_y86(y86, addr-sizeof(Word), data);      // Write data to stack
      write_pc_y86(y86, counter+(2*sizeof(Byte)));
      break;
      
/** Jump, OP1 (ALU) **/
      
    case Jxx_CODE:
      jmp(y86, instruction);    
      break;
      
    case OP1_CODE:
      // Get the next pcounter byte, contains A B registers
      a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1);
      b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0);
      op1(y86, instruction, a, b);  // Call math function
      write_pc_y86(y86, counter+(2*sizeof(Byte)));  // jump 2 bytes down program memory
      break;
    // ECCO! BEHOLD! LOOK NO FURTHER! MOV INSTRUCTIONS GO.. YES, in THIS very spot.............!
    case CMOVxx_CODE:
      cmov(y86, instruction);
      // RRMOVQ
      /*if (get_nybble(opcode, 1) == 0)
      {
	      // Register-to-Register
        a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1); // low nibble in next byte
	      b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0); // high nibble in next byte
	      write_register_y86(y86, b, read_register_y86(y86, a));	// copy contents A to B
        //printf("rrmovq %d to %d\n", a, b);
      }*/
      write_pc_y86(y86, counter + 2*sizeof(Byte));
      break;
    case IRMOVQ_CODE: // Immediate-to-Register
      b = get_nybble(read_memory_byte_y86(y86, read_pc_y86(y86) + 1), 0);
      data = read_memory_word_y86(y86, read_pc_y86(y86) + 2);
      write_register_y86(y86, b, data);
      write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      //printf("irmovq saved 0x%X in %d\n", data, b);
      break;
    case RMMOVQ_CODE: // Register-to-Memory
      a = get_nybble(read_memory_byte_y86(y86, counter + 1), 1);
	    b = get_nybble(read_memory_byte_y86(y86, counter + 1), 0);
	    write_memory_word_y86(y86, 
	        read_register_y86(y86, b), 
	        read_register_y86(y86, a));
	    write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      //printf("rmmovq \n");
      break;
    case MRMOVQ_CODE: // Memory-to-Register
      a = get_nybble(read_memory_byte_y86(y86, counter + 1), 0); // a is a pointer to value
	    b = get_nybble(read_memory_byte_y86(y86, counter + 1), 1); // will be moved into b
	    addr = read_register_y86(y86, a);
      data = read_memory_word_y86(y86, addr);
	      // read the value from the pointer stored in RegA, and write it to RegB
	    write_register_y86(y86, b, data);
	    
	    write_pc_y86(y86, counter + 2*sizeof(Byte) + sizeof(Word));
      break;
    
    default:
      // TODO Change CPU Status on Unrecognized Instruction
      write_pc_y86(y86, counter+1); // program counter next byte
      break;
  }

  // Status -> OK
  write_status_y86(y86, STATUS_AOK);
}

