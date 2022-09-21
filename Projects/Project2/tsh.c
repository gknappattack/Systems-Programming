/* 
 *tsh - A tiny shell program with job control
 * 
 *Gregory Knapp gknapp
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*Misc manifest constants */
#define MAXLINE 1024 /*max line size */ 
#define MAXARGS 128 /*max args on a command line */ 
#define MAXJOBS 16 /*max jobs at any point in time */ 
#define MAXJID 1 << 16 /*max job ID */

/*Job states */
#define UNDEF 0 /*undefined */ 
#define FG 1 /*running in foreground */ 
#define BG 2 /*running in background */ 
#define ST 3 /*stopped */

/* 
 *Jobs states: FG (foreground), BG (background), ST (stopped)
 *Job state transitions and enabling actions:
 *    FG -> ST  : ctrl-z
 *    ST -> FG  : fg command
 *    ST -> BG  : bg command
 *    BG -> FG  : fg command
 *At most 1 job can be in the FG state.
 */

/*Global variables */
extern char **environ; /*defined in libc */
char prompt[] = "tsh > "; /*command line prompt (DO NOT CHANGE) */
int verbose = 0; /*if true, print additional output */
int nextjid = 1; /*next job ID to allocate */
char sbuf[MAXLINE]; /*for composing sprintf messages */

struct job_t
{
    /*The job struct */
    pid_t pid; /*job PID */
    pid_t pgid; /*job pgid */
    int jid; /*job ID[1, 2, ...] */
    int state; /*UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /*command line */
};
struct job_t jobs[MAXJOBS]; /*The job list */
/*End global variables */

/*Function prototypes */

/*Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/*Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, pid_t pgid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t* getjobpid(struct job_t *jobs, pid_t pid);
struct job_t* getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t* Signal(int signum, handler_t *handler);

/*
 *main - The shell's main routine 
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /*emit prompt (default) */

    /*Redirect stderr to stdout (so that driver will get all output
     *on the pipe connected to stdout) */
    dup2(1, 2);

    /*Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
            case 'h':
                /*print help message */
                usage();
                break;
            case 'v':
                /*emit additional diagnostic info */
                verbose = 1;
                break;
            case 'p':
                /*don't print a prompt */
                emit_prompt = 0; /*handy for automatic testing */
                break;
            default:
                usage();
        }
    }

    /*Install the signal handlers */

    /*These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler); /*ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /*ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /*Terminated or stopped child */

    /*This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /*Initialize the job list */
    initjobs(jobs);

    /*Execute the shell's read/eval loop */
    while (1)
    {
        /*Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }

        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        {
            /*End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /*Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /*control never reaches here */
}

/* 
 *eval - Evaluate the command line that the user has just typed in
 * 
 *If the user has requested a built-in command (quit, jobs, bg or fg)
 *then execute it immediately. Otherwise, fork a child process and
 *run the job in the context of the child. If the job is running in
 *the foreground, wait for it to terminate and then return.  Note:
 *each child process must have a unique process group ID so that our
 *background children don't receive SIGINT (SIGTSTP) from the kernel
 *when we type ctrl-c (ctrl-z) at the keyboard.  
 */
