#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_SIZE 100

static struct termios old, current;
char history[HISTORY_SIZE][1000]; // Command history
int history_count = 0; // Number of commands stored in history
int history_index = 0; // Index for cycling through history

/* Initialize new terminal i/o settings */
void initTermios(int echo) //----------------------------------------------------------------------------------------------
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
void resetTermios(void) //----------------------------------------------------------------------------------------------
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) //----------------------------------------------------------------------------------------------
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) //----------------------------------------------------------------------------------------------
{
  return getch_(0);
}

/* Add a command to history */
void add_to_history(const char *cmd) {
    if (history_count == HISTORY_SIZE) {
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i - 1], history[i]);
        }
        history_count--;
    }
    strcpy(history[history_count], cmd);
    history_count++;
    history_index = history_count; // Reset history index
}


/* Read arrow key input */
char read_arrow_key() 
{
	// up is ^[[A and down is ^[[B
    char escape = 27;	// ' ^[ '
    char bracket = '[';
    char key = getch();
    if (key == escape) 
	{
        key = getch();
        if (key == bracket) 
		{
			key = getch();
			printf("\n-------->%c<-", key);
			switch (key) 
			{
				case 'A': // Up arrow
					if (history_index > 0) 
					{
						history_index--;
						return 'U';
					}
					break;
				case 'B': // Down arrow
					if (history_index < history_count) 
					{
						history_index++;
						return 'D';
					}
					break;
			}
        }
    }
    return key;
}

/* Get command from user */
void get_command(char* cmd) {
    char ch;
	char escape = 27;
    int pos = 0;
    do 
	{
        ch = read_arrow_key();
		printf("\n-------->%c", ch);
        if (ch == 'U' || ch == 'D') {
            // Erase current line
            for (int i = 0; i < pos; i++) {
                printf("\b \b");
            }
            // Print new command from history
            if (ch == 'U' && history_index > 0) {
                history_index--;
            } else if (ch == 'D' && history_index < history_count) {
                history_index++;
            }
            strcpy(cmd, history[history_index]);
            printf("%s", cmd);
            pos = strlen(cmd);
        } else {
            // Handle other characters
            if (ch == '\n') {
                cmd[pos] = '\0'; // Null-terminate string
                add_to_history(cmd);
                printf("\n");
                return;
            } else if (ch == 127 && pos > 0) { // Backspace
                printf("\b \b");
                pos--;
            } else if (ch >= 32 && ch <= 126) { // Printable characters
                printf("%c", ch);
                cmd[pos++] = ch;
            }
        }
    } while (ch != '\n' && ch != escape);
    // } while (ch != '\n' && pos < 999);
    cmd[pos] = '\0'; // Null-terminate string if buffer is full
}

int main() {
    char input[1000];
    initTermios(0); // Disable echo mode
    while (1) {
        printf("$ ");
        get_command(input);
        printf("You entered: %s\n", input);
    }
    resetTermios(); // Restore terminal settings before exiting
    return 0;
}
