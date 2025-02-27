/*
	A UNIX shell that supports the following:
		
		1. The built-in commands cd and exit
		2. All simple UNIX commands
		3. Commands running in the background using &.
		4. Input redirection with < and output redirection with either > or >>.
		5. Commands with a single pipe
		
	Note: All of the example commands appear to work as expected.
	
	Author: Ethan Broskoskie
*/

#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 	// getcwd
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h>
#include <termios.h>

#define HISTORY_SIZE 100 			// max number of cmds

static struct termios old, current;

char history[HISTORY_SIZE][1000]; 	// stores cmd history
int history_count = 0;				// number of cmds
int history_index = 0; 				// index for cycling through history

// Greeting shell during startup 
void init_shell();

// Function to print Current Directory. 
void print_dir();

// Reads and copies command line input into a string, str. 
int get_input(char* str);

// Handles the built-in commands exit and cd.
int builtin_cmd_handler(char** tokens);

// Parses the string given to the command line
void parse_string(char* str, char** tokens);

// Tokenizes the cmd line string, removes spaces, returns the number of tokens in the line
void tokenize_str(char* str, char** tokens);

// Function where the system command is executed 
void process(char** tokens);

// Function to print command history
void print_history();

// Function to add a command to history
void add_to_history(const char *cmd);

// Read arrow key input 
char read_arrow_key();

void initTermios(int echo);

void resetTermios(void);

char getch_(int echo);

char getch(void);

int main(int argc, char *argv[]) 
{ 
	char input[1000];
	char* token_args[100];
	
	
	
	if (argc == 2)
	{
		FILE *batch = fopen(argv[1], "r");
		if (batch == NULL)
		{
			perror("Error opening batch file");
			return 1;
		}
		
		// Read and execute commands from the batch file line by line
        while (fgets(input, sizeof(input), batch) != NULL) 
		{
			/*
				Removes carriage return followed by a newline character.
			    I was having a weird problem where this would show up when 
				parsing my batch file, this line fixed it.
			*/
			input[strcspn(input, "\r\n")] = '\0';
            parse_string(input, token_args);
        }
		fclose(batch);
	}
	else if (argc == 1)
	{
		init_shell();
		while (1)
		{
			print_dir();
			
			// take the entire line of input
			if (get_input(input) == 0) 
				//continue; 
			
			{
				// Parse and execute the command
				parse_string(input, token_args);
			}
		} 
	}
	else 
	{
        printf("Usage: %s [batch_file]\n", argv[0]);
        return 1;
    }
	
	return 0;
}

void init_shell() 
{ 
    printf("\n\n\n\n------------------------------------------"); 
    printf("\n\n\n\t    Welcome to my shell"); 
    printf("\n\n\t      - By: Ethan B -"); 
    printf("\n\n\n\n------------------------------------------\n"); 
} 

void print_dir() 
{ 
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); 
    printf("%s$ ", cwd); 
} 

int get_input(char* str) 
{ 
    char ch;
    int pos = 0;	// position in 
	int non_space = 0; // flag to track if non-space characters are encountered
    do 
	{
        ch = read_arrow_key();
        if (ch == 'U' || ch == 'D') 
		{
            for (int i = 0; i < pos; i++) {
                printf("\b \b"); // Overwrite previous characters with spaces
            }
			
            printf("%s", history[history_index]);  // print command with prompt
            fflush(stdout); // Flush output
            pos = strlen(history[history_index]);
            strcpy(str, history[history_index]);
        } 
		else 
		{
            // Handle other characters
			if (ch > 32 && ch <= 126)
				non_space = 1;
            if (ch == '\n') 
			{
                if (pos == 0 || str == NULL  || !non_space) 
				{
                    printf("\n");	// a new line for the directory
					return 1;
                } 
				else 
				{
                    str[pos] = '\0'; // Null-terminate string
                    printf("\n");
                    add_to_history(str);
                }
                return 0;
            } 
			else if (ch == 127 && pos >= 0) // backspace
			{ 
                printf("\b \b");
                pos--;
            } 
			else if (ch >= 32 && ch <= 126) // printable characters
			{ 
                printf("%c", ch);
                str[pos++] = ch;
            }
        }
    } while (ch != '\n' && pos < 999);
    str[pos] = '\0'; // Null-terminate string if buffer is full
	return 0;
}



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
	
    //return key;
}

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

