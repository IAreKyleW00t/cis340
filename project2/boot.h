#ifndef _BOOT_H_
#define _BOOT_H_

void load();
void unload();
void mount();
void unmount();
void structure(int l);
void traverse(int l);
void showfat();
void showsector(int sector);
void showfile(char *file);
int equals(char *str1, char* str2);
char *getFile();
int isMounted();

#endif
