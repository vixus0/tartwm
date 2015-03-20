//
// tartwm.c
//

#include <fcntl.h>  // For open() flags O_*
#include <signal.h> // For catching SIGINT, SIGTERM
#include <unistd.h> // For size_t, pid_t, getpid()
#include <string.h> // For strtok

#include <stdbool.h> // For booleans
#include <stdint.h>  // For clearer integer types
#include <stdlib.h>  // For getenv, atoi, etc.
#include <stdio.h>

#include <sys/stat.h>  // For file mode constants S_*
#include <xcb/xcb.h>

#include "tartwm.h"

bool run;

int32_t
main (int32_t argc, char * argv[])
{
  if ( argc == 1 )
  {
    // Default config
    struct config cfg = {
      .x = 6, .y = 5, .g = 8, .t = 0, .b = 0, .l = 0, .r = 0, .bw = 2,
      .cf = 0xff00ccff,
      .cu = 0xff808080,
      .ci = 0xffcc0000
      };

    parse_config(&cfg);

    // We are the host
    pid_t pid = getpid();

    char fifo_path[255];
    snprintf(fifo_path, sizeof(fifo_path), "/tmp/tartwm-%ld.fifo", (long)pid);

    if ( mkfifo(fifo_path, S_IRWXU | S_IRWXG) < 0 ) 
    {
      fputs("Failed to create FIFO.\n", stderr);
      exit(EXIT_FAILURE);
    }

    FILE *pidf = fopen("/tmp/tartwm.pid", "w");

    if ( pidf == NULL ) 
    {
      fputs("Failed to write pidfile.\n", stderr);
      exit(EXIT_FAILURE);
    } 

    fprintf(pidf, "%ld", (long)pid);
    fclose(pidf);

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
    ssize_t len;

    run = true;

    while ( run ) 
    {
      if ( fgets(buf, sizeof(buf), fifo_read) != NULL ) 
        printf("read: %s", buf);
    }

    fclose(fifo_read);
    close(fifo_fd);
    
    remove(fifo_path);
    remove("/tmp/tartwm.pid");
  }
  else
  {
    // We are a client
  }

  return EXIT_SUCCESS;
}

void 
handle_sig (int32_t sig)
{
  run = false;
}

void
parse_config (struct config * c)
{
  char buf[256];
  char * pos[32];
  uint8_t ntok;
  uint8_t i, a;

  while ( fgets(buf, sizeof(buf), stdin) != NULL )
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
}

uint8_t
split_line (char * line, const char * delim, char * pos[])
{
  // Get rid of any newline
  char * npos;

  if ( (npos = strchr(line, '\n')) != NULL )
    *npos = '\0';

  uint8_t count = 0;
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

