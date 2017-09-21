#include "Binary_Translator.h"
#include "AvrInstructions.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define CHECK_TYPE(argNum, type)                                     \
do {                                                                 \
    if (argTypes[argNum] != (type))                                  \
    {                                                                \
        fprintf(stderr, "\nIn cmd %d argument number %d "            \
                          "has invalid type %d(not equal to %d)\n",  \
                           cmd, argNum, argTypes[argNum], type);     \
        exit(BINTRANERR_INVALID_ARGUMENT_TYPE);                      \
    }                                                                \
} while (0)

void btCtor(struct BinaryTranslator* bt, FILE* file, bool makeDump)
{
    assert(file);
    if (makeDump)
    {
        bt->dumpster = fopen("dump.txt", "w");
        assert(bt->dumpster);
    }
    bt->avrRawDataFile = file;
    memset(bt->cmds,               0,     MAX_PROGRAM_SIZE * sizeof(*bt->cmds) );
    memset(bt->args,               0,     MAX_PROGRAM_SIZE * sizeof(*bt->args) );
    memset(bt->argTypes,           0,     MAX_PROGRAM_SIZE * sizeof(*bt->argTypes) );
    memset(bt->phikiProgramBuffer, '\0',  MAX_PROGRAM_SIZE * sizeof(char)   );
    memset(bt->avrProgram,         0xFF,  MAX_PROGRAM_SIZE * sizeof(AvrCmd) );
    memset(bt->isActualAddress,    false, MAX_PROGRAM_SIZE * sizeof(bool)   );
    bt->avrProgramSize = bt->phikiProgramBufferSize = 0;

    for (int i = 0; i < MAX_PROGRAM_SIZE; i++)
    {
        bt->phikiCmdAddress[i] = -1;
    }
    //Read file content in phikiProgramBuffer
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    assert(fileSize != -1);
    bt->phikiProgramBufferSize = fileSize;
    rewind(file);
    int bytesRead = fread(bt->phikiProgramBuffer, sizeof(char), fileSize, file);
    assert(bytesRead == fileSize);
}

void btDtor(struct BinaryTranslator* bt)
{
    assert(bt);
    fclose(bt->dumpster);
    bt->avrRawDataFile = bt->dumpster = NULL;
    bt->avrProgramSize = bt->phikiProgramBufferSize = 0;
}

int btOptimizePushesPops(struct BinaryTranslator* bt)
{
    assert(bt);
    assert(bt->avrRawDataFile);

    if (bt->dumpster)
    {
        fprintf(bt->dumpster, "Optimizations start\n");
    }

    int i = 0;
    fprintf(bt->dumpster, "\nCPU ID = %2x\n", (char)bt->phikiProgramBuffer[i]);
    assert((char)bt->phikiProgramBuffer[i++] == (char)0xFA); //Check CPU ID

    int addressOffset = 0;

    while (i < bt->phikiProgramBufferSize)
    {
        //Get instruction code and arguments and their types(reg, imm)
        char argAddrTypes = bt->phikiProgramBuffer[i++];
        bt->cmds[bt->phikiProgramSize] = (enum AsmCmd)bt->phikiProgramBuffer[i++];

        int argQt = getCmdArgQt(bt->cmds[bt->phikiProgramSize]);
        assert(argQt >= 0);

        for (int argNum = 0; argNum < argQt; argNum++)
        {
            double tempDouble = *(double*)(&bt->phikiProgramBuffer[i]);
            i += sizeof(double);
            bt->args    [bt->phikiProgramSize][argNum] = (uint64_t)tempDouble;
            bt->argTypes[bt->phikiProgramSize][argNum] = (enum ArgAddrMode)((argAddrTypes >> (argNum * 2)) & 0x3);
        };

        //Work on optimization
        if ((bt->cmds[bt->phikiProgramSize]      == CPU_POP)  && //< This is pop
            (bt->phikiProgramSize != 0)                       && //< This isn't first instruction in program
            (bt->cmds[bt->phikiProgramSize - 1]) == CPU_PUSH  )  //< Previous instruction is push
        {
            if (bt->argTypes[bt->phikiProgramSize - 1][0] == ARG_REG)  //< Register was pushed
            {
                if (bt->args[bt->phikiProgramSize][0] == bt->args[bt->phikiProgramSize - 1][0])
                {
                    // PUSH %X
                    // POP  %X
                    addressOffset += 2;
                    bt->phikiProgramSize -= 2; //< Just delete push and pop
                } else
                {
                    // PUSH %X| -\  MOV %Y, %X
                    // POP  %Y| -/
                    addressOffset += 1;
                    uint64_t pushedRegister = bt->args[bt->phikiProgramSize - 1][0];
                    uint64_t poppedRegister = bt->args[bt->phikiProgramSize]    [0];
                    bt->phikiProgramSize--;
                    bt->cmds     [bt->phikiProgramSize]    = CPU_MOV;
                    bt->args     [bt->phikiProgramSize][0] = poppedRegister;
                    bt->argTypes [bt->phikiProgramSize][0] = ARG_REG;
                    bt->args     [bt->phikiProgramSize][1] = pushedRegister;
                    bt->argTypes [bt->phikiProgramSize][1] = ARG_REG;
                }
            } else
            if (bt->argTypes[bt->phikiProgramSize - 1][0] == ARG_IMM)  //< Immediate was pushed
            {
                // PUSH $A|  -\  INR %X, $A
                // POP  %X|  -/
                addressOffset += 1;
                uint64_t pushedValue    = bt->args[bt->phikiProgramSize - 1][0];
                uint64_t poppedRegister = bt->args[bt->phikiProgramSize]    [0];
                bt->phikiProgramSize--;
                bt->cmds     [bt->phikiProgramSize]    = CPU_INR;
                bt->args     [bt->phikiProgramSize][0] = poppedRegister;
                bt->argTypes [bt->phikiProgramSize][1] = ARG_REG;
                bt->args     [bt->phikiProgramSize][1] = pushedValue;
                bt->argTypes [bt->phikiProgramSize][1] = ARG_IMM;
            }
        }
        bt->afterOptimAddress[bt->phikiProgramSize] = bt->phikiProgramSize + addressOffset;

        bt->phikiProgramSize++;
    };

    return 0;
}

