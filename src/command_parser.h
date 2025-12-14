#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdbool.h>

// Initialize command parser (starts thread)
void command_parser_init(void);

// Enable/disable GPS streaming
void command_parser_set_streaming(bool enable);

// Check if streaming is enabled
bool command_parser_is_streaming(void);

#endif // COMMAND_PARSER_H