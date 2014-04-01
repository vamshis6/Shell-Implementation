#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include<errno.h>
#include<iostream>
#include<vector>
#include<fstream>
#include<string>
#include<iterator>
#include<signal.h>
#include<libgen.h>
#include<unistd.h>

using namespace std;

#define SIZE=1000;
#define MAX_PIPE 512
#define MAX_VAR_NUM 10
char TERMINAL[512];
int opt_k=0;

int heredoc=0;
int append=0;

fstream histry_file;
vector<string> hist;
vector<int> bgpidlist; 
int fd,fd2;
//For ECHO..!!
// For environment variables..!
/*
HashMap<string,string> map;
map.put("SHELL","sin_shell");
*/
// for checking Background and Foreground Process..

// for environment Variables i use vector only..


fstream env;
vector<string> en;


int fg=0;
int bg=0;


// For backgroung Process

// This Variables wil be Used in Genric Parsing of a single Command.
// used to store commands
char *argv[100];

//will be used for testing redirection
int in_redir=0;
int ou_redir=0;

// will store the name of files if redirection specified
// Note:: Check in out_filename if first character is > use append or else just open..!!
char *out_filename;
char *inp_filename;

// Number of Arguments
int count=0;

// For History Purposes..!!

int x=0;
void  parse(char *input)
{
	//printf("Here\n");	
	int length;
	char * i_redir=strchr(input,'<');
	if(i_redir!=NULL)
		in_redir=1;
	i_redir=NULL;
	char * o_redir=strchr(input,'>');
	if(o_redir!=NULL)
		ou_redir=1;
	o_redir=NULL;		
	//int length;
	//length=strlen(input);
	char *c=(char *)malloc(1000*sizeof(char));
	c=strtok(input," ");
	//printf("Here in parser\n");
	if(c!=NULL)
	{
		argv[0]=c;
		count=1;
	}	
	while(c!=NULL)
	{
		c=strtok(NULL," ");
		//printf("Here in parse 2\n");		
		if(c!=NULL)
		{	
				
			char * i_red=strchr(c,'<');
			char * o_red=strchr(c,'>');			
			int n=strlen(c);
			//printf("Issue Here AGain\n");
			// It will check for the < command > command
			if(n==1 && (i_red!=NULL || o_red!=NULL))
			{
				c=strtok(NULL," ");
				if(c!=NULL)
				{
					if(i_red!=NULL)
					{
						inp_filename=c;
					}else
					{
						out_filename=c;
					}	
				}else
				{
					printf("bash: syntax error near unexpected token `newline\n");
					return ;				
				}
			}else
			{			
				if(i_red==NULL && o_red==NULL) 
				{
					argv[count]=c;
					count++;			
				}
				else if(i_red!=NULL)
				{
					c++;
					inp_filename=c;
					puts(inp_filename);
				}else
				{
					c++;
					out_filename=c;
					puts(out_filename);
				}
			}
		}
	}
	argv[count]=NULL;
	count++;
	//printf("parsing done\n");
	
}



struct commandType
{
	char *command;
	char *argv[30];
	char *input_filename;
	char *output_filename;
	int input_redir;
	int output_redir;
	int k; // no of commands
}co[100];

int i=0;




// k is a variable which contais the list of variables.
//command contains all the commands enterd by user
//argv contains options
// This wil be called only for Pipelining..!

void parse_input(char * input)
{
	int length;
	char *saveptr1,*saveptr2;	
	length=strlen(input);
	char *c=strtok_r(input,"|",&saveptr1);
	char *d=(char *) malloc(1000*sizeof(char));	
	while(c!=NULL)
	{
		char * i_redir=strchr(c,'<');
		if(i_redir!=NULL)
			co[i].input_redir=1;
		//printf("Here for one %d\n",co[i].input_redir);		
		char * o_redir=strchr(c,'>');
		if(o_redir!=NULL)
			co[i].output_redir=1;		
		//printf("Here for one %d\n",co[i].output_redir);
		d=strtok_r(c," ",&saveptr2);
		if(d!=NULL)
		{	
			co[i].command=d;
			co[i].argv[0]=d;
			co[i].k=1;
		}
		//printf("Going In\n");
		while(d!=NULL)
		{
			d=strtok_r(NULL," ",&saveptr2);
			if(d!=NULL)
			{	
				//puts(d);
				char * i_red=strchr(d,'<');
				char * o_red=strchr(d,'>');			
				if(i_red==NULL && o_red==NULL) 
				{
					//puts("Here\n");					
					co[i].argv[co[i].k]=d;
					co[i].k++;			
				}
				else if(i_red!=NULL)
				{
					d++;					
					co[i].input_filename=d;
					
				}else
				{
					d++;
					co[i].output_filename=d;
	
				}
			}
		}
		
		co[i].argv[co[i].k]=NULL;
		co[i].k++;
		c=strtok_r(NULL,"|",&saveptr1);
		i++;	
	}
} 