int btTranslate(struct BinaryTranslator* bt)
{
    assert(bt);
    assert(bt->avrRawDataFile);
    btOptimizePushesPops(bt);
    //---------Program-Prologue-------------------------------------
    bt->isActualAddress[bt->avrProgramSize] = true;
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeRJMP(ATMEGA16_INTERRUPT_TABLE_SIZE - 1);
    for (int i = 0; i < ATMEGA16_INTERRUPT_TABLE_SIZE - 1; i++)
    {
        bt->avrProgram[bt->avrProgramSize++] = avrOpcodeRETI();
    }
    //LDI r16, low(RAMEND)
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, ATMEGA16_RAMEND_LOW);
    //OUT SPL, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_SPL);
    //LDI r16, high(RAMEND)
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, ATMEGA16_RAMEND_HIGH);
    //OUT SPH, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_SPH);
    //LDI r16, 0
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, 0x00);
    //OUT DDRC, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_DDRC);
    //LDI r16, 0
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, 0x00);
    //OUT PORTC, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_PORTC);
    //LDI r16, 0xFF
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, 0xFF);
    //OUT DDRA, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_DDRA);
    //LDI r16, 0
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, 0xAA);
    //OUT PORTA, r16
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeOUT(0, ATMEGA16_PORTA);
    //CBI DDRD, 7
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeCBI(ATMEGA16_DDRD, 7);
    //CBI PORTD, 7
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeCBI(ATMEGA16_PORTD, 7);
    //LDI r16, 0
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeLDI(0, 0);
    //---------Program-Prologue-End---------------------------------
    size_t i = 0;
    while (i < bt->phikiProgramSize)
    {
        AvrCmd avrCmd         [MAX_RESULT_CMD_SIZE] = {};
        bool   isActualAddress[MAX_RESULT_CMD_SIZE] = {};
        int cmdSize = btPhikiToAvrCmd(avrCmd,
                                      bt->cmds[i],
                                      isActualAddress,
                                      bt->argTypes[i],
                                      bt->args[i]);

        memcpy(bt->avrProgram      + bt->avrProgramSize, avrCmd,          cmdSize * sizeof(AvrCmd));
        memcpy(bt->isActualAddress + bt->avrProgramSize, isActualAddress, cmdSize * sizeof(bool)  );
        bt->phikiCmdAddress[bt->avrProgramSize] = bt->afterOptimAddress[i];
        if (bt->dumpster)
        {
            fprintf(bt->dumpster,
                    "_____________________________________________________");

            fprintf(bt->dumpster,
                    "\ncmd = %d(%s)"
                    "\nargs = {%zu,%zu,%zu}"
                    "\nargTypes = {%d,%d,%d}"
                    "\nphikiProgramSize = %d"
                    "\n  avrProgramSize = %d"
                    "\ncmdSize = %d\n"
                    "\nafterOptimizationAddress = %d\n\n",
                    bt->cmds[i], getCmdName(bt->cmds[i]),
                    bt->args    [i][0], bt->args    [i][1], bt->args    [i][2],
                    bt->argTypes[i][0], bt->argTypes[i][1], bt->argTypes[i][2],
                    i, bt->avrProgramSize,
                    cmdSize, bt->afterOptimAddress[i]);
            for (int i = 0; i < cmdSize; i++)
            {
                fprintf(bt->dumpster,
                        "isActualAddress[%d] = %d\n",
                        i, bt->isActualAddress[bt->avrProgramSize + i]);
            }
            btDumpCmd(bt->avrProgram + bt->avrProgramSize, cmdSize, bt->dumpster);
            fprintf(bt->dumpster,
                    "_____________________________________________________\n");
        }

        bt->avrProgramSize += cmdSize;
        i++;
    };
    //---------Program-Epilogue-------------------------------------
    //Execute program in infinite loop
    bt->avrProgram[bt->avrProgramSize++] = avrOpcodeRJMP(0);
    //---------Program-Epilogue-End---------------------------------

    btRedispatchAddresses(bt);

    if (bt->dumpster)
    {
        btDumpCmd(bt->avrProgram, bt->avrProgramSize, bt->dumpster);
    }
    return 0;
}

