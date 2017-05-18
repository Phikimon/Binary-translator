#include "Assembler/CommonAssembler.hpp"
#include "stdint.h"
#include "stdio.h"
#include "assert.h"

typedef uint16_t AvrCmd;
//Atmega16 specific
enum { ATMEGA16_PIND  = 0x10 };
enum { ATMEGA16_DDRD  = 0x11 };
enum { ATMEGA16_PORTD = 0x12 };

enum { ATMEGA16_PINC  = 0x13 };
enum { ATMEGA16_DDRC  = 0x14 };
enum { ATMEGA16_PORTC = 0x15 };

enum { ATMEGA16_PINA  = 0x19 };
enum { ATMEGA16_DDRA  = 0x1A };
enum { ATMEGA16_PORTA = 0x1B };
//

enum { MAX_RESULT_CMD_SIZE  = 32   };
enum { MAX_PROGRAM_SIZE     = 2048 };

enum AvrInstructionOpcode
{
    AVR_IN_OPCODE    = 0xB000, // 1011 0AAd dddd AAAA
    AVR_OUT_OPCODE   = 0xB800, // 1011 1AAr rrrr AAAA
    AVR_CBI_OPCODE   = 0x9800, // 1001 1000 AAAA Abbb
    AVR_SBIS_OPCODE  = 0x9B00, // 1001 1011 AAAA Abbb
    AVR_AND_OPCODE   = 0x2000, // 0010 00rd dddd rrrr
    AVR_PUSH_OPCODE  = 0x920F, // 1001 001d dddd 1111
    AVR_POP_OPCODE   = 0x900F, // 1001 000d dddd 1111
    AVR_BREQ_OPCODE  = 0xF001, // 1111 00kk kkkk k001
    AVR_BRNE_OPCODE  = 0xF401, // 1111 01kk kkkk k001
    AVR_BRGE_OPCODE  = 0xF404, // 1111 01kk kkkk k100
    AVR_RJMP_OPCODE  = 0xC000, // 1100 kkkk kkkk kkkk
    AVR_RET_OPCODE   = 0x9508, // 1001 0101 0000 1000
    AVR_ADD_OPCODE   = 0x0C00, // 0000 11rd dddd rrrr
    AVR_SUB_OPCODE   = 0x1800, // 0001 10rd dddd rrrr
    AVR_MUL_OPCODE   = 0x9C00, // 1001 11rd dddd rrrr
    AVR_CP_OPCODE    = 0x1400, // 0001 01rd dddd rrrr
    AVR_CPI_OPCODE   = 0x3000, // 0011 KKKK dddd KKKK
    AVR_MOV_OPCODE   = 0x2C00, // 0010 11rd dddd rrrr
    AVR_LDI_OPCODE   = 0xE000, // 1110 KKKK dddd KKKK
    AVR_RCALL_OPCODE = 0xD000, // 1101 kkkk kkkk kkkk
    AVR_NOP_OPCODE   = 0x0000, // 0000 0000 0000 0000
//  AVR__OPCODE = 0x;
};

enum { REGISTER_MASK          = 0x1F };
enum { LOWER_REGISTER_MASK    = 0x0F };
enum { ADDRESS_MASK           = 0x7F };
enum { IMMEDIATE_MASK         = 0xFF };
enum { IO_REGISTER_MASK       = 0xFF };
enum { LOWER_IO_REGISTER_MASK = 0x1F };

enum BinaryTranslatorError
{
    BINTRANERR_NO_ERROR = 0,
    BINTRANERR_INVALID_ARGUMENT_TYPE,
    BINTRANERR_TOO_BIG_ARGUMENT_VALUE,
    BINTRANERR_INVALID_INSTRUCTION
};

struct BinaryTranslator
{
    FILE*  file;
    AvrCmd program[MAX_PROGRAM_SIZE];
    size_t programSize;

    bool   pushMet;
    bool   isRegisterPushed;
    int    pushedValue;
};

void btCtor(struct BinaryTranslator* bt, FILE* file);
void btDtor(struct BinaryTranslator* bt);
int btTranslate(struct BinaryTranslator* bt);
int  btPhikiToAvrCmd(     AvrCmd* avrCmd,
                     enum AsmCmd cmd,
                     enum ArgAddrMode* argTypes,
                     uint64_t* args);

void debugPrintAvrCmd(AvrCmd* avrCmd, int cmdCount);
void printBuf(struct BinaryTranslator* bt);