void print()
{

	int k;
	int t;
	printf("printing here\n");
	for(k=0;k<i;k++)
	{
		printf("Command is %s\n",(co+k)->command);
		for(t=0;t<co[k].k;t++)
			printf("options are %s \n",co[k].argv[t],co[k].input_filename,co[k].output_filename);
	}


}

void c_pwd()
{
	char *curdir=get_current_dir_name();
 	puts(curdir);
}


//Parser works fine for < > and i need to manipulate the Pipelining stuff for this !!!
// Big Job dude..!!
void pipelining()
{
	int pid_child;
   	int fds[2][2];
	int t; 
	int status;
	int k;
	for(k=0;k<i-1;k++)
   	{                                                           
		if (pipe(fds[k%2]) == -1)
         		perror("Failed to create pipes");
      		else if ((pid_child = fork()) == -1) 
         		perror("Failed to create process to run command");
      		else if (pid_child != 0) 
      		{                                                        
			waitpid(pid_child,&status,0);
			if(close(fds[k%2][1])==-1)
           			perror("Failed to close pipes");
			
		}else
		{	
			if(k==0)
			{			
				if (dup2(fds[k%2][1], STDOUT_FILENO) == -1)
            				printf("Failed to connect pipe to stdout");
         			if (close(fds[k%2][1])==-1)
           				perror("Failed to close pipes");
				if((execvp(co[k].argv[0],&(co[k].argv[0])))<0)
						perror("Failed hEre 3rd\n");
							
			}else
			{
				if (dup2(fds[(k+1)%2][0], STDIN_FILENO) == -1)                    
         				perror("Failed to connect last component");
				if(dup2(fds[(k)%2][1],STDOUT_FILENO)==-1)
					perror("Failed ");      				
				//if ((close(fds[(k)%2][1])==-1) && (close(fds[(k+1)%2])==-1))
         			//	perror("Failed to do final close");
				if((execvp(co[k].argv[0],&(co[k].argv[0])))<0)
						perror("Failed Here 2nd !\n");
				
			}
      		
		}
	
	}
	if( (pid_child=fork())== -1 )
		perror("Couldnt fork()\n");
	if(pid_child==0)
	{	
		if (dup2(fds[(k+1)%2][0],STDIN_FILENO) == -1)                    
         		perror("Failed to connect last component 1");
		if((execvp(co[k].argv[0],&(co[k].argv[0])))<0)
			perror("Failed Here !!!\n");
  	}
	if(pid_child!=0)
	{	
		waitpid(pid_child,&status,0);
	}
	
	return ;
}



void do_redirect()
{
	if(in_redir==1)
	{
		fd=open(inp_filename,O_RDONLY);
				
		if(fd<0)
		{
			perror("Error in Opening Input file\n");
			//exit(1);			
			return ;
			
		}
	}
	if(in_redir==1)
	{
		in_redir=0;		
		if (dup2(fd, STDIN_FILENO) == -1)                    
        	 	perror("Couldnt Connect STDOUT TO FD");
		if(ou_redir==0)
		{
			//printf("am i here \n");		
			close(fd);				
			if((execvp(argv[0],argv))<0)
			{
				perror("Issue here");		
								
			}	
			//exit(1);
		}
		else
		{
			ou_redir=0;
			fd2=open(out_filename,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP);
			if(fd2<0)
			{	
				perror("Error in Opening Input file\n");
				return ;
			}
			
			if (dup2(STDOUT_FILENO,fd2) == -1)                    
        	 		perror("Couldnt Connect STDOUT TO FD");
			close(fd2);			
			if((execvp(argv[0],argv))<0)
			{
				perror("Issue here");		
			}
			close(fd2);		
		}
	}else 
	{
		ou_redir=0;		
		fd2=open(out_filename,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP);
		if(fd2<0)
		{	
			perror("Error in Opening Input file\n");
			return ;
		}
		if (dup2(fd2, STDOUT_FILENO) == -1)                    
        	 	perror("Couldnt Connect STDOUT TO FD");
							
		//printf("Here for o/p");	
		close(fd2);
		
		if((execvp(argv[0],argv))<0)
		{
			perror("Issue here");		
							
		}
		//return 
		close(fd2) ;
		//close(STDOUT_FILENO);
		
	}
	
}


// CV CODE




