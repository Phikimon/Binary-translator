#include "Binary_Translator.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define CHECK_EQ(value, cut_value)                                   \
do {                                                                 \
    if (value != cut_value)                                          \
    {                                                                \
        fprintf(stderr, "\nValue in cmd %d '%s' size is too big\n",  \
                        cmd, #value);                                \
        exit(BINTRANERR_TOO_BIG_ARGUMENT_VALUE);                     \
    }                                                                \
} while (0)

#define CHECK_TYPE(argNum, type)                                     \
do {                                                                 \
    if (argTypes[argNum] != type)                                    \
    {                                                                \
        fprintf(stderr, "\nIn cmd %d argument number %d "            \
                          "has invalid type %d(not equal to %d)\n",  \
                           cmd, argNum, argTypes[argNum], type);     \
        exit(BINTRANERR_INVALID_ARGUMENT_TYPE);                      \
    }                                                                \
} while (0)

void btCtor(struct BinaryTranslator* bt, FILE* file)
{
    assert(file);
    bt->file = file;
    bt->pushMet = bt->isRegisterPushed = false;
    bt->programSize = bt->pushedValue = 0;
    memset(bt->program, 0xFF, MAX_PROGRAM_SIZE * sizeof(AvrCmd));
}

void btDtor(struct BinaryTranslator* bt)
{
    assert(bt);
    bt->file = NULL;
    bt->pushMet = bt->isRegisterPushed = false;
    bt->programSize = bt->pushedValue = 0;
}

//#define SUPER_DEBUG
int btTranslate(struct BinaryTranslator* bt)
{
    assert(bt);
    assert(bt->file);
    assert(getc(bt->file) == 0xFA); //Check CPU ID
    //LDI r16, 0
    bt->program[0] = AVR_LDI_OPCODE;
    //OUT DDRC, r16
    bt->program[1] = ( AVR_OUT_OPCODE                |
                      (ATMEGA16_DDRC & 0x0F)         |
                     ((ATMEGA16_DDRC & 0x30)  << 5)  |
                      (0x10 << 4)                    );
    //LDI r0, 0xFF
    bt->program[2] = ( AVR_LDI_OPCODE | 0xF | (0xF << 8) );
    //OUT DDRA, r16
    bt->program[3] = ( AVR_OUT_OPCODE                |
                      (ATMEGA16_DDRA & 0x0F)         |
                     ((ATMEGA16_DDRA & 0x30)  << 5)  |
                      (0x10 << 4)                    );
    //CBI DDRD, 7
    bt->program[4] = ( AVR_CBI_OPCODE       |
                      (ATMEGA16_DDRD << 3) |
                      (0x7)                );
    //LDI r0, 0
    bt->program[5] = AVR_LDI_OPCODE;
    bt->programSize = 6;
    char c = '\0';
    int IP = 0;
    do
    {
        c = getc(bt->file);
        IP++;
        if (c == EOF)
        {
            break;
        } else
        {
            ungetc(c, bt->file);
        }
        char         argAddrTypes = getc(bt->file);
        IP++;
        enum AsmCmd  cmd          = (enum AsmCmd)getc(bt->file);
        IP++;
        int          argQt        = getCmdArgQt(cmd);
        assert(argQt >= 0);
        uint64_t         args    [MAX_ARG_QT] = {};
        enum ArgAddrMode argTypes[MAX_ARG_QT] = {};

        for (int argNum = 0; argNum < argQt; argNum++)
        {
            double tempDouble = 0;
            int errcode = fread(&tempDouble, 1, sizeof(tempDouble), bt->file);
            IP += sizeof(tempDouble);
            args[argNum] = (uint64_t)tempDouble;
            assert(errcode);
            argTypes[argNum] = (enum ArgAddrMode)((argAddrTypes >> (argNum * 2)) & 0x3);
        };

        AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
        int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argTypes, args);
        memcpy(bt->program + bt->programSize, avrCmd, cmdCount * sizeof(AvrCmd));
#ifdef SUPER_DEBUG
        printf("\n__________________________________________________________________");
        printf("\nDEBUG: IP = %d, cmd = %d(%s); \n\targs = { %zu, %zu, %zu}; \n\targTypes = { %d, %d, %d}"
                "\n\tProgramSize = %d; cmdCount = %d\n",
                IP, cmd, getCmdName(cmd), args[0], args[1], args[2], argTypes[0], argTypes[1], argTypes[2], bt->programSize, cmdCount);
        debugPrintAvrCmd(bt->program + bt->programSize, cmdCount);
        printf("\n__________________________________________________________________");
#endif

        bt->programSize += cmdCount;
    } while (true);
#ifdef SUPER_DEBUG
    printf("\nDEBUG: programSize = %zu\n", bt->programSize);
    debugPrintAvrCmd(bt->program, bt->programSize);
#endif
}

