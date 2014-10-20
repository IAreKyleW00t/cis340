#include <stdio.h>
#include "shell.h"

/*
*   Prints terminal prompt.
*/
void printPrompt() {
    printf("flop> ");
}

/*
*   Prints help information.
*/
void printHelp() {
    printf("======== FLOP HELP =========\n");
    printf("  <required> - [optional]\n\n");
    printf("  fmount:\n");
    printf("    DESC:     Mounts a local floppy image.\n");
    printf("    USAGE:    `fmount <image>`\n");
    printf("  fumount:\n");
    printf("    DESC:     Unmounts a local floppy image.\n");
    printf("    USAGE:    `fumount`\n");
    printf("  structure:\n");
    printf("    DESC:     Lists the structure of the floppy disk.\n");
    printf("    USAGE:    `structure [-l]`\n");
    printf("  traverse:\n");
    printf("    DESC:     Lists the contents of the root directory.\n");
    printf("    USGE:     `traverse [-l]`\n");
    printf("  showfat:\n");
    printf("    DESC:     Show the content of the FAT tables.\n");
    printf("    USAGE:    `showfat`\n");
    printf("  showsector:\n");
    printf("    DESC:     Show the contents of a specified sector.\n");
    printf("    USAGE:    `showsector <number>`\n");
    printf("  showfile:\n");
    printf("    DESC:     Show the contents of a file.\n");
    printf("    USAGE:    `showfile <file>`\n");
    printf("  help:\n");
    printf("    DESC:     Displays all available commands.\n");
    printf("    USGAE:    `help`\n");
    printf("  exit:\n");
    printf("    DESC:     Exits the flop program.\n");
    printf("    USAGE:    `exit`\n");
}

/*
*   Prints invalid command warning.
*/
void printInvalid() {
    printf("    ERROR: Invalid command. Please use `help` for a list of all commands.\n");
}

/*
*   Prints incorrect command usage warning.
*/
void printIncorrect() {
    printf("    ERROR: Incorrect usage. Please see `help` for proper usage.\n");
}