void eval(char *cmdline)
{
    
    //Create variables to use in eval
    char *argv[MAXARGS];
    int bg;
    int cmds[MAXARGS];
    int stdin_redir[MAXARGS];
    int stdout_redir[MAXARGS];
    int jid;
    pid_t currpid;
    sigset_t mask_all, mask_one, prev_one;

    //Set up masks to block SIGCHLD
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);

    //Call parseline to get arguments and background or foreground process
    bg = parseline(cmdline, argv);

    //Return from eval if command line is blank
    if (argv[0] == NULL)
    {
        return;
    }

    //Call buildtin_cmd, run immeditely if true, else continue with eval
    if (!builtin_cmd(argv))
    {

        //Get number of commands and argument arrays from parseargs
        int numcmds = parseargs(argv, cmds, stdin_redir, stdout_redir);

        //Begin looping through commands
        for (int i = 0; i < numcmds; i++)
        {
            //Block SIGCHLD before forking
            sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

            //Fork and save pid of child to use in parent
            currpid = fork();

            if (currpid == 0)
            {
               	//In child process

                //Unblock SIGCHLD and set process group
                sigprocmask(SIG_SETMASK, &prev_one, NULL);
                setpgrp();

                //Print error message and return if execv fails
                if (execv(argv[cmds[i]], &argv[cmds[i]]) < 0)
                {
                    printf("%s: Command not found\n", argv[0]);
                    return;
                }
            }
            else
            {
               	//In parent process

               	//Block all signals before adding job
                sigprocmask(SIG_BLOCK, &mask_all, NULL);

                //If child process is a background function
                if (bg)
                {
                   	//Add job
                    int jobadded = addjob(jobs, currpid, currpid, BG, cmdline);

                   	//Unblock signals
                    sigprocmask(SIG_SETMASK, &prev_one, NULL);

                    //If job was successfully added, print info of background process
                    if (jobadded)
                    {
                        jid = pid2jid(currpid);
                        printf("[%d] (%d) %s", jid, currpid, cmdline);
                    }

                   	//Prepare to receive next user command, no waitpid()
                }
                //Child is a foreground process
                else
                {
                   	// Add job
                    int jobadded = addjob(jobs, currpid, currpid, FG, cmdline);

                   	//Unblock signals
                    sigprocmask(SIG_SETMASK, &prev_one, NULL);

                   	//Run waitfg if the job was successfully added
                    if (jobadded)
                    {
                        waitfg(currpid);
                    }
                }
            }
        }
    }

    return;
}

/* 
 *parseargs - Parse the arguments to identify pipelined commands
 * 
 *Walk through each of the arguments to find each pipelined command.  If the
 *argument was | (pipe), then the next argument starts the new command on the
 *pipeline.  If the argument was < or>, then the next argument is the file
 *from/to which stdin or stdout should be redirected, respectively.  After it
 *runs, the arrays for cmds, stdin_redir, and stdout_redir all have the same
 *number of items---which is the number of commands in the pipeline.  The cmds
 *array is populated with the indexes of argv corresponding to the start of
 *each command sequence in the pipeline.  For each slot in cmds, there is a
 *corresponding slot in stdin_redir and stdout_redir.  If the slot has a -1,
 *then there is no redirection; if it is >= 0, then the value corresponds to
 *the index in argv that holds the filename associated with the redirection.
 *
 */
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir)
{
    int argindex = 0; /*the index of the current argument in the current cmd */
    int cmdindex = 0; /*the index of the current cmd */

    if (!argv[argindex])
    {
        return 0;
    }

    cmds[cmdindex] = argindex;
    stdin_redir[cmdindex] = -1;
    stdout_redir[cmdindex] = -1;
    argindex++;
    while (argv[argindex])
    {
        if (strcmp(argv[argindex], "<") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            {
                /*if we have reached the end, then break */
                break;
            }

            stdin_redir[cmdindex] = argindex;
        }
        else if (strcmp(argv[argindex], ">") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            {
                /*if we have reached the end, then break */
                break;
            }

            stdout_redir[cmdindex] = argindex;
        }
        else if (strcmp(argv[argindex], "|") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            {
                /*if we have reached the end, then break */
                break;
            }

            cmdindex++;
            cmds[cmdindex] = argindex;
            stdin_redir[cmdindex] = -1;
            stdout_redir[cmdindex] = -1;
        }

        argindex++;
    }

    return cmdindex + 1;
}

/* 
 *parseline - Parse the command line and build the argv array.
 * 
 *Characters enclosed in single quotes are treated as a single
 *argument.  Return true if the user has requested a BG job, false if
 *the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /*holds local copy of command line */
    char *buf = array; /*ptr that traverses command line */
    char *delim; /*points to first space delimiter */
    int argc; /*number of args */
    int bg; /*background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' '; /*replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /*ignore leading spaces */
        buf++;

    /*Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /*ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }

    argv[argc] = NULL;

    if (argc == 0) /*ignore blank line */
        return 1;

    /*should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }

    return bg;
}

/* 
 *builtin_cmd - If the user has typed a built-in command then execute
 *   it immediately.  
 */
