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
  char *cmd; // The command to be run
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

        while (!done)
        {
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
                if (block_depth == 1)
                {
                  print_with_color(33, "Creating process: %s", event.data.scalar.value);
                }

                // Store the value in a buffer
                char buf[128];
                sprintf(buf, "%s", event.data.scalar.value);

                // Based on last tag
                if (last_tag == S_COMMAND)
                {
                  printf("Command Value: %s\n", buf);
                  system(buf);
                }

                if (buf[0] == 'c') last_tag = S_COMMAND;
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

        printf("%s (%d events)\n", (error ? "FAILURE" : "SUCCESS"), count);
    }

    return 0;
}
