#include "Binary_Translator.h"
#include "stdio.h"

int main(void)
{
    struct BinaryTranslator bt = {};

    btCtor(&bt, stdin, true);

    btTranslate(&bt);
    printBuf(&bt);
    btDtor(&bt);
    return 0;
}