int btRedispatchAddresses(struct BinaryTranslator* bt)
{
    for (int i = 0; i < bt->avrProgramSize; i++)
    {
        if (bt->isActualAddress[i])
        {
            continue;
        }
        AvrCmd cmd = bt->avrProgram[i];
        if (btAvrIsJump(cmd))
        {
            int phikiCmdAddress = btJumpGetArgument(cmd);
            int newCmdAddress   = btGetNewCmdAddress(bt, phikiCmdAddress);
            if (newCmdAddress == -1)
            {
                fclose(bt->dumpster);
                assert(newCmdAddress != -1);
            }
            int      addressDiff     = newCmdAddress - i - 1;
            uint16_t relativeAddress = (addressDiff < 0) ?
                                       (addressDiff & LONG_ADDRESS_MASK) :
                                       (addressDiff);
            if (btAvrJumpOpcode(cmd) == AVR_RJMP_OPCODE)
            {
                bt->avrProgram[i] = avrOpcodeRJMP (relativeAddress);
            } else
            if (btAvrJumpOpcode(cmd) == AVR_RCALL_OPCODE)
            {
                bt->avrProgram[i] = avrOpcodeRCALL(relativeAddress);
            }
        } else
        if (btAvrIsBranch(cmd))
        {
            int phikiCmdAddress = btBranchGetArgument(cmd);
            int newCmdAddress   = btGetNewCmdAddress(bt, phikiCmdAddress);
            if (newCmdAddress == -1)
            {
                fclose(bt->dumpster);
                assert(newCmdAddress != -1);
            }
            int      addressDiff     = newCmdAddress - i - 1;
            uint16_t relativeAddress = (addressDiff < 0) ?
                                       (addressDiff & ADDRESS_MASK) :
                                       (addressDiff);
            if (btAvrBranchOpcode(cmd) == AVR_BREQ_OPCODE)
            {
                bt->avrProgram[i] = avrOpcodeBREQ(relativeAddress);
            } else
            if (btAvrBranchOpcode(cmd) == AVR_BRNE_OPCODE)
            {
                bt->avrProgram[i] = avrOpcodeBRNE(relativeAddress);
            } else
            if (btAvrBranchOpcode(cmd) == AVR_BRGE_OPCODE)
            {
                bt->avrProgram[i] = avrOpcodeBRGE(relativeAddress);
            };
        }
    }
}

uint16_t btJumpGetArgument(AvrCmd cmd)
{
    return cmd & AVR_RJMPS_ARGUMENTS_MASK;

}

uint16_t btBranchGetArgument(AvrCmd cmd)
{
    return (cmd & AVR_BRANCHES_ARGUMENTS_MASK) >> 3;
}

int btGetNewCmdAddress(struct BinaryTranslator* bt, int phikiCmdAddress)
{
    for (int i = 0; i < bt->avrProgramSize; i++)
    {
        if ( ( phikiCmdAddress > bt->phikiCmdAddress[i - 1] &&
               phikiCmdAddress < bt->phikiCmdAddress[i]     ) ||
               (bt->phikiCmdAddress[i] == phikiCmdAddress)  )
            return i;
    }
    return -1;
}

bool btAvrIsPush(AvrCmd cmd)
{
    return ( btAvrPushOpcode(cmd) == AVR_PUSH_OPCODE);
}

