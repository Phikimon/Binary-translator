#include "Binary_Translator.h"
#include "AvrInstructions.h"

//-----------------------------------------------------------
#define CHECK_EQ(value, cut_value)                          \
do {                                                        \
    if ((value) != (cut_value))                             \
    {                                                       \
        printf("Too big argument %x(%s) in %s",             \
                value, #value, __func__);                   \
        exit(BINTRANERR_TOO_BIG_ARGUMENT_VALUE);            \
    }                                                       \
} while (0)
//-----------------------------------------------------------
#define REG_IOREG_INSTRUCTION(NAME)                         \
AvrCmd avrOpcode##NAME(uint8_t reg, uint8_t ioReg)          \
{                                                           \
    reg += 0x10;                                            \
    CHECK_EQ(reg,   reg   &    REGISTER_MASK);              \
    CHECK_EQ(ioReg, ioReg & IO_REGISTER_MASK);              \
    /*xxxx xAAd dddd AAAA*/                                 \
    return ( AVR_##NAME##_OPCODE  |                         \
             (reg << 4)           |                         \
             (ioReg & 0x0F)       |                         \
            ((ioReg & 0x30) << 5) );                        \
}

REG_IOREG_INSTRUCTION(IN)
REG_IOREG_INSTRUCTION(OUT)

#undef REG_IOREG_INSTRUCTION
//-----------------------------------------------------------
#define IOREG_BITNUM_INSTRUCTION(NAME)                      \
AvrCmd avrOpcode##NAME(uint8_t ioReg, uint8_t bitNum)       \
{                                                           \
    CHECK_EQ(ioReg,  ioReg  & LOWER_IO_REGISTER_MASK);      \
    CHECK_EQ(bitNum, bitNum & BITNUM_MASK);                 \
    /* xxxx xxxx AAAA Abbb */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            (ioReg << 3)         |                          \
            (bitNum)             );                         \
}

IOREG_BITNUM_INSTRUCTION(CBI)
IOREG_BITNUM_INSTRUCTION(SBIS)

#undef IOREG_BITNUM_INSTRUCTION
//-----------------------------------------------------------
#define REG_REG_INSTRUCTION(NAME)                           \
AvrCmd avrOpcode##NAME(uint8_t reg1, uint8_t reg2)          \
{                                                           \
    reg1 += 0x10;                                           \
    reg2 += 0x10;                                           \
    CHECK_EQ(reg1, reg1 & REGISTER_MASK);                   \
    CHECK_EQ(reg2, reg2 & REGISTER_MASK);                   \
    /* xxxx xxrd dddd rrrr */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            ( reg2 & 0x0F)       |                          \
            ((reg2 & 0x10) << 5) |                          \
             (reg1 << 4)         );                         \
}

REG_REG_INSTRUCTION(AND)
REG_REG_INSTRUCTION(ADD)
REG_REG_INSTRUCTION(SUB)
REG_REG_INSTRUCTION(MUL)
REG_REG_INSTRUCTION(CP )
REG_REG_INSTRUCTION(MOV)

AvrCmd avrOpcodeMOVCorrectSourceValue(uint8_t reg1, uint8_t reg2)
{
    reg1 += 0x10;
    CHECK_EQ(reg1, reg1 & REGISTER_MASK);
    CHECK_EQ(reg2, reg2 & REGISTER_MASK);
    /* xxxx xxrd dddd rrrr */
    return ( AVR_MOV_OPCODE |
            ( reg2 & 0x0F)       |
            ((reg2 & 0x10) << 5) |
             (reg1 << 4)         );
}

#undef REG_REG_INSTRUCTION
//-----------------------------------------------------------
#define REG_INSTRUCTION(NAME)                               \
AvrCmd avrOpcode##NAME(uint8_t reg)                         \
{                                                           \
    reg += 0x10;                                            \
    CHECK_EQ(reg, reg & REGISTER_MASK);                     \
    /* xxxx xxxd dddd 1111 */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            (reg << 4)           );                         \
}

REG_INSTRUCTION(PUSH)
REG_INSTRUCTION(POP)

#undef REG_INSTRUCTION
//-----------------------------------------------------------
#define ADDR_INSTRUCTION(NAME)                              \
AvrCmd avrOpcode##NAME(uint8_t relAddr)                     \
{                                                           \
    CHECK_EQ(relAddr, relAddr & ADDRESS_MASK);              \
    /* xxxx xxkk kkkk kxxx */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            (relAddr << 3)       );                         \
}

ADDR_INSTRUCTION(BREQ)
ADDR_INSTRUCTION(BRNE)
ADDR_INSTRUCTION(BRGE)

#undef ADDR_INSTRUCTION
//-----------------------------------------------------------
#define LONG_ADDR_INSTRUCTION(NAME)                         \
AvrCmd avrOpcode##NAME(uint16_t longRelAddr)                \
{                                                           \
    CHECK_EQ(longRelAddr, longRelAddr & LONG_ADDRESS_MASK); \
    /* xxxx kkkk kkkk kkkk */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            (longRelAddr)        );                         \
}

LONG_ADDR_INSTRUCTION(RJMP)
LONG_ADDR_INSTRUCTION(RCALL)

#undef LONG_ADDR_INSTRUCTION
//-----------------------------------------------------------
#define REG_IMM_INSTRUCTION(NAME)                           \
AvrCmd avrOpcode##NAME(uint8_t reg, uint8_t imm)            \
{                                                           \
    CHECK_EQ(reg, reg & LOWER_REGISTER_MASK);               \
    CHECK_EQ(imm, imm & IMMEDIATE_MASK);                    \
    /* xxxx KKKK dddd KKKK */                               \
    return ( AVR_##NAME##_OPCODE |                          \
            (reg << 4)           |                          \
            (imm & 0x0F)         |                          \
           ((imm & 0xF0) << 4)   );                         \
}

REG_IMM_INSTRUCTION(CPI)
REG_IMM_INSTRUCTION(LDI)

#undef REG_IMM_INSTRUCTION
//-----------------------------------------------------------
#define NO_ARG_INSTRUCTION(NAME)                            \
AvrCmd avrOpcode##NAME(void)                                \
{                                                           \
    return AVR_##NAME##_OPCODE;                             \
}

NO_ARG_INSTRUCTION(RET)
NO_ARG_INSTRUCTION(RETI)
NO_ARG_INSTRUCTION(NOP)

#undef NO_ARG_INSTRUCTION
//-----------------------------------------------------------
AvrCmd avrOpcodeTST(uint8_t reg)
{
    reg += 0x10;
    // 0010 00rd dddd rrrr
    CHECK_EQ(reg, reg & REGISTER_MASK);

    return avrOpcodeAND(reg, reg);
}

#undef CHECK_EQ
