//
// tartwm.h
// Argument processing
//

// -- Handy structs
struct config 
{
  uint16_t x, y, g, t, b, l, r, bw;
  uint32_t cf, cu, ci;
};

struct rectangle {
  uint16_t x, y, w, h;
};

struct main_args
{
  char ** command;
  char * config_file;
  bool is_host, is_watcher;
  size_t ncommand;
};


// -- Misc
void handle_sig (int32_t signal);
void parse_config (struct config * cfg, char * path);
FILE * open_fifo_writer (const char * fifo_path, int32_t * fifo_fd);

// -- String handling
size_t split_line (char * line, const char * delim, char * tokens[], size_t tok_size);
void join_line(char * tokens[], size_t ntok, char * buf, size_t buf_size, const char * delim);
bool assign_uint16 (const char * match, char * key, char * val, uint16_t * dest);
bool assign_uint32 (const char * match, char * key, char * val, uint32_t * dest);

// -- Argument parsing
void parse_main_args (int32_t argc, char * argv[], struct main_args * args);
void parse_subcommand (char * line);

// -- Events
bool handle_event (xcb_generic_event_t * event, char * send, size_t send_size);
