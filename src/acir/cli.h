#ifndef ACIR_CLI_H
#define ACIR_CLI_H
#include <annec_anchor.h>

#define ANSI_RESET "\033[m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_GRAY "\033[90m"

extern AnchCharWriteStream *wsStdout;
extern AnchCharWriteStream *wsStderr;

#endif