int builtin_cmd(char **argv)
{

    //If command is quit, exit(0) from tsh.c
    if (strcmp(argv[0], "quit") == 0)
    {
        exit(0);
    }

    //If command is bg, check input then run do_bgfb()
    if (strcmp(argv[0], "bg") == 0)
    {
    	
        //Check for bg command with no PID/JID argument
        if (argv[1] == NULL)
        {
            printf("%s command requires PID or %cjobid argument\n", argv[0], '%');
        }
        else
        {
            do_bgfg(argv);
        }

        return 1;
    }

    //If command is fg, check input then run do_bgfg()
    if (strcmp(argv[0], "fg") == 0)
    {
    	//Check for fg command with no PID/JID
        if (argv[1] == NULL)
        {
            printf("%s command requires PID or %cjobid argument\n", argv[0], '%');
        }
        else
        {
            do_bgfg(argv);
        }

        return 1;
    }

    //If command is jobs
    if (strcmp(argv[0], "jobs") == 0)
    {
    	//Call listjobs() to print job list to terminal
        listjobs(jobs);
        return 1;
    }

    return 0; /*not a builtin command */
}

/* 
 *do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
   	//Error checking for non numbers
    char *value = argv[1];

    //The input does not have a % sign so it cannot be a JID
    if (strchr(value, '%') == NULL)
    {
    	//First character of second argument is not a number, not a valid PID
        if (!isdigit(value[0]))
        {
            printf("%s: argument must be a PID or %cjobid\n", argv[0], '%');
            return;
        }
    }
    //The input does contain % at the start of the second argument so check for valid JID
    else
    {
        //First character after % is not number, invalid JID
        if (!isdigit(value[1]))
        {
            printf("%s: argument must be a PID or %cjobid\n", argv[0], '%');
            return;
        }
    }

    //If command is fg
    if (strcmp(argv[0], "fg") == 0)
    {
        char *value = argv[1];
        struct job_t * fgjob;

        //If second argument is a JID
        if (strchr(value, '%') != NULL)
        {
        	//Get int form of JID from atoi()
            int jid = atoi(argv[1] + 1);

            //Get job by JID
            fgjob = getjobjid(jobs, jid);

           	//Error checking for non existant job
            if (fgjob == NULL)
            {
                printf("%s: No such job\n", argv[1]);
                return;
            }

            //Set state of job found by JID to FG
            fgjob->state = FG;

            //Send SIGCONT signal to new foreground job
            kill(-1 *fgjob->pid, SIGCONT);
        }
        //Second argument is PID
        else
        {
        	//Get int form of PID from atoi() 
            int fgpid = atoi(argv[1]);

            //Get job by PID of job to make foreground job
            fgjob = getjobpid(jobs, fgpid);

           	//Error checking for non existant process
            if (fgjob == NULL)
            {
                printf("(%d): No such process\n", fgpid);
                return;
            }

            //Set state of job to FG
            fgjob->state = FG;

            //Send SIGCONT signal to new foreground job
            kill(-1 *fgpid, SIGCONT);
        }

        //Call waitfg() to reap new foreground job when finished.
        waitfg(fgjob->pid);
    }
    //Command is bg
    else
    {
        char *value = argv[1];
        struct job_t * bgjob;

        //Second argument is a JID
        if (strchr(value, '%') != NULL)
        {
            //Get int form of JID from atoi()
            int jid = atoi(argv[1] + 1);

            //Get job by JID to set as background job
            bgjob = getjobjid(jobs, jid);

           	//Error checking for non existant job
            if (bgjob == NULL)
            {
                printf("%s: No such job\n", argv[1]);
                return;
            }

           	//Error checking for job that is already in background
            if (bgjob->state == BG)
            {
                printf("[%d] (%d) %s", bgjob->jid, bgjob->pid, bgjob->cmdline);
                return;
            }

            //Set state of job to BG
            bgjob->state = BG;

            //Send SIGCONT signal to new background job
            kill(-1 *bgjob->pid, SIGCONT);

            //Print details of background job to terminal
            printf("[%d] (%d) %s", bgjob->jid, bgjob->pid, bgjob->cmdline);
        }
        //Second argument is PID
        else
        {
        	//Get int form of PID from atoi()
            int bgpid = atoi(argv[1]);

            //Get job by PID
            bgjob = getjobpid(jobs, bgpid);

           	//Error checking for non existant job
            if (bgjob == NULL)
            {
                printf("(%d): No such process\n", bgpid);
                return;
            }

           	//Error checking for process that is already in background
            if (bgjob->state == BG)
            {
                printf("[%d] (%d) %s", bgjob->jid, bgjob->pid, bgjob->cmdline);
                return;
            }

            //Set state of job to BG
            bgjob->state = BG;

            //Send SIGCONT signal to new background job
            kill(-1 *bgpid, SIGCONT);

            //Print details of new background job to terminal
            printf("[%d] (%d) %s", bgjob->jid, bgjob->pid, bgjob->cmdline);
        }
    }

    return;
}

/* 
 *waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{

    //While the current process is the foreground job
    while (fgpid(jobs) == pid)
    {
        //Sleep one to give a delay for the foreground job to run
        sleep(1);
    }

    return;
}

/*****************
 *Signal handlers
 *****************/

