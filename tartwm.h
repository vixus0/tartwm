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
  char ** cmd_argv;
  char * config_file;
  bool is_host;
  size_t cmd_argc;
};


// -- Misc
void parse_config (struct config * cfg, char * path);
int32_t connect_host (const char * socket_path);

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

// -- XCB
void init_xcb (xcb_connection_t ** c, xcb_screen_t ** s);
void register_events (xcb_connection_t * c, xcb_window_t win, uint32_t mask);
uint32_t win_children (xcb_connection_t * c, xcb_window_t win, xcb_window_t * children[]);
