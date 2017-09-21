#!/bin/bash
EXAMPLE_FOLDER=./example
FILE_TO_TRANSLATE=LoopFactorial.bin #example.bin #EmptyExample.bin

#Delete results of previous execution
rm $EXAMPLE_FOLDER/*File
#Put output in RawFile
./BinaryTranslator > $EXAMPLE_FOLDER/RawFile < $EXAMPLE_FOLDER/$FILE_TO_TRANSLATE
#Dump
avr-objcopy -I binary -O elf32-avr $EXAMPLE_FOLDER/RawFile $EXAMPLE_FOLDER/ElfFile
avr-objdump -D $EXAMPLE_FOLDER/ElfFile > $EXAMPLE_FOLDER/DisassemblyFile
avr-objcopy -I elf32-avr -O ihex $EXAMPLE_FOLDER/ElfFile $EXAMPLE_FOLDER/HexFile
