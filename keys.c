#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct termios old, current;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  current = old; /* make new settings same as old settings */
  current.c_lflag &= ~ICANON; /* disable buffered i/o */
  if (echo) {
      current.c_lflag |= ECHO; /* set echo mode */
  } else {
      current.c_lflag &= ~ECHO; /* set no echo mode */
  }
  tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}

/* Let's test it out 
int main(void) 
{
  char c;
  
  
  // while (c == 91 || c == 93){
	  printf("(getche example) please type a letter: ");
	  c = getche();
	  printf("\nYou typed: %c\n\n", c);
	  // printf("(getch example) please type a letter...");
	  // c = getch();
	  // printf("\nYou typed: %c\n", c);
  // }
  
  return 0;
}
*/

int main(void) 
{
    char c;
    printf("Please type a character: ");
    c = getche();
    printf("\nYou typed: %c\n", c);
    
    if (c == '[' || c == ']') {
        // Do work if '[' or ']'
        printf("You typed a special character: %c\n", c);
    } else {
        // Place the character back on the screen
        printf("\b"); // Move the cursor back by one position
        printf("%c", c); // Print the character again
    }
    
    return 0;
}