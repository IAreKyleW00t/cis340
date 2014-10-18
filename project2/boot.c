#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boot.h"
#include "Fat12Boot.h"
#include "Fat12Entry.h"

//Define global variables
FILE *img = NULL;
int mounted = 0;
char *filename = '\0';

//Define FAT12Boot Structure
struct Fat12Boot boot;
struct Fat12Entry *entry;

/*
*   Loads all FAT12 boot information into 'boot' and FAT12 entries into 'entry'
*/
void load() {
    fseek(img, 11, SEEK_SET); //skip 11 bytes

    //BYTES_PER_SECTOR (2 bytes)
    fread(&boot.BYTES_PER_SECTOR, 2, 1, img);

    //SECTORS_PER_CLUSTER (2 bytes)
    fread(&boot.SECTORS_PER_CLUSTER, 1, 1, img);

    //RESERVED_SECTORS (2 bytes)
    fread(&boot.RESERVED_SECTORS, 2, 1, img);

    //NUM_OF_FATS (1 byte)
    fread(&boot.NUM_OF_FATS, 1, 1, img);

    //MAX_ROOT_DIRS (2 bytes)
    fread(&boot.MAX_ROOT_DIRS, 2, 1, img);

    //Initialize 'entry'
    entry = (struct Fat12Entry *)malloc(sizeof(struct Fat12Entry) * boot.MAX_ROOT_DIRS);

    //TOTAL_SECTORS (2 bytes)
    fread(&boot.TOTAL_SECTORS, 2, 1, img);

    fseek(img, 1, SEEK_CUR); //skip 1 byte

    //SECTORS_PER_FAT (2 bytes)
    fread(&boot.SECTORS_PER_FAT, 2, 1, img);

    //SECTORS_PER_TRACK (2 bytes)
    fread(&boot.SECTORS_PER_TRACK, 2, 1, img);

    //NUM_OF_HEADS (2 bytes)
    fread(&boot.NUM_OF_HEADS, 2, 1, img);

    fseek(img, 11, SEEK_CUR); //skip 11 bytes

    //VOUME_ID (4 bytes)
    fread(&boot.VOLUME_ID, 4, 1, img);

    //VOLUME_LABEL
    fread(&boot.VOLUME_LABEL, 11, 1, img);

    //Move to beginning of ROOT DIRECTORY
    fseek(img, ((boot.NUM_OF_FATS * boot.SECTORS_PER_FAT) + 1) * boot.BYTES_PER_SECTOR, SEEK_SET);

    for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
        //FILENAME (8 bytes)
        fread(&entry[i].FILENAME, 8, 1, img);

        //EXT (3 bytes)
        fread(&entry[i].EXT, 3, 1, img);

        //ATTRIBUTES (1 byte)
        fread(&entry[i].ATTRIBUTES, 1, 1, img);

        //RESERVED (10 bytes)
        fread(&entry[i].RESERVED, 2, 1, img);

        //CREATION_TIME (2 bytes)
        fread(&entry[i].CREATION_TIME, 2, 1, img);

        //CREATION_DATE (2 bytes)
        fread(&entry[i].CREATION_DATE, 2, 1, img);

        //LAST_ACCESS_DATE (2 bytes)
        fread(&entry[i].LAST_ACCESS_DATE, 2, 1, img);

        fseek(img, 2, SEEK_CUR); //skip 2 bytes

        //MODIFY_TIME (2 bytes)
        fread(&entry[i].MODIFY_TIME, 2, 1, img);

        //MODIFY_DATE (2 bytes)
        fread(&entry[i].MODIFY_DATE, 2, 1, img);

        //START_CLUSTER (2 bytes)
        fread(&entry[i].START_CLUSTER, 2, 1, img);

        //FILE_SIZE (4 bytes)
        fread(&entry[i].FILE_SIZE, 4, 1, img);
    }
}

/*
*   Clears 'boot'
*/
void unload() {
    boot = (struct Fat12Boot){0};
    entry = (struct Fat12Entry *){0};
}

/*
*   Saves file information and opens the file
*/
void mount(char *file) {
    printf("    Mounting `%s`... ", file);
	filename = (char *)malloc(strlen(file) + 1);
	strcpy(filename, file);
    img = fopen(file, "r");
    mounted = 1;
    load();
    printf("Done!\n");
}

/*
*   Clears  file information and closes the file
*/
void unmount() {
    printf("    Unmounting `%s`... ", filename);
    filename = '\0';
    fclose(img);
    mounted = 0;
    unload();
    printf("Done!\n");
}

/*
*   Prints FAT12 image structure
*/
void structure(int l) {
    printf("        number of FAT:                      %d\n", boot.NUM_OF_FATS);
    printf("        number of sectors used by FAT:      %d\n", boot.SECTORS_PER_FAT);
    printf("        number of sectors per cluster:      %d\n", boot.SECTORS_PER_CLUSTER);
    printf("        number of ROOT Entries:             %d\n", boot.MAX_ROOT_DIRS);
    printf("        number of bytes per sector          %d\n", boot.BYTES_PER_SECTOR);
    if (l) {
        printf("        ---Sector #---      ---Sector Types---\n");
        printf("             0                    BOOT\n");

        for(int i = 0; i < boot.NUM_OF_FATS; i++)
                printf("          %02d -- %02d                FAT%d\n", (boot.SECTORS_PER_FAT * i) + 1, boot.SECTORS_PER_FAT * (i + 1), i);

        printf("          %02d -- %02d                ROOT DIRECTORY\n", boot.SECTORS_PER_FAT * boot.NUM_OF_FATS, (boot.MAX_ROOT_DIRS / 16) + (boot.SECTORS_PER_FAT * boot.NUM_OF_FATS)); 
    }
}

