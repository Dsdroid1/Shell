/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the signal handler code (which should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you do not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()




char** parseInput(char *commands,int *type)
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	if(commands!=NULL)
	{
		int mode=0;
		/*
			0->single cmd
			1->parallel
			2->sequential
			3->redirection single
		*/
		int parallel=0,sequential=0,prev_hash=0,prev_and=0,isRedir=0,isCommaOpen=0,emptyCheck=0;
		int i=0,len=strlen(commands),error=0;
		for(i=0;i<len&&error==0;i++)//currently, #& and &# are allowed,but add conditon to parse them as wrong
		{
			if(commands[i]!=' ')
			{
				emptyCheck=1;
			}
			if(isCommaOpen==0)
			{
				if(commands[i]=='#')
				{
					if(prev_hash==0)
					{
						prev_hash=1;
					}
					else if(prev_hash==1)
					{
						sequential++;
						prev_hash++;
					}
					else
					{
						error=1;
					}
					prev_and=0;
				}
				else if(commands[i]=='&')
				{
					if(prev_and==0)
					{
						prev_and=1;
					}
					else if(prev_and==1)
					{
						parallel++;
						prev_and++;
					}
					else
					{
						error=1;
					}
					prev_hash=0;
				}
				else if(commands[i]=='>')
				{
					isRedir=1;
					prev_and=0;
					prev_hash=0;
				}
				else if(commands[i]=='"')
				{
					isCommaOpen=1;
					prev_hash=0;
					prev_and=0;
				}
				else
				{
					prev_hash=0;
					prev_and=0;
				}
			}
			else
			{
				if(commands[i]=='"')
				{
					isCommaOpen=0;
					prev_hash=0;
					prev_and=0;
				}
			}
		}
		if(parallel==0&&sequential==0)
		{
			mode=(isRedir==1)?3:0;
		}
		else if(parallel==0&&isRedir==0)
		{
			mode=2;
		}
		else if(sequential==0&&isRedir==0)
		{
			mode=1;
		}
		else
		{
			error=1;
		}
		//DEBUG STUFF
		if(error==0)
		{
			mode=(emptyCheck==1)?mode:-1;//-1 signifies empty string
			//printf("Your command was classified as:%d",mode);
		}
		else
		{
			printf("Shell:Incorrect command\n");
		}
		*type=mode;
		//Want to separate into commands here
		if(error==0)
		{
			char **separate_cmds=NULL;
			if(mode==0||mode==3)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(2));
				separate_cmds[0]=strdup(commands);
				separate_cmds[1]=NULL;
			}
			else if(mode==2)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(sequential+1+1));//seq+1 is no.of cmds,+1 for NULL
				int tokens=0;
				int isCommaOpen=0;
				int prev_hash=0;
				int i=0,found=0,start_loc=0;//to iterate
				while(tokens<sequential+1)
				{
					//Find the ## not inside a string
					found=0;
					for(;i<len&&found==0;i++)
					{
						if(isCommaOpen==0)
						{
							if(commands[i]=='"')
							{
								isCommaOpen=1;
							}
							else if(commands[i]=='#')
							{
								if(prev_hash==1)
								{
									//This is where the first arg ends
									commands[i-1]='\0';
									found=1;
								}
								else
								{
									prev_hash=1;
								}
							}
							else
							{
								prev_hash=0;
							}
						}
						else
						{
							if(commands[i]=='"')
							{
								isCommaOpen=0;
							}	
						}
					}
					separate_cmds[tokens++]=strdup(commands+start_loc);
					//printf("\nString identified:%s",separate_cmds[tokens-1]);
					//commands[i-2]='#';
					start_loc=i;
				}
				separate_cmds[tokens]=NULL;
			}
			else if(mode==1)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(parallel+1+1));//para+1 is no.of cmds,+1 for NULL
				int tokens=0;
				int isCommaOpen=0;
				int prev_and=0;
				int i=0,found=0,start_loc=0;//to iterate
				while(tokens<parallel+1)
				{
					//Find the ## not inside a string
					found=0;
					for(;i<len&&found==0;i++)
					{
						if(isCommaOpen==0)
						{
							if(commands[i]=='"')
							{
								isCommaOpen=1;
							}
							else if(commands[i]=='&')
							{
								if(prev_and==1)
								{
									//This is where the first arg ends
									commands[i-1]='\0';
									found=1;
								}
								else
								{
									prev_and=1;
								}
							}
							else
							{
								prev_and=0;
							}
						}
						else
						{
							if(commands[i]=='"')
							{
								isCommaOpen=0;
							}	
						}
					}
					separate_cmds[tokens++]=strdup(commands+start_loc);
					//printf("\nString identified:%s",separate_cmds[tokens-1]);
					//commands[i-2]='#';
					start_loc=i;
				}
				separate_cmds[tokens]=NULL;
			}
			return separate_cmds;
		}
		return NULL;
	}
	return NULL;
}

