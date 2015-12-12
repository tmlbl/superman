#include <yaml.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

const int MAX_PROCESSES = 100;
int use_color = 0;

const int S_COMMAND = 2;

struct process {
  char* name;
  char* cmd; // The command to be run
};

void print_with_color(int color, const char *msg, ...)
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

void spawn_all(struct process processes[])
{
  int i;
  int done;
  while (!done)
  {
    print_with_color(33, "Spawning process: %s Command: %s", processes[i].name, processes[i].cmd);
    i++;
    done = (processes[i].cmd == NULL);
  }
}

int main(int argc, char *argv[])
{
    int number;
    use_color = isatty(fileno(stdout));

    print_with_color(33, "Started superman");

    if (argc < 2) {
        printf("Usage: %s file1.yaml ...\n", argv[0]);
        return 0;
    }

    for (number = 1; number < argc; number ++)
    {
        FILE *file;
        yaml_parser_t parser;
        yaml_event_t event;
        char msgbuf[128];
        struct process processes[MAX_PROCESSES];
        int done = 0;
        int count = 0;
        int error = 0;

        sprintf(msgbuf, "Loading config from '%s'", argv[number]);
        print_with_color(33, msgbuf);
        fflush(stdout);

        file = fopen(argv[number], "rb");
        assert(file);

        assert(yaml_parser_initialize(&parser));

        yaml_parser_set_input_file(&parser, file);

        int block_depth = 0;
        int last_tag;
        int cur_process = -1;
        // char buf[128];

        while (!done)
        {
            char buf[128];

            if (!yaml_parser_parse(&parser, &event)) {
                error = 1;
                break;
            }

            switch (event.type)
            {
              case YAML_MAPPING_START_EVENT:
                block_depth++;
                break;
              case YAML_MAPPING_END_EVENT:
                block_depth--;
                break;
              case YAML_SCALAR_EVENT:
                // Store value in the buffer
                // sprintf(buf, "%s", event.data.scalar.value);
                // printf("Buf: %s\n", buf);
                printf("");
                char this_buf[128];
                sprintf(this_buf, "%s", event.data.scalar.value);

                if (block_depth == 1)
                {
                  cur_process++;
                  // strcpy(this_buf, buf);
                  // printf("Creating process %s\n", this_buf);
                  char name_buf[128];
                  strcpy(name_buf, this_buf);
                  processes[cur_process].name = name_buf;
                }

                if (strcmp(this_buf, "command")) {
                  // printf("COMMAND TAG\n");
                  char cmd_buf[128];
                  strcpy(cmd_buf, this_buf);
                  processes[cur_process].cmd = cmd_buf;
                }

                printf("Process: %s Command: %s\n", processes[cur_process].name,
                    processes[cur_process].cmd);
                break;
              default:
                // printf("Event type: %d\n", event.type);
                break;
            }

            done = (event.type == YAML_STREAM_END_EVENT);

            yaml_event_delete(&event);

            count ++;
        }

        yaml_parser_delete(&parser);

        assert(!fclose(file));

        spawn_all(processes);
    }

    return 0;
}
