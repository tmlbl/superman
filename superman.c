#include <yaml.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

const int MAX_PROCESSES = 100;

struct process {
  char *cmd; // The command to be run
}

void print_with_color(int color, char *msg)
{
  time_t timer;
  char buffer[26];
  struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 26, "%Y/%m/%d %H:%M:%S", tm_info);

  if (isatty(fileno(stdout)))
  {
    printf("\x1B[35m[\x1B[36m%s\x1B[35m] \x1B[%dm%s\x1B[0m\n", buffer, color, msg);
  }
  else
  {
    printf("[%s] %s\n", buffer, msg);
  }
}

int main(int argc, char *argv[])
{
    int number;

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
                printf("Mapping start %d\n", block_depth);
                break;
              case YAML_MAPPING_END_EVENT:
                block_depth--;
                printf("Depth %d\n", block_depth);
                break;
              case YAML_SCALAR_EVENT:
                printf("Value: %s\n", event.data.scalar.value);
                break;
              default:
                printf("Event type: %d\n", event.type);
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
