/**
*   Name:       Kyle Colantonio, Kulveer Kaur, Panchito Rosario
*   Group:      F
*   Date:       9-21-2014
*   Project 1:  image to ACSII conversion
*   Goal:       Convert a stream of numerical characters into an ACSII image.
**/

#include <stdio.h>
#include <stdlib.h>
#include "shade.h"

int main(int argc, char *argv[]) {
    int x = 0,y = 0;
    int width, height;
    char c;

    //Check for valid command line arguments.
    if (argc != 3) {
        printf("Requires two (2) arguments; width and height\n");
        printf("Usage: %s [width] [height]\n", argv[0]);
        return 1;
    }
    
    //Set variables
    width = atoi(argv[1]);
    height = atoi(argv[2]);
   
    //Read each character until EOF
    while(c != EOF) {
        c = getchar();

        if (!shade(c, &c)) {
            printf("\nERROR: Invalid character '%c'.\n", c);
            return 1;
        }
        printf("%c", c);

        x++;
        if (x >= width) {
             //Create a new row when x >= width and add +1 to the height.
             printf("\n");
             x = 0;
             y++;
        }
        if (y >= height) {
            break; //End the program if we reach the specified height.
        }
    }
    
    //Print a new line and free memory.
    printf("\n");
    return 0;
}
