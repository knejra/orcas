#ifndef _USER_H
#define _USER_H

#define NAMELEN 128
#define PWDLEN  128

char usrName[NAMELEN];
char password[PWDLEN];

void login(char *name);
void logout(char *name);

#endif // _USER_H