#ifndef ACIR_CLI_H
#define ACIR_CLI_H
#include <annec/anchor.h>

#define ANSI_RESET "\033[m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_GRAY "\033[90m"
#define SEPARATOR "======================="
#define SEPARATOR_STRING(NAME, L, R) ANSI_GRAY "\n" SEPARATOR L "[ " ANSI_MAGENTA NAME ANSI_GRAY " ]" R SEPARATOR ANSI_RESET
#define WRITE_SEPARATOR2(NAME, L, R) AnchWriteString(wsStdout, SEPARATOR_STRING(NAME, L, R) "\n")
#define WRITE_SEPARATOR1(NAME, A) WRITE_SEPARATOR2(NAME, A, A)
#define WRITE_SEPARATOR(NAME) WRITE_SEPARATOR1(NAME, "")

extern AnchCharWriteStream *wsStdout;
extern AnchCharWriteStream *wsStderr;

#endif