/*
*   Lists all files within the FAT12 image
*/
void traverse(int l) {
    if (l) {
        printf("    *****************************\n");
        printf("    ** FILE ATTRIBUTE NOTATION **\n");
        printf("    **                         **\n");
        printf("    ** R ------ READ ONLY FILE **\n");
        printf("    ** S ------ SYSTEM FILE    **\n");
        printf("    ** H ------ HIDDEN FILE    **\n");
        printf("    ** A ------ ARCHIVE FILE   **\n");
        printf("    *****************************\n");
        printf("\n");

        for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
            if (entry[i].FILENAME[0] != 0x00 && entry[i].START_CLUSTER != 0) {
                char attr[6] = {'-', '-', '-', '-', '-'};
                unsigned char a = entry[i].ATTRIBUTES[0];
                if (a == 0x01)
                    attr[0] = 'R';
                if (a == 0x02)
                    attr[1] = 'H';
                if (a == 0x04)
                    attr[2] = 'S';
                if (a == 0x20)
                    attr[5] = 'A';
                if (a == 0x10) {
                    for (int j = 0; j < 6; j++)
                        attr[j] = '-';
                }

                if (entry[i].ATTRIBUTES[0] == 0x10) {
                    printf("%.6s    %d %d       < DIR >      /%.8s                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, entry[i].START_CLUSTER);
                    printf("%.6s    %d %d       < DIR >      /%.8s/.                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, entry[i].START_CLUSTER);
                    printf("%.6s    %d %d       < DIR >      /%.8s/..                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, 0);
                } else {
                    printf("%.6s    %d %d       %lu      /%.8s.%.3s                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILE_SIZE, entry[i].FILENAME, entry[i].EXT, entry[i].START_CLUSTER);
                }
            }
        }

    } else {
        for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
            if (entry[i].FILENAME[0] != 0x00 && entry[i].START_CLUSTER != 0) {
                if (entry[i].ATTRIBUTES[0] == 0x10) {
                    printf("/%.8s                       < DIR >\n", entry[i].FILENAME);
                    printf("/%.8s/.                     < DIR >\n", entry[i].FILENAME);
                    printf("/%.8s/..                    < DIR >\n", entry[i].FILENAME);
                } else {
                    printf("/%.8s.%.3s\n", entry[i].FILENAME, entry[i].EXT);
                }
            }
        }
    }
}

/*
*   Outputs a hex dump of all FAT tables
*/
void showfat() {
    unsigned char in;

    fseek(img, boot.BYTES_PER_SECTOR, SEEK_SET);

    printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");
    for (int i = 0; i < (boot.NUM_OF_FATS * boot.SECTORS_PER_FAT * boot.BYTES_PER_SECTOR); i++) {
        if (i % 16 == 0 || i == 0) {
            printf("\n");
            printf("%4x", i);
        }
        fread(&in, 1, 1, img);
        printf("%5x", in);
    }

    printf("\n");
}

/*
*   Outputs a hex dump of a specific sector (512 bytes)
*/
void showsector(int sector) {
    unsigned char in;

    fseek(img, boot.BYTES_PER_SECTOR * sector, SEEK_SET);

    printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");
    for (int i = 0; i < boot.BYTES_PER_SECTOR; i++) {
        if (i % 16 == 0 || i == 0) {
            printf("\n");
            printf("%4x", i);
        }
        fread(&in, 1, 1, img);
        printf("%5x", in);
    }

    printf("\n");
}

/*
*   Outputs a hex dump of a specific file
*/
void showfile(char *file) {
    char *filename;
	char *ext;
	char *full_filename;
	char *p;
	
	//Parse filename
	p = strtok(file, ".");
	filename = p;
	
	//Parse extension
	p = strtok(NULL, ".");
	ext = p;
	
	full_filename = (char *)malloc(strlen(filename) + strlen(ext) + 1);
	strcpy(full_filename, filename);
	strcat(full_filename, ext);
	
	printf("%s\n", full_filename);
	
	for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
		if (entry[i].FILENAME != 0x00 && entry[i].START_CLUSTER != 0) {
			if(equals(full_filename, (char *)entry[i].FILENAME)) {
				unsigned char in;
				
				fseek(img, ((boot.MAX_ROOT_DIRS / 16) + (boot.SECTORS_PER_FAT * boot.NUM_OF_FATS) - 1) + (boot.BYTES_PER_SECTOR * entry[i].START_CLUSTER), SEEK_SET);

				printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");
				for (int j = 0; j < entry[i].FILE_SIZE; j++) {
					if (j % 16 == 0 || j == 0) {
						printf("\n");
						printf("%4x", j);
					}
					fread(&in, 1, 1, img);
					printf("%5x", in);
				}

				printf("\n");
			}
		}
	}
}

/*
*	Compares two char*'s exactly
*	Used for comparing FILENAME's
*/
int equals(char * str1, char *str2) {
	for (int i = 0; i < strlen(str1); i++) {
		if (str1[i] != str2[i])
			return 0;
	}
	return 1;
}

/*
*   Returns the filename
*/
char *getFile() {
    return filename;
}

/*
*   Returns 1 if the file is mounted, 0 if it is not.
*/
int isMounted() {
    return mounted;
}
