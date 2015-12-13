#include <yaml.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

const int MAX_PROCESSES = 100;
const int BUF_SIZE = 128;

const int P_COMMAND = 1;

struct process {
  char name[BUF_SIZE]; // Identifier
  char cmd[BUF_SIZE];  // The command to be run
};

void spawn_all(int np, struct process *ps[MAX_PROCESSES])
{
  for (np = np; np >= 0; --np)
  {
    printf("Running process %s\n", ps[np]->name);
    int pid = fork();
    if (pid == 0)
    {
      printf("Child process\n");
      int exit_code;
      while (1)
      {
        exit_code = system(ps[np]->cmd);
        printf("Child exited: %d\n", exit_code);
      }
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

  // Open the config file
  fflush(stdout);
  file = fopen(argv[1], "rb");
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
        // printf("%s at depth %d\n", this_buf, block_depth);

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
          }

          if (strcmp(this_buf, "command"))
          {
            strcpy(p->cmd, this_buf);
            this_prop = P_COMMAND;
          }
        }
        break;
      default:
        break;
    }
    yaml_event_delete(&event);
  }

  printf("Name: %s Command: %s\n", ps[0]->name, ps[0]->cmd);
  printf("Name: %s Command: %s\n", ps[1]->name, ps[1]->cmd);

  spawn_all(this_process, ps);

  while (1)
  {
    sleep(1);
  }
}
