//
// tartwm.c
//

#include <errno.h>
#include <signal.h> // For catching SIGINT, SIGTERM
#include <unistd.h> // For getopt()
#include <string.h> // For strtok
#include <stdbool.h> // For booleans
#include <stdint.h>  // For clearer integer types
#include <stdlib.h>  // For getenv, atoi, etc.
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>

#include "tartwm.h"

#define BUFSIZE 255
#define MAXSPLIT 32
#define MAXCLIENTS 5

#define NORTH (1 << 0)
#define SOUTH (1 << 1)
#define EAST (1 << 2)
#define WEST (1 << 3)

static bool run;
static struct config cfg = {
  .x = 6, .y = 5, .g = 8, .t = 0, .b = 0, .l = 0, .r = 0, .bw = 2,
  .cf = 0xff00ccff,
  .cu = 0xff808080,
  .ci = 0xffcc0000
  };


int32_t
main (int32_t argc, char * argv[])
{
  char * display = getenv("DISPLAY");
  char * rundir = getenv("XDG_RUNTIME_DIR");
  char socket_path[BUFSIZE];
  struct main_args args;

  parse_main_args(argc, argv, &args);

  if ( display == NULL )
  {
    fputs("DISPLAY not set.\n", stderr);
    exit(EXIT_FAILURE);
  }

  if ( rundir != NULL )
  {
    char rundir_root[BUFSIZE];

    snprintf(rundir_root, BUFSIZE, "%s/tartwm", rundir);

    if ( mkdir(rundir_root, S_IRWXU) == 0 )
      snprintf(socket_path, BUFSIZE, "%s/display-%s.sock", rundir_root, &display[1]);
    else
      rundir = NULL;
  }

  if ( rundir == NULL )
  {
    snprintf(socket_path, BUFSIZE, "/tmp/tartwm-%s.sock", &display[1]);
  }

  if ( args.is_host )
  { /* Host */
    int32_t slisten, sclient;
    uint32_t len, t;
    struct sockaddr_un local, remote;

    local.sun_family = AF_UNIX;

    if ( args.config_file != NULL )
    {
      parse_config(&cfg, args.config_file);
    }

    if ( (slisten = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 )
    {
      fputs("Failed to create socket.\n", stderr);
      perror("socket");
      exit(EXIT_FAILURE);
    }
    
    strncpy(local.sun_path, socket_path, sizeof(local.sun_path));
    unlink(local.sun_path);
    len = SUN_LEN(&local);

    if ( bind(slisten, (struct sockaddr *)&local, len) == -1 )
    {
      fputs("Failed to bind socket.\n", stderr);
      perror("bind");
      exit(EXIT_FAILURE);
    }

    if ( listen(slisten, MAXCLIENTS) == -1 )
    {
      fputs("Failed to listen.\n", stderr);
      perror("listen");
      exit(EXIT_FAILURE);
    }

    if ( fork() != 0 )
    { /* Parent */
      char buf[BUFSIZE] = "";
      bool done;

      run = true;

      while ( run ) 
      {
        t = sizeof(remote);

        if ( (sclient = accept(slisten, (struct sockaddr *)&remote, &t)) == -1 )
        {
          fputs("Failed to accept connection.\n", stderr);
          perror("accept");
          exit(EXIT_FAILURE);
        }

        done = false;

        do {
          switch ( recv(sclient, buf, BUFSIZE, 0) )
          {
            case 0:
              done = true;
              break;
            case -1:
              perror("recv");
              done = true;
              break;
            default:
              printf("recv: %s\n", buf);
              break;
          }
        }
        while (!done);

        close(sclient);
      }

      close(slisten);
    }
    else
    { /* Child */
      char buf[BUFSIZE] = "test";
      int32_t s;
      //xcb_connection_t * conn = xcb_connect(NULL, NULL);
      //xcb_generic_event_t * event;
      
      close(slisten);

      s = connect_host(socket_path);

      run = true;

      while ( run )
      {
        printf("Sending test message.\n");

        if ( send(s, buf, BUFSIZE, 0) == -1 )
        {
          fputs("Couldn't send to TartWM.\n", stderr);
          perror("send");
          exit(EXIT_FAILURE);
        }

        sleep(1);
      }

      close(s);
    }
  }
  else
  { /* Client */
    char buf[BUFSIZE] = "";
    int32_t s; 

    s = connect_host(socket_path);

    join_line(args.cmd_argv, args.cmd_argc, buf, BUFSIZE, " ");

    if ( send(s, buf, BUFSIZE, 0) == -1 )
    {
      fputs("Couldn't send to TartWM.\n", stderr);
      perror("send");
      exit(EXIT_FAILURE);
    }

    close(s);
  }

  return EXIT_SUCCESS;
}


int32_t
connect_host (const char * socket_path)
{
  int32_t s;
  uint32_t len;
  struct sockaddr_un remote;

  if ( (s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 )
  {
    fputs("Error creating socket.\n", stderr);
    perror("socket");
    exit(EXIT_FAILURE);
  }

  remote.sun_family = AF_UNIX;
  strncpy(remote.sun_path, socket_path, sizeof(remote.sun_path));
  len = SUN_LEN(&remote);

  if ( connect(s, (struct sockaddr *)&remote, len) == -1 )
  {
    fputs("Error connecting to TartWM.\n", stderr);
    perror("connect");
    exit(EXIT_FAILURE);
  }

  return s;
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

  char buf[BUFSIZE] = "";
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
  size_t count = 0;
  char * tok;

  if ( (tok = strchr(line, '\n')) != NULL )
    *tok = '\0';

  tok = strtok(line, delim); 

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
  args->config_file = NULL;

  while ( (opt = getopt(argc, argv, "+c:h")) != -1 ) {
    switch ( opt )
    {
      case 'c':
        args->config_file = optarg;
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
    args->is_host = true;
  }
  else
  {
    args->cmd_argv = &argv[optind];
    args->cmd_argc = argc - optind;
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


bool
handle_event (xcb_generic_event_t * event, char * send, size_t send_size)
{
  return false;
}