/* 
 *sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *    a child job terminates (becomes a zombie), or stops because it
 *    received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *    available zombie children, but doesn't wait for any other
 *    currently running children to terminate.  
 */
void sigchld_handler(int sig)
{
    //Save errno to reset later
    int olderrno = errno;

    //Set up other variables for masking and job list modifiation
    sigset_t mask_all, prev_all;
    pid_t pid;
    struct job_t * job;

    int status;

    //Set mask to block all signals for deleting jobs
    sigfillset(&mask_all);

    //While there are still child processes to be reaped
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        //If the child process was terminated by Ctrl+C signal
        if (WIFSIGNALED(status))
        {
            //Get JID from helper function and print details of terminated process to print
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, WTERMSIG(status));

            //Block all signals before deleting job
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

            //Delete terminated job from job list
            deletejob(jobs, pid);
            
            //Unblock all signals
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
        //If the child process was stopped by Ctrl-Z signal
        else if (WIFSTOPPED(status))
        {
            //Get JID from helper function and print details of stopped process to terminal
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, WSTOPSIG(status));

            //Get job with PID and updated status to ST (stopped)
            job = getjobpid(jobs, pid);
            job->state = ST;

        }
        //The child exited normally by finishing its process
        else if (WIFEXITED(status))
        {
            //Block all signals before deleting jobs
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

            //Delete finished process from job list
            deletejob(jobs, pid);

            //Unblock all signals
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
    }

    //Reset global variable errno in case it changed during signal handling
    errno = olderrno;

    return;
}

/* 
 *sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *   user types ctrl-c at the keyboard.  Catch it and send it along
 *   to the foreground job.  
 */
void sigint_handler(int sig)
{
    //Get PID of current foreground process
    pid_t fgprocessid = fgpid(jobs);

    //Send current foreground process and all members of process group SIGINT signal
    kill(-1 *fgprocessid, SIGINT);

    return;
}

/*
 *sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *    the user types ctrl-z at the keyboard. Catch it and suspend the
 *    foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{
    //Get pid of current foreground process
    pid_t fgprocessid = fgpid(jobs);

    //Send current foregroudn process and all members of process group SIGTSTP signal 
    kill(-1 *fgprocessid, SIGTSTP);

    return;
}

/*********************
 *End signal handlers
 *********************/

/***********************************************
 *Helper routines that manipulate the job list
 **********************************************/

/*clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/*initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/*maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/*addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, pid_t pgid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].pid = pid;
            jobs[i].pgid = pgid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job[%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }

            return 1;
        }
    }

    printf("Tried to create too many jobs\n");
    return 0;
}

/*deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }

    return 0;
}

/*fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/*getjobpid  - Find a job (by PID) on the job list */
struct job_t* getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/*getjobjid  - Find a job (by JID) on the job list */
struct job_t* getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/*pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
        {
            return jobs[i].jid;
        }

    return 0;
}

/*listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
                case BG:
                    printf("Running ");
                    break;
                case FG:
                    printf("Foreground ");
                    break;
                case ST:
                    printf("Stopped ");
                    break;
                default:
                    printf("listjobs: Internal error: job[%d].state=%d ",
                        i, jobs[i].state);
            }

            printf("%s", jobs[i].cmdline);
        }
    }
}

/******************************
 *end job list helper routines
 ******************************/

/***********************
 *Other helper routines
 ***********************/

/*
 *usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell[-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 *unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 *app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 *Signal - wrapper for the sigaction function
 */
handler_t* Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /*block sigs of type being handled */
    action.sa_flags = SA_RESTART; /*restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 *sigquit_handler - The driver program can gracefully terminate the
 *   child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
