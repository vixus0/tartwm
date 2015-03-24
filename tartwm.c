//
// tartwm.c
//

#include <errno.h>
#include <fcntl.h>  // For open() flags O_*
#include <signal.h> // For catching SIGINT, SIGTERM
#include <unistd.h> // For getopt()
#include <string.h> // For strtok
#include <stdbool.h> // For booleans
#include <stdint.h>  // For clearer integer types
#include <stdlib.h>  // For getenv, atoi, etc.
#include <stdio.h>
#include <sys/stat.h>  // For file mode constants S_*
#include <xcb/xproto.h>
#include <xcb/xcb.h>

#include "tartwm.h"

#define BUFSIZE 255
#define MAXSPLIT 32

#define NORTH (1 << 0)
#define SOUTH (1 << 1)
#define EAST (1 << 2)
#define WEST (1 << 3)

static bool run;


int32_t
main (int32_t argc, char * argv[])
{
  int32_t fifo_fd;
  char * display = getenv("DISPLAY");

  if ( display == NULL )
  {
    fputs("DISPLAY not set.\n", stderr);
    exit(EXIT_FAILURE);
  }

  struct main_args args;
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
    fifo_fd = open(fifo_path, O_RDONLY);
    FILE * fifo_read = fdopen(fifo_fd, "r");

    if ( fifo_read == NULL ) 
    {
      fputs("Failed to open FIFO for reading.\n", stderr);
      exit(EXIT_FAILURE);
    }

    // Fork watcher process
    pid_t pid = fork();

    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    if ( pid != 0 )
    {
      char buf[BUFSIZE];

      printf("Running.\n");
      run = true;

      while ( run ) 
      {
        if ( fgets(buf, BUFSIZE, fifo_read) != NULL ) 
          parse_subcommand(buf);
      }

      fclose(fifo_read);
      close(fifo_fd);
      
      remove(fifo_path);
    }
    else
    {
      printf("Forking tartwm -w\n");
      fclose(fifo_read);
      close(fifo_fd);

      char * args[] = { "tartwm", "-w", NULL };

      if ( execv("./tartwm", args) < 0 )
      {
        fprintf(stderr, "Error spawning watcher: %d\n", errno);
      }
    }
  }
  else if ( args.is_watcher )
  {
    // We watch for X events and send them to the host
    FILE * fifo_write;
    char send[BUFSIZE];
    int32_t ret = 0;

    xcb_connection_t * conn = xcb_connect(NULL, NULL);
    xcb_generic_event_t * event;

    printf("Watching for X events.\n");

    run = true;

    while ( run )
    {
      fifo_write = open_fifo_writer(fifo_path, &fifo_fd);

      if (fifo_write == NULL)
      {
        run = false;
      }
      else
      {
        //event = xcb_wait_for_event(conn);
        //if ( handle_event(event, send, BUFSIZE) )
        //  ret = fputs(send, fifo_write);
        fputs("test", fifo_write);
        fclose(fifo_write);
        sleep(1);
      }
    }

    close(fifo_fd);
  }
  else
  {
    // We are a client
    FILE * fifo_write = open_fifo_writer(fifo_path, &fifo_fd);

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
    close(fifo_fd);
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
  char * str;

  if ( (str = strchr(line, '\n')) != NULL )
    *str = '\0';

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


void
parse_main_args (int32_t argc, char * argv[], struct main_args * args)
{
  extern char * optarg;
  extern int optind;
  int32_t opt;

  const char * usage = "Usage: tartwm [-c config_file] [command [args]]\n";

  args->is_host = false;
  args->is_watcher = false;
  args->config_file = NULL;

  while ( (opt = getopt(argc, argv, "+c:hw")) != -1 ) {
    switch ( opt )
    {
      case 'c':
        args->config_file = optarg;
        break;

      case 'w':
        args->is_watcher = true;
        break;

      case 'h':
        fputs(usage, stderr);
        exit(EXIT_SUCCESS);
        break;

      default:
        fputs(usage, stderr);
        exit(EXIT_FAILURE);
    }
  }

  if ( optind == argc )
  {
    // No more arguments
    args->is_host = ! args->is_watcher;
  }
  else
  {
    args->is_watcher = false;
    args->command = &argv[optind];
    args->ncommand = argc - optind;
  }
}


void
parse_subcommand (char * line)
{
  char * tokens[MAXSPLIT];
  char * cmd;

  if ( split_line(line, " ", tokens, MAXSPLIT) > 0 )
  { 
    cmd = tokens[0];

    if ( strcmp(cmd, "move") == 0 )
    {
      printf("moving here, boss\n");
    } 
    else if ( strcmp(cmd, "size") == 0 )
    {
      printf("sizing here, boss\n");
    }
    else
    {
      fprintf(stderr, "Unrecognised command: %s\n", cmd);
    }
  }
}


FILE *
open_fifo_writer (const char * fifo_path, int32_t * fifo_fd)
{
  if ( (*fifo_fd = open(fifo_path, O_NONBLOCK | O_WRONLY)) == -1 )
  {
    fputs("Is TartWM running?\n", stderr);
    exit(EXIT_FAILURE);
  }

  FILE * fifo_write = fdopen(*fifo_fd, "w");

  if ( fifo_write == NULL )
  {
    fputs("Can't contact TartWM.\n", stderr);
    exit(EXIT_FAILURE);
  }

  return fifo_write;
}


bool
handle_event (xcb_generic_event_t * event, char * send, size_t send_size)
{
  if ( event != NULL )
  {
    strncpy(send, "X event received", send_size);
    return true;
  }
  return false;
}