void printBuf(struct BinaryTranslator* bt)
{
    fwrite(bt->program,
           sizeof(char),
           bt->programSize * sizeof(AvrCmd),
           stdout);
}

void debugPrintAvrCmd(AvrCmd* avrCmd, int cmdCount)
{
    //changeEndianness(avrCmd, sizeof(AvrCmd), cmdCount);
    for (int i = 0; i < cmdCount * sizeof(AvrCmd) * 8; i++)
    {

        if ((i % 4 == 0) && (i != 0))
        {
            printf(" ");
        }
        if ((i % 16 == 0) && (i != 0))
        {
            if (i > 15)
                printf("| %04x", avrCmd[(i - 1) / (sizeof(AvrCmd) * 8)]);
            printf("\n");
        }
        printf("%d", (avrCmd[i / (sizeof(AvrCmd) * 8)] >> ((sizeof(AvrCmd) * 8 - 1) - (i % (sizeof(AvrCmd) * 8)))) & 1 );

    }
    printf(" | %04x", avrCmd[cmdCount - 1]);
    printf("\n");
}

int btPhikiToAvrCmd(     AvrCmd* avrCmd,
                    enum AsmCmd cmd,
                    enum ArgAddrMode* argTypes,
                    uint64_t* args)
{
    assert(avrCmd); assert(argTypes); assert(args);
    int  cmdCount = 0;
    switch (cmd)
    {
        case CPU_POP:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg = args[0] & REGISTER_MASK;
            CHECK_EQ(reg, args[0]);
            avrCmd[cmdCount++] = ( AVR_POP_OPCODE  | (reg << 4) );
        }; break;

        case CPU_PUSH:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg = args[0] & REGISTER_MASK;
            CHECK_EQ(reg, args[0]);
            avrCmd[cmdCount++] = ( AVR_PUSH_OPCODE | (reg << 4) );
        }; break;

        case CPU_JE:
        {
            CHECK_TYPE(0, ARG_REG);
            if ((args[1] == 0) && (argTypes[1] == ARG_IMM))
            {
                uint8_t reg = args[0] & REGISTER_MASK;
                CHECK_EQ(reg, args[0]);
                //TST
                avrCmd[cmdCount++] = ( AVR_AND_OPCODE  | (reg << 4) | ((reg & 0x10) << 5) | (reg & 0x0F) );
            } else
            {
                if (argTypes[1] == ARG_REG)
                {
                    uint8_t reg0 = args[0] & REGISTER_MASK;
                    CHECK_EQ(reg0, args[0]);
                    uint8_t reg1 = args[1] & REGISTER_MASK;
                    CHECK_EQ(reg1, args[1]);
                    avrCmd[cmdCount++] = ( AVR_CP_OPCODE | (reg0 << 4) | ((reg1 & 0x10) << 5) | (reg1 & 0x0F) );
                } else
                {
                    CHECK_TYPE(1, ARG_IMM);
                    uint8_t reg = args[0] & LOWER_REGISTER_MASK;
                    CHECK_EQ(reg, args[0]);
                    uint8_t imm = args[1] & IMMEDIATE_MASK;
                    CHECK_EQ(imm, args[1]);
                    avrCmd[cmdCount++] = ( AVR_CPI_OPCODE | (reg << 4) | (imm & 0x0F) | ((imm & 0xF0) << 4) );
                }
            }
            uint8_t relAddr = 0000000000000000 & ADDRESS_MASK; //TODO
            CHECK_EQ(relAddr, 0000000000000000);
            avrCmd[cmdCount++] = ( AVR_BREQ_OPCODE | (relAddr << 3) );
        }; break;

        case CPU_JMP:
        {
            uint8_t relAddr = 0000000000000000 & ADDRESS_MASK; //TODO
            avrCmd[cmdCount++] = ( AVR_RJMP_OPCODE | relAddr );
        }; break;

        case CPU_CALL:
        {
            uint8_t relAddr = 0000000000000000 & ADDRESS_MASK; //TODO
            avrCmd[cmdCount++] = ( AVR_RCALL_OPCODE | relAddr );
        }; break;

        case CPU_RET:
        {
            avrCmd[cmdCount++] = AVR_RET_OPCODE;
        }; break;

        case CPU_MOV:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg0 = args[0] & REGISTER_MASK;
            CHECK_EQ(reg0, args[0]);
            CHECK_TYPE(1, ARG_REG);
            uint8_t reg1 = args[1] & REGISTER_MASK;
            CHECK_EQ(reg1, args[1]);
            avrCmd[cmdCount++] = ( AVR_MOV_OPCODE       |
                                   (reg0 << 4)          |
                                   ((reg1 & 0x10) << 5) |
                                   (reg1 & 0x0F)        );
        }; break;

        case CPU_INR:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg = args[0] & LOWER_REGISTER_MASK;
            CHECK_EQ(reg, args[0]);
            CHECK_TYPE(1, ARG_IMM);
            uint8_t imm = args[1] & IMMEDIATE_MASK;
            CHECK_EQ(imm, args[1]);
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE    |
                                   (reg << 4)        |
                                   (imm & 0x0F)      |
                                  ((imm & 0xF0) << 4));
        }; break;