int wordcount(char *str)
{
	int words=0,space=1;
	int i=0,len=strlen(str);
	for(;i<len;i++)
	{
		if(space==1)
		{
			if(str[i]!=' ')
			{
				words++;
				space=0;
			}
		}
		else
		{
			if(str[i]==' ')
			{
				space=1;
			}	
		}
	}
	return words;
}

void executeCommand(char **command)
{
	// This function will fork a new process to execute a command
	//We will first tokenise it
	/*int i=0;
	while(i<strlen(command[0])&&command[0][i]==' ')
	{
		i++;
	}*/
	int num_toks=wordcount(command[0]);
	int i=0;
	char **tokens=NULL;
	tokens=(char **)malloc(sizeof(char *)*(num_toks+1));//+1 for NULL termination
	char *temp;
	temp=strdup(command[0]);
	while(temp!=NULL&&temp[0]==' ')
	{
		strsep(&temp," ");
	}
	while(i<num_toks)
	{
		tokens[i++]=strsep(&temp," ");
		//printf("\nToken identifed:%s",tokens[i-1]);
		while(temp!=NULL&&temp[0]==' ')
		{
			strsep(&temp," ");
		}
	}
	tokens[i]=NULL;
	free(command[0]);
	free(command);
	int child_pid=fork();
	if(child_pid==0)
	{
		//child
		execvp(tokens[0],tokens);
	}
	else if(child_pid>0)
	{
		//in parent;
		wait(NULL);
	}
}

void executeParallelCommands()
{
	// This function will run multiple commands in parallel
}

void executeSequentialCommands()
{	
	// This function will run multiple commands in parallel
}

void executeCommandRedirection()
{
	// This function will run a single command with output redirected to an output file specificed by user
}

int main()
{
	// Initial declarations
	
	while(1)	// This loop will keep your shell running until user exits.
	{
		//Ignoring ctrl+c stopping
		signal(SIGINT,SIG_IGN);//Normal behaviour to be resumed in child
		signal(SIGTSTP,SIG_IGN);
		
		char *cwd=NULL,*command=NULL;
		size_t buffer_size=100,cmd_buffer=0;
		cwd=getcwd(cwd,buffer_size);
		// Print the prompt in format - currentWorkingDirectory$
		printf("\n%s$",cwd);
		// accept input with 'getline()'
		getline(&command,&cmd_buffer,stdin);//Remember to delloc the command
		//printf("\n%s",command);
		command=strsep(&command,"\n");//Back '\n' trim
		
		//printf("\n%s",command);
		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		int mode;
		char **split_commands=NULL;//cmds here are not trimmed
		split_commands=parseInput(command,&mode);
		free(command);
		//Check if the first command is exit
		if(mode!=-1)
		{
			int i=0;

			while(i<strlen(split_commands[0])&&split_commands[0][i]==' ')
			{
				i++;
			} 		
			char *exit_check=strdup(split_commands[0]+i);
			exit_check=strsep(&exit_check," ");
			
			if(strcmp(exit_check,"exit")==0)	// When user uses exit command.
			{
				printf("Exiting shell...");
				free(exit_check);
				i=0;
				while(split_commands[i]!=NULL)
				{
					free(split_commands[i]);
					i++;
				}
				free(split_commands);
				break;
			}
		}
		
		
		if(mode==1)
			executeParallelCommands();		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(mode==2)
			executeSequentialCommands();	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(mode==3)
			executeCommandRedirection();	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else if(mode==0)
			executeCommand(split_commands);		// This function is invoked when user wants to run a single commands
			
	}
	
	return 0;
}
