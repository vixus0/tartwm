//
// args.h
// Argument processing
//

struct main_args
{
  char ** command;
  char * config_file;
  bool is_host;
  size_t ncommand;
};

struct move_args
{
  bool collide, delta;
  xcb_window_t window;
  int32_t x, y;
}

void
parse_main_args (int32_t argc, char * argv[], struct main_args * args)
{
  extern char * optarg;
  extern int optind;
  int32_t opt;

  const char * usage = "Usage: tartwm [-c config_file] [command [args]]\n";

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
    args->command = &argv[optind];
    args->ncommand = argc - optind;
  }
}
