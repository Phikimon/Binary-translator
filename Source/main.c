#include "Binary_Translator.h"
#include "stdio.h"

void printAvrCmd(AvrCmd* avrCmd, int cmdCount)
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

#define testOneRegInstr(cmd) testOneRegInstr_(cmd, #cmd)
void testOneRegInstr_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_REG };
    uint64_t    args[MAX_ARG_QT] = { 0xA }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testRegRegInstr(cmd) testRegRegInstr_(cmd, #cmd)
void testRegRegInstr_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_REG, ARG_REG };
    uint64_t    args[MAX_ARG_QT] = { 0xA , 0xB }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testNoArgInstr(cmd) testNoArgInstr_(cmd, #cmd)
void testNoArgInstr_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = {};
    uint64_t    args[MAX_ARG_QT] = {};
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testImmInstr(cmd) testImmInstr_(cmd, #cmd)
void testImmInstr_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_IMM };
    uint64_t    args[MAX_ARG_QT] = { 0xA }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testJE_REG_IMM(cmd) testJE_REG_IMM_(cmd, #cmd)
void testJE_REG_IMM_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_REG, ARG_IMM };
    uint64_t    args[MAX_ARG_QT] = { 0xA, 0xB }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("JE_REG_IMM\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testJE_REG_REG(cmd) testJE_REG_REG_(cmd, #cmd)
void testJE_REG_REG_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_REG, ARG_REG };
    uint64_t    args[MAX_ARG_QT] = { 0xA, 0xB }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("JE_REG_REG\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

#define testJE_REG_NULL(cmd) testJE_REG_NULL_(cmd, #cmd)
void testJE_REG_NULL_(AvrCmd cmd, char cmdName[100])
{
    AvrCmd avrCmd[MAX_RESULT_CMD_SIZE] = {};
    enum ArgAddrMode argAddrTypes[MAX_ARG_QT] = { ARG_REG, ARG_IMM };
    uint64_t    args[MAX_ARG_QT] = { 0xA, 0x0 }; //0b1010
    int cmdCount = btPhikiToAvrCmd(avrCmd, cmd, argAddrTypes, args);
    printf("\n____________________________________________________________\n");
    printf("JE_REG_NULL\n");
    printf("cmd = %d(%s)\n", cmd, cmdName);
    printf("Result = \n");

    printAvrCmd(avrCmd, cmdCount);
}

void superTest(void)
{
    testOneRegInstr(CPU_POP);
    testOneRegInstr(CPU_PUSH);
    testOneRegInstr(CPU_OUT);
    testOneRegInstr(CPU_IN);

    testRegRegInstr(CPU_ADDBIN);
    testRegRegInstr(CPU_SUBBIN);
    testRegRegInstr(CPU_LGEBIN);
    testRegRegInstr(CPU_LEBIN);
    testRegRegInstr(CPU_LNEBIN);
    testRegRegInstr(CPU_LANDBIN);
    testRegRegInstr(CPU_LORBIN);
    testRegRegInstr(CPU_MULBIN);
    testRegRegInstr(CPU_MOV);

    testNoArgInstr(CPU_RET);

    testImmInstr(CPU_CALL);
    testImmInstr(CPU_JMP);

    testJE_REG_REG(CPU_JE);
    testJE_REG_IMM(CPU_JE);
    testJE_REG_NULL(CPU_JE);
}

void main(void)
{
    struct BinaryTranslator bt = {};
#ifdef EXAMPLE_FILE
    FILE* exampleFile = fopen("example/example.bin", "r");
    assert(exampleFile);
#endif

#ifdef EXAMPLE_FILE
    btCtor(&bt, exampleFile);
#else
    btCtor(&bt, stdin);
#endif
    btTranslate(&bt);
    printBuf(&bt);
    btDtor(&bt);
#ifdef EXAMPLE_FILE
    fclose(exampleFile); exampleFile = NULL;
#endif
}
