#include "Assembler/CommonAssembler.hpp"
#include "stdint.h"
#include "stdio.h"
#include "assert.h"

typedef uint16_t AvrCmd;

enum { REGISTER_MASK          = 0x001F };
enum { LOWER_REGISTER_MASK    = 0x000F };
enum { LONG_ADDRESS_MASK      = 0x0FFF };
enum { ADDRESS_MASK           = 0x007F };
enum { IMMEDIATE_MASK         = 0x00FF };
enum { IO_REGISTER_MASK       = 0x003F };
enum { LOWER_IO_REGISTER_MASK = 0x001F };
enum { BITNUM_MASK            = 0x0007 };

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

enum { ATMEGA16_SPL   = 0x3D };
enum { ATMEGA16_SPH   = 0x3E };

enum { ATMEGA16_RAMEND_LOW  = 0x5F };
enum { ATMEGA16_RAMEND_HIGH = 0x04 };

enum { ATMEGA16_INTERRUPT_TABLE_SIZE = 21 };
//

enum { PD_SIGNAL_VALIDITY_PIN = 7 };
enum { MAX_RESULT_CMD_SIZE  = 32   };
enum { MAX_PROGRAM_SIZE     = 2048 };

enum BinaryTranslatorError
{
    BINTRANERR_NO_ERROR = 0,
    BINTRANERR_INVALID_ARGUMENT_TYPE,
    BINTRANERR_TOO_BIG_ARGUMENT_VALUE,
    BINTRANERR_INVALID_INSTRUCTION
};

struct BinaryTranslator
{
    FILE*  avrRawDataFile;
    FILE*  dumpster;
    enum AsmCmd      cmds               [MAX_PROGRAM_SIZE];
    uint64_t         args               [MAX_PROGRAM_SIZE][MAX_ARG_QT];
    enum ArgAddrMode argTypes           [MAX_PROGRAM_SIZE][MAX_ARG_QT];
    char             phikiProgramBuffer [MAX_PROGRAM_SIZE];
    AvrCmd           avrProgram         [MAX_PROGRAM_SIZE];
    int              phikiCmdAddress    [MAX_PROGRAM_SIZE];
    bool             isActualAddress    [MAX_PROGRAM_SIZE];
    bool             afterOptimAddress  [MAX_PROGRAM_SIZE];

    size_t           phikiProgramBufferSize;

    size_t           avrProgramSize;
    size_t           phikiProgramSize;
};

void     btCtor                 (struct BinaryTranslator* bt, FILE* file, bool makeDump);
void     btDtor                 (struct BinaryTranslator* bt);
int      btTranslate            (struct BinaryTranslator* bt);
int      btPhikiToAvrCmd        (AvrCmd*           avrCmd,
                                 enum AsmCmd       cmd,
                                 bool*             isActualAddress,
                                 enum ArgAddrMode* argTypes,
                                 uint64_t*         args);
int      btOptimizePushesPops   (struct BinaryTranslator* bt);
int      btRedispatchAddresses  (struct BinaryTranslator* bt);
uint16_t btJumpGetArgument      (AvrCmd cmd);
uint16_t btBranchGetArgument    (AvrCmd cmd);
int      btGetNewCmdAddress     (struct BinaryTranslator* bt, int oldCmdAddress);

void     btDumpCmd              (AvrCmd* avrCmd, int cmdCount, FILE* stream);
void     printBuf               (struct BinaryTranslator* bt);

bool     btAvrIsPush            (AvrCmd cmd);
bool     btAvrIsPop             (AvrCmd cmd);
bool     btAvrIsJump            (AvrCmd cmd);
bool     btAvrIsBranch          (AvrCmd cmd);
bool     btAvrIsJumpOrBranch    (AvrCmd cmd);

bool     btAvrPushOpcode        (AvrCmd cmd);
bool     btAvrPopOpcode         (AvrCmd cmd);
bool     btAvrJumpOpcode        (AvrCmd cmd);
bool     btAvrBranchOpcode      (AvrCmd cmd);