int builtin_cmd_handler(char** tokens) 
{ 
    int cmd_count = 3;
    int curr_arg = 0; 
    char* cmd_list[cmd_count]; 
  
    cmd_list[0] = "exit"; 
    cmd_list[1] = "cd";  
    cmd_list[2] = "history";  
  
    for (int i = 0; i < cmd_count; i++) 
    { 
    	// if our token matches one of the built-in commands
        if (strcmp(tokens[0], cmd_list[i]) == 0) 
        { 
            curr_arg = i + 1; 
            break; 
        } 
    } 
  
  	// Determine which cmd is being called
    if (curr_arg == 1) 
    {
		exit(0);
	} 
	else if (curr_arg == 2) 
	{
		if (*tokens[1] == '~')
			chdir(getenv("HOME"));
		else
			chdir(tokens[1]);
		return 1;
	}
	else if (curr_arg == 3) 
	{
		print_history();
		return 1;
	}
  
    return 0; 
} 

/*

*/
void print_history() 
{
    printf("\n");
    for (int i = 0; i < history_count; i++) 
	{
		if (i < 9)
			printf(" %d: %s\n", i + 1, history[i]);
		else
			printf("%d: %s\n", i + 1, history[i]);
    }
	printf("\n");
}

/*

*/
void add_to_history(const char *cmd) 
{
    // Check if history is full, if so, remove the oldest command
    if (history_count == HISTORY_SIZE) 
	{
        for (int i = 1; i < HISTORY_SIZE; i++) 
		{
            strcpy(history[i - 1], history[i]);
        }
        history_count--;
    }
    strcpy(history[history_count], cmd);
    history_count++;
	history_index = history_count;
}

void tokenize_str(char* str, char** tokens) 
{ 	
    for (int i = 0; i < 100; i++) // 100 is our maximum size
    { 
    	// put each string token into the list
        tokens[i] = strsep(&str, " "); 
  
  		// if the current token is null
        if (tokens[i] == NULL) 
            break; 
        
        // if the token is an empty string, don't store it
        if (strlen(tokens[i]) == 0) 
		{
            i--; 
		}
    } 
} 