bool btAvrIsPop(AvrCmd cmd)
{
    return ( btAvrPushOpcode(cmd) == AVR_POP_OPCODE);
}

bool btAvrIsJump(AvrCmd cmd)
{
    return ( ( btAvrJumpOpcode(cmd)   == AVR_RJMP_OPCODE  ) ||
             ( btAvrJumpOpcode(cmd)   == AVR_RCALL_OPCODE ) );
}

bool btAvrIsBranch(AvrCmd cmd)
{
    return ( ( btAvrBranchOpcode(cmd) == AVR_BREQ_OPCODE  ) ||
             ( btAvrBranchOpcode(cmd) == AVR_BRNE_OPCODE  ) ||
             ( btAvrBranchOpcode(cmd) == AVR_BRGE_OPCODE  ) );
}

#define BT_AVR_SMTH_OPCODE(NAME, CONSTANT_NAME)           \
bool btAvr##NAME##Opcode(AvrCmd cmd)                      \
{                                                         \
    return cmd & ~(AVR_##CONSTANT_NAME##_ARGUMENTS_MASK); \
}
BT_AVR_SMTH_OPCODE(Push,   PUSH)
BT_AVR_SMTH_OPCODE(Pop,    POP)
BT_AVR_SMTH_OPCODE(Jump,   RJMPS)
BT_AVR_SMTH_OPCODE(Branch, BRANCHES)
#undef BT_AVR_SMTH_OPCODE

void printBuf(struct BinaryTranslator* bt)
{
    fwrite(bt->avrProgram,
           sizeof(char),
           bt->avrProgramSize * sizeof(AvrCmd),
           stdout);
}

void btDumpCmd(AvrCmd* avrCmd, int cmdCount, FILE* stream)
{
    //changeEndianness(avrCmd, sizeof(AvrCmd), cmdCount);
    for (int i = 0; i < cmdCount * sizeof(AvrCmd) * 8; i++)
    {
        if ((i % 4 == 0) && (i != 0))
        {
            fprintf(stream, " ");
        }
        if ((i % 16 == 0) && (i != 0))
        {
            if (i > 15)
                fprintf(stream, "| %04x", avrCmd[(i - 1) / (sizeof(AvrCmd) * 8)]);
            fprintf(stream, "\n");
        }
        fprintf(stream, "%d", (avrCmd[i / (sizeof(AvrCmd) * 8)] >>
                              ((sizeof(AvrCmd) * 8 - 1) -
                               (i % (sizeof(AvrCmd) * 8)))) & 1 );

    }
    fprintf(stream, " | %04x", avrCmd[cmdCount - 1]);
    fprintf(stream, "\n");
}

int  btPhikiToAvrCmd(     AvrCmd*      avrCmd,
                     enum AsmCmd       cmd,
                          bool*        isActualAddress,
                     enum ArgAddrMode* argTypes,
                          uint64_t*    args)
{
    assert(avrCmd); assert(argTypes); assert(args);
    int  cmdCount = 0;
    switch (cmd)
    {
        case CPU_POP:
        {
            CHECK_TYPE(0, ARG_REG);
            avrCmd[cmdCount++] = avrOpcodePOP(args[0]);
        }; break;

        case CPU_PUSH:
        {
            CHECK_TYPE(0, ARG_REG);
            avrCmd[cmdCount++] = avrOpcodePUSH(args[0]);
        }; break;

        case CPU_JE:
        {
            CHECK_TYPE(0, ARG_REG);
            if (argTypes[1] == ARG_REG)
            {
                avrCmd[cmdCount++] = avrOpcodeCP(args[0], args[1]);
            } else
            {
                avrCmd[cmdCount++] = avrOpcodeCPI(args[0], args[1]);
            }
            avrCmd[cmdCount++] = avrOpcodeBREQ(args[2]);
        }; break;

        case CPU_JMP:
        {
            avrCmd[cmdCount++] = avrOpcodeRJMP(args[0]);
        }; break;

        case CPU_CALL:
        {
            avrCmd[cmdCount++] = avrOpcodeRCALL(args[0]);
        }; break;

#define REG_REG_OPCODE(NAME, AVR_NAME)                          \
case CPU_##NAME:                                                \
{                                                               \
    CHECK_TYPE(0, ARG_REG);                                     \
    CHECK_TYPE(1, ARG_REG);                                     \
    avrCmd[cmdCount++] = avrOpcode##AVR_NAME(args[0], args[1]); \
}; break;
REG_REG_OPCODE(MOV, MOV)
REG_REG_OPCODE(ADDBIN, ADD)
REG_REG_OPCODE(SUBBIN, SUB)
#undef REG_REG_OPCODE

#define COMP_CASE(OPER, AVR_OPER)                       \
case CPU_##OPER:                                        \
{                                                       \
    CHECK_TYPE(0, ARG_REG);                             \
    CHECK_TYPE(1, ARG_REG);                             \
    /*CP RD, RR*/                                       \
    avrCmd[cmdCount++] = avrOpcodeCP(args[0], args[1]); \
    /*BRxx +2*/                                         \
    isActualAddress[cmdCount] = true;                   \
    avrCmd[cmdCount++] = avrOpcode##AVR_OPER(2);        \
    /*LDI RD, 0*/                                       \
    avrCmd[cmdCount++] = avrOpcodeLDI(args[0], 0);      \
    /*JMP +1*/                                          \
    isActualAddress[cmdCount] = true;                   \
    avrCmd[cmdCount++] = avrOpcodeRJMP(1);              \
    /*LDI RD, 1*/                                       \
    avrCmd[cmdCount++] = avrOpcodeLDI(args[0], 1);      \
}; break;
COMP_CASE(LGEBIN, BRGE)
COMP_CASE(LEBIN,  BREQ)
COMP_CASE(LNEBIN, BRNE)
#undef COMP_CASE

        case CPU_INR:
        {
            CHECK_TYPE(0, ARG_REG);
            CHECK_TYPE(1, ARG_IMM);
            avrCmd[cmdCount++] = avrOpcodeLDI(args[0], args[1]);
        }; break;

        case CPU_LANDBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            CHECK_TYPE(1, ARG_REG);
            //TST RD, RD
            avrCmd[cmdCount++] = avrOpcodeTST(args[0]);
            //BREQ lbl1
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeBREQ(0x4);
            //LDI RD, 1
            avrCmd[cmdCount++] = avrOpcodeLDI(args[0], 1);
            //TST RR, RR
            avrCmd[cmdCount++] = avrOpcodeTST(args[1]);
            //BREQ lbl1
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeBREQ(1);
            //JMP lbl2
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeRJMP(1);
            //lbl1:
            //LDI RD, 0
            avrCmd[cmdCount++] = avrOpcodeLDI(args[1], 0);
            //lbl2:
        }; break;

        case CPU_LORBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            CHECK_TYPE(1, ARG_REG);
            //TST RD, RD
            avrCmd[cmdCount++] = avrOpcodeTST(args[0]);
            //BRNE lbl1
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeBRNE(0x4);
            //LDI RD, 0
            avrCmd[cmdCount++] = avrOpcodeLDI(args[0], 0);
            //TST RR, RR
            avrCmd[cmdCount++] = avrOpcodeTST(args[1]);
            //BRNE lbl1
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeBRNE(1);
            //JMP lbl2
            avrCmd[cmdCount++] = avrOpcodeRJMP(1);
            //lbl1:
            //LDI RD, 1
            avrCmd[cmdCount++] = avrOpcodeLDI(args[1], 1);
            //lbl2:
        }; break;

        case CPU_MULBIN:
        {
            CHECK_TYPE(0, ARG_REG);
            CHECK_TYPE(1, ARG_REG);
            //MUL RD, RR -> R0(lower):R1(higher)
            avrCmd[cmdCount++] = avrOpcodeMUL(args[0], args[1]);
            //MOV RD, R0
            //See AvrInstructions.h
            avrCmd[cmdCount++] = avrOpcodeMOVCorrectSourceValue(args[0], 0);
        }; break;

        case CPU_OUT:
        {
            CHECK_TYPE(0, ARG_REG);

            avrCmd[cmdCount++] = avrOpcodeOUT(args[0], ATMEGA16_PORTA);
        }; break;

        case CPU_IN:
        {
            CHECK_TYPE(0, ARG_REG);

            //sbis PIND, 7
            //RJMP -1

            //Wait until PD7 become high
            avrCmd[cmdCount++] = avrOpcodeSBIS(ATMEGA16_PIND, PD_SIGNAL_VALIDITY_PIN);
            isActualAddress[cmdCount] = true;
            avrCmd[cmdCount++] = avrOpcodeRJMP((uint16_t)0x0FFE);
            //                                            ^
            //                                Jump one instruction back (-2)

            avrCmd[cmdCount++] = avrOpcodeIN(args[0], ATMEGA16_PINC);
        }; break;

        case CPU_RET:
        {
            avrCmd[cmdCount++] = avrOpcodeRET();
        }; break;

        case CPU_NOP:
        {
            avrCmd[cmdCount++] = avrOpcodeNOP();
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

#undef CHECK_TYPE
