//
// tartwm.c
//

#include <error.h>  // For error_t
#include <fcntl.h>  // For open() flags O_*
#include <signal.h> // For catching SIGINT, SIGTERM
#include <unistd.h> // For getopt()
#include <string.h> // For strtok
#include <stdbool.h> // For booleans
#include <stdint.h>  // For clearer integer types
#include <stdlib.h>  // For getenv, atoi, etc.
#include <stdio.h>
#include <sys/stat.h>  // For file mode constants S_*
#include <xcb/xcb.h>

#include "args.h"

#define BUFSIZE 255
#define MAXSPLIT 32


// -- Handy structs
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
size_t split_line (char * line, const char * delim, char * tokens[], size_t tok_size);
void join_line(char * tokens[], size_t ntok, char * buf, size_t buf_size, const char * delim);
bool assign_uint16 (const char * match, char * key, char * val, uint16_t * dest);
bool assign_uint32 (const char * match, char * key, char * val, uint32_t * dest);


// -- Some vars
static bool run;


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

  struct main_args args = { .is_host = false, .config_file = NULL };
  parse_main_args(argc, argv, &args);

  char fifo_path[BUFSIZE];
  snprintf(fifo_path, BUFSIZE, "/tmp/tartwm-%s.fifo", &display[1]);

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

    char buf[BUFSIZE];

    printf("Running.\n");
    run = true;

    while ( run ) 
    {
      if ( fgets(buf, BUFSIZE, fifo_read) != NULL ) 
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
    int32_t fifo_fd;

    if ( (fifo_fd = open(fifo_path, O_NONBLOCK | O_WRONLY)) == -1 )
    {
      fputs("Is TartWM running?\n", stderr);
      exit(EXIT_FAILURE);
    }

    FILE * fifo_write = fdopen(fifo_fd, "w");

    if ( fifo_write == NULL )
    {
      fputs("Can't contact TartWM.\n", stderr);
      exit(EXIT_FAILURE);
    }

    if ( args.ncommand == 1 )
    {
      fputs(args.command[0], fifo_write); 
    }
    else
    {
      size_t i;
      for ( i=0; i<args.ncommand; i++ )
      {
        fputs(args.command[i], fifo_write);
        fputs(" ", fifo_write);
      }
    }

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

  char buf[BUFSIZE];
  char * tok[MAXSPLIT];
  size_t ntok;

  while ( fgets(buf, BUFSIZE, config_file) != NULL )
  {
    ntok = split_line(buf, " \t", tok, MAXSPLIT);

    if ( ntok > 1 )
    {
      if ( assign_uint16("x", tok[0], tok[1], &c->x) );
      else if ( assign_uint16("y", tok[0], tok[1], &c->y) ); 
      else if ( assign_uint16("gap", tok[0], tok[1], &c->g) ); 
      else if ( assign_uint16("top", tok[0], tok[1], &c->t) ); 
      else if ( assign_uint16("bottom", tok[0], tok[1], &c->b) ); 
      else if ( assign_uint16("left", tok[0], tok[1], &c->l) ); 
      else if ( assign_uint16("right", tok[0], tok[1], &c->r) ); 
      else if ( assign_uint16("bw", tok[0], tok[1], &c->bw) ); 
      else if ( assign_uint32("cf", tok[0], tok[1], &c->cf) ); 
      else if ( assign_uint32("cu", tok[0], tok[1], &c->cu) ); 
      else if ( assign_uint32("ci", tok[0], tok[1], &c->ci) ); 
    }
  }

  fclose(config_file);
}


size_t
split_line (char * line, const char * delim, char * tokens[], size_t tok_size)
{
  // Get rid of any newline
  char * ntok;

  if ( (ntok = strchr(line, '\n')) != NULL )
    *ntok = '\0';

  size_t count = 0;
  char * tok = strtok(line, delim);

  while ( tok != NULL ) 
  {
    if ( count < tok_size )
      tokens[count] = tok;
    else
      break;

    count++;
    tok = strtok(NULL, delim);
  }

  return count;
}


void
join_line ( char * tokens[], size_t ntok, char * buf, size_t buf_size, const char * delim)
{
  size_t t=0, len=0, dlen=strlen(delim);
  char * str = buf;

  printf("Join line\n");

  while ( (dlen<buf_size) && (len<buf_size) && (t<ntok) )
  {
    str = strncat(str, tokens[t], buf_size);
    str = strncat(str, delim, dlen);
    len += strlen(tokens[t]) + dlen; 
    t++;
  }
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