#define REG_REG_CASE(OPER, AVR_OPER)                           \
        case CPU_##OPER:                                       \
        {                                                      \
            CHECK_TYPE(0, ARG_REG);                            \
            uint8_t reg0 = args[0] & REGISTER_MASK;            \
            CHECK_EQ(reg0, args[0]);                           \
            CHECK_TYPE(1, ARG_REG);                            \
            uint8_t reg1 = args[1] & REGISTER_MASK;            \
            CHECK_EQ(reg1, args[1]);                           \
            avrCmd[cmdCount++] = ( AVR_##AVR_OPER##_OPCODE  |  \
                                   (reg0 << 4)              |  \
                                   ((reg1 & 0x10) << 5)     |  \
                                   (reg1 & 0x0F)            ); \
        }; break;

        REG_REG_CASE(ADDBIN, ADD);
        REG_REG_CASE(SUBBIN, SUB);
#undef REG_REG_CASE

#define COMP_CASE(OPER, AVR_OPER)                                          \
        case CPU_##OPER:                                                   \
        {                                                                  \
            CHECK_TYPE(0, ARG_REG);                                        \
            uint8_t reg0 = args[0] & LOWER_REGISTER_MASK;                  \
            CHECK_EQ(reg0, args[0]);                                       \
            CHECK_TYPE(1, ARG_REG);                                        \
            uint8_t reg1 = args[1] & REGISTER_MASK;                        \
            CHECK_EQ(reg1, args[1]);                                       \
            /*LDI RD, 1*/                                                  \
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) | (1) );   \
            /*CP RD, RR*/                                                  \
            avrCmd[cmdCount++] = ( AVR_CP_OPCODE            |              \
                                   (reg0 << 4)              |              \
                                   ((reg1 & 0x10) << 5)     |              \
                                   (reg1 & 0x0F)            );             \
            /*BRxx +2 instructions*/                                       \
            avrCmd[cmdCount++] = ( AVR_##AVR_OPER##_OPCODE | (0x4 << 3) ); \
            /*LDI RD, 0*/                                                  \
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) );         \
        }; break;

        COMP_CASE(LGEBIN, BRGE)
        COMP_CASE(LEBIN,  BREQ)
        COMP_CASE(LNEBIN, BRNE)