/*
	Sets up the redirection files if found, and replaces any input or output
	files and redirection symbols with NULL. Then, it executes all the commands.
	If there is piping and redirection in the same line, it opens the output 
	file for writing, and then executes the rest of the command, including the
	piping.
*/
void parse_string(char* str, char** tokens) 
{ 
	//add_to_history(str);

	tokenize_str(str, tokens); 
	
	if (tokens[0] == NULL)
		perror("no tokens");
	
	// copy the stdin, stdout, and stderr descriptors so they can 
	// be restored later
	int std_in = dup(0);
	int std_out = dup(1);
	int std_err = dup(2);
	
	int i = 1;
	char* curr_phrase[100];
	char* first_phrase[100];
	first_phrase[0] = tokens[0];
	int is_piped = 0;
	
	// Search through the tokens to check if we have piping and then a redirection.
	// If we do, open the appropriate file with the appropriate appending 
	// strategy.
	int k = 0;
	int pp = 0;
	while (tokens[k] != NULL)
	{
		if (strcmp(tokens[k], "|") == 0)
		{
			pp = 1;
		}
		
		if (pp == 1)
		{
			if (strcmp(tokens[k], ">") == 0)
			{
				int out = open(tokens[k + 1], O_WRONLY | O_TRUNC | O_CREAT, 0666);
				if (out < 0 || dup2(out, 1) < 0)
				{
					perror("redirection '>' ");
				}

				close(out);
				tokens[k] = NULL;
				tokens[k + 1] = NULL;
				break;
			}
			else if(strcmp(tokens[k], ">>") == 0)
			{
				int out = open(tokens[k + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
				if (out < 0 || dup2(out, 1) < 0)
				{
					perror("redirection '>>' ");
				}

				close(out);
				tokens[k] = NULL;
				tokens[k + 1] = NULL;
				break;
			}
			k++;
		}
		else 
		{
			k++;
		}
	}
	
	while (tokens[i] != NULL)
	{
		first_phrase[i] = tokens[i];
		
		if (strcmp(tokens[i], "<" ) == 0)
		{	// Input redirection
			int in = open(tokens[i + 1], O_RDONLY, 0666);
			if (in < 0 || dup2(in, 0) < 0)
			{
				perror("redirection '<' ");
			}

			close(in);
			tokens[i] = NULL;
			tokens[i + 1] = NULL;
			i += 2;
		}
		else if(strcmp(tokens[i], ">") == 0)
		{
			int out = open(tokens[i + 1], O_WRONLY | O_TRUNC | O_CREAT, 0666);
			if (out < 0 || dup2(out, 1) < 0)
			{
				perror("redirection '>' ");
			}

			close(out);
			tokens[i] = NULL;
			tokens[i + 1] = NULL;
			i += 2;
		}
		else if(strcmp(tokens[i], ">>") == 0)
		{
			int out = open(tokens[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
			if (out < 0 || dup2(out, 1) < 0)
			{
				perror("redirection '>>' ");
			}

			close(out);
			tokens[i] = NULL;
			tokens[i + 1] = NULL;
			i += 2;
		}
		else if (strcmp(tokens[i], "|") == 0) 
		{
			is_piped = 1;
			int j = 0;
			first_phrase[i] = NULL;
			tokens[i] = NULL;
			i++;
			while (tokens[i] != NULL)
			{
				curr_phrase[j] = tokens[i];
				tokens[i] = NULL;
								
				j++;
				i++;
			}
			curr_phrase[j] = NULL;
			
            // Pipe redirection
			int fd[2];
			pipe(fd);
			
			// left side of pipe
			if (fork() == 0) 
			{
				// replace stdout with pipe output
				dup2(fd[1], STDOUT_FILENO);
				close(fd[0]);
				close(fd[1]);
				
				// execute command and check for error
				if (execvp(first_phrase[0], first_phrase) == -1)
				{
					perror("execvp first command");
					exit(EXIT_FAILURE);
				}
			}
			
			// right side of pipe
			if (fork() == 0)
			{
				// replace stdin with pipe input
				dup2(fd[0], STDIN_FILENO);
				close(fd[0]);
				close(fd[1]);
				
				if (execvp(curr_phrase[0], curr_phrase) == -1)
				{
					perror("execvp second command");
					exit(EXIT_FAILURE);
				}
			}
			
			// parent doesn't need the pipe anymore
			close(fd[0]);
			close(fd[1]);
			
			// wait for both children to finish
			while (wait(NULL) > 0);
		}
		else
		{
			i++;
		}
	}
  
	// if the cmd is a built-in one, execute it. If it's a normal UNIX command
	// without piping, execute it.
    if (builtin_cmd_handler(tokens)) 
	{
		dup2(std_in, 0);
		dup2(std_out, 1);
		dup2(std_err, 2);
	}
	else if (is_piped == 0)
	{
		process(tokens);
	}
	
	dup2(std_in, 0);
	dup2(std_out, 1);
	dup2(std_err, 2);
} 

void process(char** tokens) 
{ 
	int is_background = 0;
	for (int i = 0; tokens[i] != NULL; i++) 
	{
		// if we're trying to run the process in the background
		if (strcmp(tokens[i], "&") == 0) 
		{
			// update flag and remove the & from the command args
			is_background = 1;
			tokens[i] = NULL;
			break;
		}
	}

    // forking a child 
    pid_t pid = fork();  
  
    if (pid == -1) 
	{ 
        printf("\nFailed forking child.."); 
        return; 
    } 
	else if (pid == 0) 
	{ 
        if (execvp(tokens[0], tokens) < 0) 
		{ 
            printf("Could not execute command\n"); 
			exit(1);
        } 
        exit(0); 
    } 
	else 
	{ 
        // if process shouldn't run in background, wait for the 
		// child process to finish
        if (is_background < 1) 
		{
            waitpid(pid, NULL, 0);
        }

        return; 
    } 
} 