// check for input redirection and output redirection..!!
void execute_command()
{
	
	int pid_child;
	int status;
	if ((pid_child = fork()) == -1) 
         		perror("Failed to create process to run command");		
	if(pid_child==0)
	{
		if(in_redir==1 || ou_redir==1)
		{
			do_redirect();	
		}		
		else
		{	
		 	if((execvp(argv[0],argv))<0)
				perror("Failed Here !!!\n");
		}
		//exit(1);
	}
	else if(pid_child!=0)
	{
		if(bg==1)		
		{	
			waitpid(pid_child,&status,0);
			//cout<<bgpidlist.front();	
			//exit(1);		
			//cout<<"  Job Terminated\n";
			bgpidlist.pop_back();
		}else
		{	
						
			waitpid(pid_child,&status,0);	
			//printf("Parent has to wait\n");
			//exit(1);
			
		}
	}

}


//D WORKS PERFECT...!!
// Check with Others For other Options..!!
void do_cd()
{
	
	
	//printf("am i here\n");	
	if(argv[1]==NULL)
	{
		chdir(getenv("HOME"));
		return ;
	}else
	{
		char *forcd=(char *)malloc(strlen(argv[1])*sizeof(char));		
		strcpy(forcd,argv[1]);
		if(*forcd=='~')
		{	
			chdir(getenv("HOME"));
			return;
		}else if(*forcd=='-')
		{
			chdir("../");
			printf("%s\n",get_current_dir_name());
			return;
		}
		{	
			chdir(forcd);
			return;
		}

	}
}

void signalHandler( int signum )
{
    	//cout<<"\nHandling Control-C"<<endl;
	cout<<"Clean all the Background Jobs and ETC"<<endl;
	exit(signum);
	//return ;  
}


char* getnExecExclain(char *input)
{
	// Search in the Iterator for the Information about the Last executed command
	vector<string>::reverse_iterator it;
	int n=strlen(input);
	//printf("%d",n);
	for(it=hist.rbegin();it!=hist.rend();++it)
        {
		
		if(strncmp(input,(*it).c_str(),n)==0)
        	{
			//puts((*it).c_str());          		
			strcpy(input,(*it).c_str());
			break;
         	}
	}
	return input;
}