#undef COMP_CASE

        case CPU_LANDBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg0 = args[0] & LOWER_REGISTER_MASK;
            CHECK_EQ(reg0, args[0]);
            CHECK_TYPE(1, ARG_REG);
            uint8_t reg1 = args[1] & REGISTER_MASK;
            CHECK_EQ(reg1, args[1]);
            //TST RD, RD
            avrCmd[cmdCount++] = ( AVR_AND_OPCODE  | (reg0 << 4) | ((reg0 & 0x10) << 5) | (reg0 & 0x0F) );
            //BREQ lbl1
            avrCmd[cmdCount++] = ( AVR_BREQ_OPCODE | (0x10 << 3) );
            //LDI RD, 1
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) | (1) );
            //TST RR, RR
            avrCmd[cmdCount++] = ( AVR_AND_OPCODE  | (reg1 << 4) | ((reg1 & 0x10) << 5) | (reg1 & 0x0F) );
            //BREQ lbl1
            avrCmd[cmdCount++] = ( AVR_BREQ_OPCODE | (0x4 << 3) );
            //JMP lbl2
            avrCmd[cmdCount++] = ( AVR_RJMP_OPCODE | 0x4 );
            //lbl1:
            //LDI RD, 0
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) );
            //lbl2:
        }; break;

        case CPU_LORBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg0 = args[0] & LOWER_REGISTER_MASK;
            CHECK_EQ(reg0, args[0]);
            CHECK_TYPE(1, ARG_REG);
            uint8_t reg1 = args[1] & REGISTER_MASK;
            CHECK_EQ(reg1, args[1]);
            //TST RD, RD
            avrCmd[cmdCount++] = ( AVR_AND_OPCODE  | (reg0 << 4) | ((reg0 & 0x10) << 5) | (reg0 & 0x0F) );
            //BRNE lbl1
            avrCmd[cmdCount++] = ( AVR_BRNE_OPCODE | (0x10 << 3) );
            //LDI RD, 0
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) );
            //TST RR, RR
            avrCmd[cmdCount++] = ( AVR_AND_OPCODE  | (reg1 << 4) | ((reg1 & 0x10) << 5) | (reg1 & 0x0F) );
            //BRNE lbl1
            avrCmd[cmdCount++] = ( AVR_BRNE_OPCODE | (0x4 << 3) );
            //JMP lbl2
            avrCmd[cmdCount++] = ( AVR_RJMP_OPCODE | 0x4 );
            //lbl1:
            //LDI RD, 1
            avrCmd[cmdCount++] = ( AVR_LDI_OPCODE | (reg0 << 4) | (1) );
            //lbl2:
        }; break;

        case CPU_MULBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg0 = args[0] & REGISTER_MASK;
            CHECK_EQ(reg0, args[0]);
            CHECK_TYPE(1, ARG_REG);
            uint8_t reg1 = args[1] & REGISTER_MASK;
            CHECK_EQ(reg1, args[1]);
            //PUSH R0
            avrCmd[cmdCount++] = ( AVR_PUSH_OPCODE );
            //PUSH R1
            avrCmd[cmdCount++] = ( AVR_PUSH_OPCODE | (1 << 4) );
            //MUL RD, RR -> R0(lower):R1(higher)
            avrCmd[cmdCount++] = ( AVR_MUL_OPCODE           |
                                   (reg0 << 4)              |
                                   ((reg1 & 0x10) << 5)     |
                                   (reg1 & 0x0F)            );

            //MOV RD, 0
            avrCmd[cmdCount++] = ( AVR_MOV_OPCODE | (reg0 << 4) );
            //POP R1
            avrCmd[cmdCount++] = ( AVR_POP_OPCODE | (0x1 << 4) );
            //POP R0
            avrCmd[cmdCount++] = ( AVR_POP_OPCODE );
        }; break;

        case CPU_OUT:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg = args[0] & REGISTER_MASK;
            CHECK_EQ(reg, args[0]);

            uint8_t ioReg = ATMEGA16_PORTD & IO_REGISTER_MASK;
            CHECK_EQ(ioReg, ATMEGA16_PORTD);

            avrCmd[cmdCount++] = ( AVR_OUT_OPCODE        |
                                   (reg << 4)            |
                                   (ioReg & 0x0F)        |
                                  ((ioReg & 0x30) << 5)  );
        }; break;

        case CPU_IN:
        {
            CHECK_TYPE(0, ARG_REG);
            uint8_t reg = args[0] & REGISTER_MASK;
            CHECK_EQ(reg, args[0]);

            uint8_t ioReg = ATMEGA16_PIND & LOWER_IO_REGISTER_MASK;
            CHECK_EQ(ioReg, ATMEGA16_PIND);
            //sbis PIND, 7
            //RJMP -1
            enum { PD_SIGNAL_VALIDITY_PIN = 7 };
            //Wait until PD7 become high
            avrCmd[cmdCount++] = ( AVR_SBIS_OPCODE | (PD_SIGNAL_VALIDITY_PIN) | (ioReg << 3) );
            avrCmd[cmdCount++] = ( AVR_RJMP_OPCODE | ((uint64_t)0xFFE) );
            //                                                   ^
            //                                             Jump one instrucion back (-2)

            avrCmd[cmdCount++] = ( AVR_IN_OPCODE         |
                                   (reg << 4)            |
                                   (ioReg & 0x0F)        |
                                  ((ioReg & 0x30) << 5)  );
        }; break;

        case CPU_NOP:
        {
            avrCmd[cmdCount++] = ( AVR_NOP_OPCODE );
        }; break;

        case CPU_DEFAULT:
        default:
        {
            printf("\nUnknown instruction 0x%x is passed to %s\n",
                    cmd, __func__);
            exit(BINTRANERR_INVALID_INSTRUCTION);
        }

    }
    return cmdCount;
}

AvrCmd avrOpcodeIn(uint8_t arg1)
{
    CHECK_TYPE(0, ARG_REG);
    uint8_t reg = arg1 & REGISTER_MASK;
    CHECK_EQ(reg, arg1);

    uint8_t ioReg = ATMEGA16_PIND & LOWER_IO_REGISTER_MASK;
    CHECK_EQ(ioReg, ATMEGA16_PIND);

    return ( AVR_IN_OPCODE         |
             (reg << 4)            |
             (ioReg & 0x0F)        |
            ((ioReg & 0x30) << 5)  );
}

AvrCmd avrOpcodeNop(void)
{
    return AVR_NOP_OPCODE;
}

#ifdef DEPRECATED_ON
void changeEndianness(void* arr, size_t msize, size_t nmemb)
{
    assert(arr);
    for (size_t i = 0; i < nmemb; i += msize)
    {
        char* curArr = (char*)(arr + i);
        for (int j = 0; j < msize / 2; j++)
        {
            char temp = curArr[j];
            curArr[j] = curArr[(msize - 1) - j];
            curArr[(msize - 1) - j] = temp;
        }
    }
}
#endif

#undef CHECK_EQ
#undef CHECK_TYPE
