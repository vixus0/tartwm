//
// tartwm.c
//

#include <error.h>  // For error_t
#include <fcntl.h>  // For open() flags O_*
#include <signal.h> // For catching SIGINT, SIGTERM
#include <unistd.h> // For size_t, access()
#include <string.h> // For strtok
#include <argp.h>   // For better argument parsing
#include <stdbool.h> // For booleans
#include <stdint.h>  // For clearer integer types
#include <stdlib.h>  // For getenv, atoi, etc.
#include <stdio.h>
#include <sys/stat.h>  // For file mode constants S_*
#include <xcb/xcb.h>


// -- Handy structs
struct arguments
{
  char * command;
  char ** command_args;
  char * config_file;
  bool is_host;
};

struct config 
{
  uint16_t x, y, g, t, b, l, r, bw;
  uint32_t cf, cu, ci;
};

struct rectangle {
  uint16_t x, y, w, h;
};



// -- Prototypes
void handle_sig (int32_t signal);
void parse_config (struct config * cfg, char * path);
uint8_t split_line (char * line, const char * delim, char ** tokens);
bool assign_uint16 (const char * match, char * key, char * val, uint16_t * dest);
bool assign_uint32 (const char * match, char * key, char * val, uint32_t * dest);


// -- Used for SIGINT and SIGTERM cleanup
bool run;


// -- Argument processing
const char * argp_program_version = "1";
const char * argp_program_bug_address = "<vixus0@lilhom.co.uk>";
static char doc[] = "TartWM -- A tasty grid-based, floating window manager.";
static char args_doc[] = "[command [command arguments]]";
static struct argp_option options[] = {
  { "config", 'c', "file", 0, "Configuration file" },
  { 0 }
  };

static error_t 
parse_opt (int key, char * arg, struct argp_state * state)
{
  struct arguments * args = state->input;

  switch (key)
  {
    case 'c':
      args->config_file = arg;
      printf("args->config_file: %s\n", arg);
      break;

    case ARGP_KEY_NO_ARGS:
      // No commands means we're a host
      args->is_host = true;
      printf("args->is_host: true\n");
      break;

    case ARGP_KEY_ARG:
      // We're a client sending a command
      args->command = arg;
      // Put all remaining args in commands
      args->command_args = &state->argv[state->next];
      // Don't process any more args
      state->next = state->argc;
      printf("args->command: %s\n", arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


// -- Implementations
int32_t
main (int32_t argc, char * argv[])
{
  char * display = getenv("DISPLAY");

  if ( display == NULL )
  {
    fputs("DISPLAY not set.\n", stderr);
    exit(EXIT_FAILURE);
  }

  char fifo_path[128];

  snprintf(fifo_path, 128, "/tmp/tartwm-%s.fifo", &display[1]);

  struct arguments args = { .is_host = false, .config_file = NULL };

  argp_parse(&argp, argc, argv, 0, 0, &args);

  if ( args.is_host )
  {
    // Default config
    struct config cfg = {
      .x = 6, .y = 5, .g = 8, .t = 0, .b = 0, .l = 0, .r = 0, .bw = 2,
      .cf = 0xff00ccff,
      .cu = 0xff808080,
      .ci = 0xffcc0000
      };

    if ( args.config_file != NULL )
    {
      printf("Parsing config: %s\n", args.config_file);
      parse_config(&cfg, args.config_file);
    }

    // We are the host
    printf("Creating FIFO: %s\n", fifo_path);

    if ( mkfifo(fifo_path, S_IRWXU | S_IRWXG) < 0 ) 
    {
      fputs("Failed to create FIFO.\n", stderr);
      exit(EXIT_FAILURE);
    }

    printf("Opening FIFO.\n");
    int32_t fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    FILE * fifo_read = fdopen(fifo_fd, "r");

    if ( fifo_read == NULL ) 
    {
      fputs("Failed to open FIFO for reading.\n", stderr);
      exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    char buf[256];

    printf("Running.\n");
    run = true;

    while ( run ) 
    {
      if ( fgets(buf, sizeof(buf), fifo_read) != NULL ) 
        printf("read: %s\n", buf);
      fflush(stdout);
    }

    fclose(fifo_read);
    close(fifo_fd);
    
    remove(fifo_path);
  }
  else
  {
    // We are a client
    printf("Opening FIFO for writing.\n");
    FILE * fifo_write = fopen(fifo_path, "w");

    if ( fifo_write == NULL )
    {
      fputs("Can't contact TartWM.\n", stderr);
      exit(EXIT_FAILURE);
    }

    printf("Writing to fifo: %s\n", args.command);
    fputs(args.command, fifo_write); 
    fclose(fifo_write);
  }

  return EXIT_SUCCESS;
}


void 
handle_sig (int32_t sig)
{
  run = false;
}


void
parse_config (struct config * c, char * path)
{
  FILE * config_file = fopen(path, "r");

  if ( config_file == NULL )
  {
    fprintf(stderr, "Could not open file: %s\n", path);
    return;
  }

  char buf[256];
  char * pos[32];
  uint8_t ntok;

  while ( fgets(buf, sizeof(buf), config_file) != NULL )
  {
    ntok = split_line(buf, " \t", pos);

    if ( ntok > 1 )
    {
      if ( assign_uint16("x", pos[0], pos[1], &c->x) );
      else if ( assign_uint16("y", pos[0], pos[1], &c->y) ); 
      else if ( assign_uint16("gap", pos[0], pos[1], &c->g) ); 
      else if ( assign_uint16("top", pos[0], pos[1], &c->t) ); 
      else if ( assign_uint16("bottom", pos[0], pos[1], &c->b) ); 
      else if ( assign_uint16("left", pos[0], pos[1], &c->l) ); 
      else if ( assign_uint16("right", pos[0], pos[1], &c->r) ); 
      else if ( assign_uint16("bw", pos[0], pos[1], &c->bw) ); 
      else if ( assign_uint32("cf", pos[0], pos[1], &c->cf) ); 
      else if ( assign_uint32("cu", pos[0], pos[1], &c->cu) ); 
      else if ( assign_uint32("ci", pos[0], pos[1], &c->ci) ); 
    }
  }

  fclose(config_file);
}


uint8_t
split_line (char * line, const char * delim, char * pos[])
{
  // Get rid of any newline
  char * npos;

  if ( (npos = strchr(line, '\n')) != NULL )
    *npos = '\0';

  size_t count = 0;
  size_t pos_size = sizeof(pos);
  char * tok = strtok(line, delim);

  while ( tok != NULL ) 
  {
    if ( count < pos_size )
      pos[count] = tok;
    else
      break;

    count++;
    tok = strtok(NULL, delim);
  }

  return count;
}


bool
assign_uint16 (const char * match, char * key, char * val, uint16_t * dest)
{
  if ( strcmp(match, key) == 0 )
  {
    uint16_t a = abs(atoi(val)); 
    *dest = (a > 0)? a:0;
    return true;
  }

  return false;
}


bool 
assign_uint32 (const char * match, char * key, char * val, uint32_t * dest)
{
  if ( strcmp(match, key) == 0 )
  {
    *dest = strtol(val, NULL, 16);
    return true;
  }

  return false;
}