int  main()
{
	char *input=(char *) malloc(1000 * sizeof(char));
	// Opening this for Append and Reading in Input Mode..!!	
	//char *cmd=(char *) malloc(1000 * sizeof(char));	
	string cmd;
	histry_file.open("history", fstream::app|fstream::in);	
	while(getline(histry_file,cmd))                            
    		hist.push_back(cmd);
  	histry_file.close();
	//en.push_back("SHELL=sin_shell");
	env.open(".sinshelrc",fstream::app|fstream::in);
	while(getline(env,cmd))
		en.push_back(cmd);
	env.close();	
	char *exclaim;
	signal (SIGINT,signalHandler);
	while(1)
	{
				
		char *now;
		in_redir==0;
		ou_redir==0;    		
		now=get_current_dir_name();
    		strcpy(TERMINAL,"S!N_$HEll::");
   		strcat(TERMINAL,now);
    		strcat(TERMINAL,"->");
		if (fputs(TERMINAL,stdout)==EOF)
      			continue;		
		gets(input); 
		if(input==NULL) 
		{
      			printf("Unable to read command\n");
      			continue;
    		}
		while(*input==' ')
			input++;
		//puts(input);
		// handling for History..!!		
		hist.push_back(input);    
    		histry_file.open("history", fstream::app|fstream::out);    //History File
    		histry_file<<input<<endl;
    		histry_file.close();
		// Search If the Command needs to be executed in the Background..!!
		// If so, Run the Parent and Child common..!		
		char *backgr=strchr(input,'&');
		if(backgr!=NULL)		
		{
			int k=strlen(input);
			input[k-1]='\0';
			bg=1;
			//puts(input);
			//continue;	
		
		}
		//Create a child Process of the Parent shell and manipulate on that..!!
		// if not ok..
		// THis code is added for Background Process and other stuff..		
		pid_t childpid;		
		if(bg==1)
		{			
			 if ((childpid = fork()) == -1)
      				perror("Failed to fork child");
			 if(setpgid(0,0)==-1)                    //If background process, put the child in its own group
			        return 1;
			 //cout<<" Iam Running in background"<<childpid<<endl;
      			 bgpidlist.push_back(childpid);
			 //cout<<"List of bg procs"<<endl;
			 copy(bgpidlist.begin(),bgpidlist.end(),ostream_iterator<int>(cout,"\n"));
      			 while(waitpid(-1,NULL,WNOHANG)>0);		
		} // If parent Please Execute the Next command from First..!!
    		if(bg==1 && childpid!=0 )
      		{	
			bg=0;
			printf("Parent here\n");			
			continue;			
			//waitpid(childpid,NULL,0);             //Wait for foreground process
    		
		}
		// fg % process num..Id doesnt take any ID..!
		if(strncmp(input,"fg %",4)==0)
    		{
			//printf("%s",(input+5));      			
			if(bgpidlist.size()>=atoi(&input[5]))
     			 {
       				 //cout<<"foregrounding pid "<<bgpidlist[atoi(&inbuf[5])];
       				 waitpid(bgpidlist[atoi(&input[5])],NULL,0);
      			}
      			else
        			cout<<"error: Couldn't fg the requested process.";
			
			continue;	
		}
		//printf("Here\n");		
		// Coming Here for Excalim !!
		exclaim=strchr(input,'!');
		if(exclaim!=NULL)
		{
			//printf("EXcalim Command\n");
			char *k=(char*)malloc(1000*sizeof(char));			
			input++;
			while(*input==' ')
				input++;
			k=input;
			input=getnExecExclain(k);
			puts(input);
			//continue;			
						
		}	
		//printf("Again Here\n");	
		if(strcmp(input,"pwd")==0)
		{	
			c_pwd();
			continue;
		}
		
		if(strcmp(input,"history")==0)
		{
			// Use Iterator to print down the History..!!
			int t=1;			
			for(vector<string>::iterator i = hist.begin(); i != hist.end(); ++i)
    			{
        			cout<<t++<<" "<<*i<<endl;
    			}
			continue;
		}
		
		// Even the pipes may contain redirection please take care of this..!!
		// checks in the Input if the pipe do present..!!
		if (strcmp(input, "exit") == 0)
    		{
      			if(bgpidlist.size()>0)
      			{
        			for(int i=0;i<bgpidlist.size();i++)
        			{
          				kill(bgpidlist[i],SIGKILL);
        			}
      			}
      			break;
    		}	
			
		char* found_pipe=strchr(input,'|');
		if(found_pipe!=NULL)		
		{
			//printf("Doing SOmething\n");			
			parse_input(input);
			pipelining();
			continue ;	
		}else
		{	
			//printf("parsing in general\n");
			parse(input);
		}
		// Since the Input is Parsed i got for cd and other stuff..!!
		// Export and Echo Must come here !!
		char *equals;
		equals=strchr(argv[0],'=');
		if(strcmp(argv[0],"cd")==0)
		{	
			//printf("CD Ing\n");			
			do_cd();
			continue;
		}
		else if(strcmp(argv[0],"echo")==0)
		{
			int lo;
			char *s;
			char *temp;
			for(lo=1;lo<count-1;lo++)
			{
				temp=argv[lo];				
				s=strchr(temp,'$');
				if(s!=NULL)
				{
					s++;
					vector<string>::iterator it;
					//int n=strlen(input);
					//printf("%d",n);
					int flag=0;
					for(it=en.begin();it!=en.end();it++)
        				{
						int n=strlen(s);							
						if(strncmp(s,(*it).c_str(),n)==0)
        					{
							char txt[100];
              						strcpy(txt,(*it).c_str());
              						strtok(txt,"=");
              						cout<<strtok(NULL,";");
							flag=1;							
							break;
         					}
					}
					if(!flag)
					{	
							cout<<getenv(s);
					}
				
				}else
				{
					printf("%s ",argv[lo]);				

				}
						
		
			}
			printf("\n");
			continue;
		}
		else if(strcmp(argv[0],"export")==0)
		{
			//puts(input);
			if(count==3)
			{
				char *k=argv[1];
				char * semi=strchr(k,';');
				if(semi!=NULL)
				{				
					char *s1=(char *)malloc(100*sizeof(char));				
					char *s2=(char *)malloc(100*sizeof(char));				
					s1=strtok(k,"=");
					s2=strtok(NULL,";");
					//puts(s1);
					//puts(s2);					
					if(setenv(s1,s2,1)==-1)
					{
						printf("couldnt set the Environment variable for %s\n",s1);
						continue;
					}
				}else
				{
					char *s1=(char *)malloc(100*sizeof(char));				
					char *s2=(char *)malloc(100*sizeof(char));				
					s1=strtok(k,"=");
					s2=strtok(NULL,"=");
					//puts(s1);
					//puts(s2);
					if(setenv(s1,s2,1)==-1)
					{
						printf("couldnt set the Environment variable for %s\n",s1);
						continue;
					}
				}				
			}else
			{
				printf("Usage for Export ::\n");
				printf("export env_variable=value\n");
				continue;
			}
			continue;
		}		
		else if(equals!=NULL)
		{
			en.push_back(argv[0]);
			continue;				
		}else
		{
			//printf("here to execute command\n");
			printf("%d %d",in_redir,ou_redir);
			execute_command();
			
		}
	}
	return 0;
}
