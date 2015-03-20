//
// tartwm.h
//

struct config 
{
  uint16_t x, y, g, t, b, l, r, bw;
  uint32_t cf, cu, ci;
};

struct rectangle {
  uint16_t x, y, w, h;
};

void handle_sig (int32_t signal);

void parse_config (struct config * cfg, FILE * input);
uint8_t split_line (char * line, const char * delim, char ** tokens);
bool assign_uint16 (const char * match, char * key, char * val, uint16_t * dest);
bool assign_uint32 (const char * match, char * key, char * val, uint32_t * dest);
