#include "stdint.h"

enum AvrInstructionOpcode
{
    AVR_IN_OPCODE    = 0xB000, // 1011 0AAd dddd AAAA
    AVR_OUT_OPCODE   = 0xB800, // 1011 1AAd dddd AAAA

    AVR_CBI_OPCODE   = 0x9800, // 1001 1000 AAAA Abbb
    AVR_SBIS_OPCODE  = 0x9B00, // 1001 1011 AAAA Abbb

    AVR_AND_OPCODE   = 0x2000, // 0010 00rd dddd rrrr
    AVR_ADD_OPCODE   = 0x0C00, // 0000 11rd dddd rrrr
    AVR_SUB_OPCODE   = 0x1800, // 0001 10rd dddd rrrr
    AVR_MUL_OPCODE   = 0x9C00, // 1001 11rd dddd rrrr
    AVR_CP_OPCODE    = 0x1400, // 0001 01rd dddd rrrr
    AVR_MOV_OPCODE   = 0x2C00, // 0010 11rd dddd rrrr

    AVR_PUSH_OPCODE  = 0x920F, // 1001 001d dddd 1111
    AVR_POP_OPCODE   = 0x900F, // 1001 000d dddd 1111

    AVR_BREQ_OPCODE  = 0xF001, // 1111 00kk kkkk k001
    AVR_BRNE_OPCODE  = 0xF401, // 1111 01kk kkkk k001
    AVR_BRGE_OPCODE  = 0xF404, // 1111 01kk kkkk k100

    AVR_RJMP_OPCODE  = 0xC000, // 1100 kkkk kkkk kkkk
    AVR_RCALL_OPCODE = 0xD000, // 1101 kkkk kkkk kkkk

    AVR_CPI_OPCODE   = 0x3000, // 0011 KKKK dddd KKKK
    AVR_LDI_OPCODE   = 0xE000, // 1110 KKKK dddd KKKK

    AVR_NOP_OPCODE   = 0x0000, // 0000 0000 0000 0000
    AVR_RET_OPCODE   = 0x9508, // 1001 0101 0000 1000
    AVR_RETI_OPCODE  = 0x9518, // 1001 0101 0001 1000
//  AVR__OPCODE = 0x;
};

enum { AVR_RJMPS_ARGUMENTS_MASK    = 0x0FFF };
enum { AVR_BRANCHES_ARGUMENTS_MASK = 0x03F8 };
enum { AVR_PUSH_ARGUMENTS_MASK     = 0x01F0 };
enum { AVR_POP_ARGUMENTS_MASK      = 0x01F0 };

AvrCmd avrOpcodeIN    (uint8_t reg, uint8_t ioReg);

AvrCmd avrOpcodeOUT   (uint8_t reg, uint8_t ioReg);

AvrCmd avrOpcodeCBI   (uint8_t ioReg, uint8_t bitNum);

AvrCmd avrOpcodeSBIS  (uint8_t ioReg, uint8_t bitNum);

AvrCmd avrOpcodeAND   (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodeADD   (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodeSUB   (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodeMUL   (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodeCP    (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodeMOV   (uint8_t reg1, uint8_t reg2);

AvrCmd avrOpcodePUSH  (uint8_t reg);

AvrCmd avrOpcodePOP   (uint8_t reg);

AvrCmd avrOpcodeBREQ  (uint8_t relAddr);

AvrCmd avrOpcodeBRNE  (uint8_t relAddr);

AvrCmd avrOpcodeBRGE  (uint8_t relAddr);

AvrCmd avrOpcodeRJMP  (uint16_t longRelAddr);

AvrCmd avrOpcodeRCALL (uint16_t longRelAddr);

AvrCmd avrOpcodeCPI   (uint8_t reg, uint8_t imm);

AvrCmd avrOpcodeLDI   (uint8_t reg, uint8_t imm);

AvrCmd avrOpcodeRET   (void);

AvrCmd avrOpcodeRETI  (void);

AvrCmd avrOpcodeNOP   (void);

AvrCmd avrOpcodeTST   (uint8_t reg);

//Template's broken. In every instruction 0x10 is added to register number,
//because of LDI and CPI, which can't be used for lower 16 registers.
//And when you multiply two registers' values with mul, result is stored
//in R0:R1 registers, which can't be accessed, when 0x10 is added, so
//this functions uses MOV without adding anything to second argument.
AvrCmd avrOpcodeMOVCorrectSourceValue (uint8_t reg1, uint8_t reg2);
