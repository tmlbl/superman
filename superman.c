#include <yaml.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <time.h>

const int MAX_PROCESSES = 100;
const int BUF_SIZE = 128;
const int YELLOW = 33;

const int P_COMMAND = 1;
const int P_FILE = 2;

static volatile int keepRunning = 1;
int use_color = 0;

struct process {
  char name[BUF_SIZE]; // Identifier
  char cmd[BUF_SIZE];  // The command to be run
  int  pid;
  char file[BUF_SIZE]; // Path to log files
};

void sm_log(int color, const char *msg, ...)
{
  time_t timer;
  char buffer[26];
  char msg_buf[128];
  struct tm* tm_info;
  va_list ap;
  int i;

  va_start(ap, msg);
  vsnprintf(msg_buf, 100, msg, ap);
  va_end(ap);

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 26, "%Y/%m/%d %H:%M:%S", tm_info);

  if (use_color)
  {
    printf("\x1B[35m[\x1B[36m%s\x1B[35m] \x1B[%dm%s\x1B[0m\n", buffer, color, msg_buf);
  }
  else
  {
    printf("[%s] %s\n", buffer, msg_buf);
  }
}

// Override SIGINT -- kill all children
void intHandler(int dummy)
{
  keepRunning = 0;
}

void kill_all(int np, struct process *ps[MAX_PROCESSES])
{
  for (np = np; np >= 0; --np)
  {
    sm_log(YELLOW, "Killing process %d", ps[np]->pid);
    kill(ps[np]->pid, SIGKILL);
  }
}

// Spawn the processes gathered from config
void spawn_all(int np, struct process *ps[MAX_PROCESSES])
{
  for (np = np; np >= 0; --np)
  {
    int pid = fork();
    if (pid == 0)
    {
      // If log to file is specified, reassign stdout and stderr
      if (strlen(ps[np]->file) > 0)
      {
        char outlog[BUF_SIZE];
        char errlog[BUF_SIZE];
        sprintf(outlog, "%s.log", ps[np]->file);
        sprintf(errlog, "%s.error.log", ps[np]->file);
        FILE *outfile;
        outfile = freopen(outlog, "a", stdout);
        if (outfile == NULL)
        {
          char errmsg[BUF_SIZE];
          sprintf(errmsg, "Error opening %s", outlog);
          perror(errmsg);
        }
        FILE *errfile;
        errfile = freopen(errlog, "a", stderr);
        if (errfile == NULL)
        {
          char errmsg[BUF_SIZE];
          sprintf(errmsg, "Error opening %s", errlog);
          perror(errmsg);
        }
      }
      int exit_code;
      while (1)
      {
        exit_code = system(ps[np]->cmd);
        sm_log(YELLOW, "%s exited %d, restarting...", ps[np]->name, exit_code);
      }
    }
    else
    {
      int p = pid;
      ps[np]->pid = p;
      sm_log(YELLOW, "Spawned \"%s\" with PID %d", ps[np]->name, pid);
    }
  }
}

int main(int argc, char *argv[])
{
  // Processes
  struct process *ps[MAX_PROCESSES];
  struct process *p;
  int this_process = -1;
  int num_processes;
  // YAML
  yaml_parser_t parser;
  yaml_event_t event;
  // Config
  FILE *file;
  int  done_parsing;
  int  block_depth;
  int  this_prop;
  char *this_token;

  // Determine if we are logging to file or TTY
  use_color = isatty(fileno(stdout));

  // Open the config file
  fflush(stdout);
  file = fopen(argv[1], "rb");
  if (file == NULL)
  {
    char errmsg[BUF_SIZE];
    sprintf(errmsg, "Error opening %s", argv[1]);
    perror(errmsg);
    exit(1);
  }
  // Initialize YAML parser
  yaml_parser_initialize(&parser);
  yaml_parser_set_input_file(&parser, file);

  while (!done_parsing)
  {
    if (!yaml_parser_parse(&parser, &event)) {
        break;
    }
    switch (event.type)
    {
      case YAML_STREAM_END_EVENT:
        done_parsing = 1;
        break;
      case YAML_MAPPING_START_EVENT:
        block_depth++;
        break;
      case YAML_MAPPING_END_EVENT:
        block_depth--;
        break;
      case YAML_SCALAR_EVENT:
        ; // Copy the value into a buffer
        char this_buf[BUF_SIZE];
        sprintf(this_buf, "%s", event.data.scalar.value);
        this_token = this_buf;

        // Create a new process
        if (block_depth == 1)
        {
          this_process++;
          p = malloc(sizeof(struct process));
          strcpy(p->name, this_buf);
          ps[this_process] = p;
        }

        if (block_depth == 2)
        {
          if (this_prop == P_COMMAND)
          {
            this_prop = 0;
            strcpy(p->cmd, this_token);
          }
          if (this_prop == P_FILE)
          {
            this_prop = 0;
            strcpy(p->file, this_token);
          }

          if (!strcmp(this_token, "command"))
          {
            this_prop = P_COMMAND;
          }
          if (!strcmp(this_token, "file"))
          {
            this_prop = P_FILE;
          }
        }
        break;
      default:
        break;
    }
    yaml_event_delete(&event);
  }

  // printf("Name: %s Command: %s File: %s\n", ps[0]->name, ps[0]->cmd, ps[0]->file);
  // printf("Name: %s Command: %s\n", ps[1]->name, ps[1]->cmd);

  spawn_all(this_process, ps);

  signal(SIGINT, intHandler);

  while (keepRunning)
  {
    sleep(1);
  }
  kill_all(this_process, ps);
}
