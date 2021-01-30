// 
// tsh - A tiny shell program with job control
// 
// <Justin Yara & Asmita Dhakal>
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

//
// main - The shell's main routine 
//
int main(int argc, char *argv[]) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) { //reads command line
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline); //calls the eval function with that command line
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
void eval(char *cmdline) 
{
/* Parse command line */
// routine below. It provides the arguments needed
// for the execve() routine, which you'll need to
// use below to launch a process.
//add a job after i create a process, keep track of pid of process
//keep track of pid of process
//defined array, maximum amount of arguments we can get
// The 'bg' variable is TRUE if the job should run
// in background mode or FALSE if it should run in FG
    
    
char *argv[MAXARGS];    // Argument list execve().
    //defined array, with max number of arguments allowed
int bg = parseline(cmdline, argv);      // Holds modified command line
    //returns an integar on whether or not its a BG(background) job
pid_t pid; //keep track of PID of the process
struct job_t *job; //need the job struct that contains the job item
    
if (!builtin_cmd(argv)) { //if its not a built in command, then fork an exec the 
    //specified program
    
 if((pid = fork()) == 0){ //after doingthis, you are in the child
     setpgid(0,0);
     //in the child now, have to call execvp , 
     
     //execvp takes a path to the command, (the first string inside the parsed argument vector)
     //takes the entire argument vector as second command
     //p means itll look things up in the system path, (easier to type in commands)
     
     //exec shouldnt return if running the new program was successful
     //kills off current program, if it does return, command wasnt found
     if (execvp(argv[0], argv) < 0){ 
         printf("Command not found\n");
         exit(0); //in the child still, so you have to terminate
     }
 }
 //parent process returns from eval , does not wait for child. You MUST
    //wait for the child... unless it is a background process
    
 addjob(jobs, pid, bg?BG:FG, cmdline); //call add job, add the job to jobs, needs the pid, 
    //and it needs the state, use conditional op, if bg is true bg ?, i want a BG or a FG
    
    //reap and delete with sigchldhndler down below
    
    if(!bg){ //if its not a background process
        
        //no longer waiting for FG job to complete, plus we dont want the FG job to be reaped in 2 places
        
        waitfg(pid); //wait for your child, was originally wait(NULL);
        
    }else{ // if it is a BG job, we need to print a status message
        job = getjobpid(jobs, pid);
        printf("[%d] (%d) %s", job->jid, pid, cmdline);
        //%d is the job id, 
    }
    }
   if(argv[0] == NULL) 
       return;

}
//./myspin 1 & means it will run in the bg, for 1 second
/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv) 
{
if (strcmp("quit", argv[0]) == 0) {
     //originally: if (strcmp(argv[0], "quit") == 0) , checking if argv is equal to quit
         //exit(0); for test02
  exit(0);
} else if (strcmp(argv[0], "jobs") == 0) {
    //if any of these return true, then fork a proccess to exect a command
  listjobs(jobs);
  return 1;
} else if ((strcmp(argv[0], "bg") == 0) || (strcmp(argv[0], "fg")) == 0) { 
    //testing foreground and background commands, return 1 bc its a built in command
  do_bgfg(argv);
  return 1;
}

return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) {

  struct job_t *job = NULL;


  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }


  if (isdigit(argv[1][0])) {
    pid_t pid = stoi(argv[1]);
    if (!(job = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  } else if (argv[1][0] == '%') {
    int jid = stoi(&argv[1][1]);
    if (!(job = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  } else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
  string cmd(argv[0]);

  if (cmd == "fg") {

    if (job -> state == ST) {
      kill(-job -> pid, SIGCONT);
    }
    job -> state = FG;
    waitfg(job -> pid);

  } else if (cmd == "bg") {
    kill(-job -> pid, SIGCONT);

    job -> state = BG;
    printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
  }
}
/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{ //if you give it a pid, it will wait until this pid is no longer assosicated with a FG job
    
    struct job_t *job = getjobpid(jobs, pid); //check the data struct we already created for this purpose
    
    while (job->state == FG) { //state equal to foreground state
        sleep(1); //if its true, keep staying around. Or just sleep, which does not hurt perf. 
        //returns immediatley on signal
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
void sigchld_handler(int sig) { //sent a signal when a job terminates,
    //that signal is a sigchild signal,
    //this is where we are notified that there is a terminating child BG or FG, doesnt care
    
  pid_t pid; 
  int stat = -1; //need job status and PID

while ((pid = waitpid(stat, &stat, WNOHANG | WUNTRACED)) > 0) { //reap the child here
    //WNOHANG - not interested in waiting around if there is no one currently stopped

  if ( WIFEXITED(stat) ) {
    deletejob(jobs, pid); //deletes the job
  }

  else if ( WIFSIGNALED(stat) ) {
    printf("Job [%d] (%d) terminated by signal %d \n", pid2jid(pid), pid, WTERMSIG(stat));
    deletejob(jobs, pid);
  } else if ( WIFSTOPPED(stat) ) {
    printf("Job [%d] (%d) stopped by signal %d \n", pid2jid(pid), pid, WSTOPSIG(stat));
    job_t *temp = getjobpid(jobs, pid);
    temp -> state = ST; //temp happens before state
  }
}
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
void sigint_handler(int sig) 
{
  pid_t pid = fgpid(jobs);
  //if theres a FG task, reap it
  if (pid) {
    kill(-pid, SIGINT);
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig) 
{
  pid_t pid = fgpid(jobs);

  if (pid != 0){ //if there was a FG task then pause it 
    kill(-pid, SIGTSTP);
  }


  return;
}

/*********************
 * End signal handlers
 *********************/


