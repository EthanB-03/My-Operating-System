#include <stdio.h>

#define BUFFERSIZE 100

int main (int argc, char *argv[])
{
    char buffer[BUFFERSIZE];
    char buffer2[BUFFERSIZE];
    fgets(buffer, BUFFERSIZE , stdin);
    printf("Read: %s", buffer);
	fgets(buffer2, BUFFERSIZE , stdin);
    printf("Read: %s", buffer2);
    return 0;
}