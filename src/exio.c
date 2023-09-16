/*
 * Copyright (C) 2023 Emil Overbeck <emil.a.overbeck at gmail dot com>
 * Subject to the MIT License. See LICENSE.txt for more information.
 *
 */

#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <limits.h>

#include "exio.h"

#define PREF_ERROR      "error: "
#define PREF_WARNING    "warning: "
#define PREF_INFO       "info: "

#define CHAR_YES    'y'
#define CHAR_NO     'n'

#define STR_EQ(a, b) (strcmp(a, b) == 0)
#define ARRAY_LEN(a) (sizeof(a) / sizeof(*(a)))

#define MSG(pref, format)                                   \
    do {                                                    \
        va_list ap;                                         \
        bool ret;                                           \
                                                            \
        va_start(ap, (format));                             \
                                                            \
        ret = (fputs((pref), stderr) != EOF                 \
               && vfprintf(stderr, (format), ap) >= 0       \
               && fputc('\n', stderr) != EOF);              \
                                                            \
        va_end(ap);                                         \
        return ret;                                         \
    } while (0)


bool err(const char *restrict format, ...)
{
    MSG(C_ERROR PREF_ERROR C_NORMAL, format);
}

bool warn(const char *restrict format, ...)
{
    MSG(C_WARNING PREF_WARNING C_NORMAL, format);
}

bool info(const char *restrict format, ...)
{
    MSG(C_INFO PREF_INFO C_NORMAL, format);
}

bool confirm(const char *prompt)
{
    char in[] = { '\0', '\0', '\0' };
    int  c;

    for (;;) {
        fputs(prompt, stderr);
        if (!fgets(in, sizeof(in), stdin)) return false;

        if (in[1] != '\n' && in[0] != '\n') {
            /* Consume the rest of the input */
            while ((c = fgetc(stdin)) != '\n' && c != EOF);
            continue;
        }

        if (in[0] == CHAR_YES)
            return true;
        else if (in[0] == CHAR_NO)
            return false;
        else
            continue;
    }
}

char *getusrln(const char *prompt, size_t *input_len, enum input_mode mode)
{
    struct termios old, new;

    ssize_t  input_sz;
    char    *buf = NULL;
    size_t   buf_sz = 0;

    if (mode == IN_HIDE) {
        /* Test if terminal supports 'termios' */
        if (tcgetattr(STDIN_FILENO, &old) != 0)
            return NULL;

        new = old;
        new.c_lflag &= ~ECHO;

        /* Temporarily disable terminal text echoing to hide input */
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
            return NULL;
    }

    fputs(prompt, stderr);
    input_sz = getline(&buf, &buf_sz, stdin);
    if (mode == IN_HIDE || input_sz == -1) fputc('\n', stderr);    /* Print the newline entered */
    if (input_sz == -1) goto fail;

    if (mode == IN_HIDE && tcsetattr(STDIN_FILENO, TCSAFLUSH, &old) != 0)
        goto fail;

    // Ignore the trailing newline
    if (input_len)
        *input_len = input_sz - 1;
    else
        buf[input_sz - 1] = '\0';

    return buf;

fail:
    /* We have no way of reliably zeroing the memory allocated by 'getline()' on
       failure because we cannot know its size or if it is safe to clear. This
       is the disadvantage of accepting sensitive input of arbitrary length. */
    free(buf);
    return NULL;
}

int get_xdg_path(char *restrict path, const char *sub_dir,
                 const char *xdg_dir, const char *fallback_dir)
{
    const char *xdg, *home;

    if ((xdg = getenv(xdg_dir))) {
        // Take the '/' character to be added into account
        if (strlen(xdg) + strlen(sub_dir) + 1 > PATH_MAX)
            goto fail;

        snprintf(path, PATH_MAX, "%s/%s", xdg, sub_dir);
    } else if ((home = getenv("HOME"))) {
        // Take the '/' characters to be added into account
        if (strlen(home) + strlen(fallback_dir) + strlen(sub_dir)+ 2 > PATH_MAX)
            goto fail;

        snprintf(path, PATH_MAX, "%s/%s/%s", home, fallback_dir, sub_dir);
    } else {
        return -2;
    }

    return 0;

fail:
    errno = ENAMETOOLONG;
    return -1;
}

bool mkpath(char *path)
{
    char *path_iter;

    /* The directories are created with permissions 0777 - umask, yielding
       standard permissions. Error 'EEXIST' is acceptable because the path or
       part of it may already exist, which we are expected to ignore. */
    for (path_iter = path + 1; *path_iter; ++path_iter) {
        if (*path_iter != '/') break;
        *path_iter = '\0';      /* Temporarily truncate */

        if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && errno != EEXIST) {
            *path_iter = '/';
            return false;
        }

        *path_iter = '/';
    }

    if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && errno != EEXIST)
        return false;

    return true;
}

off_t fsize(int fd)
{
    struct stat st;

    /* We do not use 'fseek()' or 'ftello()' because 'SEEK_END' with binary data
       is not standard C. */
    return (fstat(fd, &st) == 0) ? st.st_size : -1;
}

void set_handler_segv(void (*func)(int signo))
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigfillset(&act.sa_mask);
    act.sa_handler = func;

    sigaction(SIGSEGV, &act, NULL);
}

void set_handler_term(void (*func)(int signo))
{
    struct sigaction act, test;
    size_t i;

    int term_sigs[] = {
        // Standard C
        SIGINT,
        SIGTERM,
        SIGABRT,
        SIGFPE,
        SIGILL,

        // POSIX.1
#ifdef SIGHUP
        SIGHUP,
#endif
#ifdef SIGQUIT
        SIGQUIT,
#endif
#ifdef SIGPIPE
        SIGPIPE,
#endif
#ifdef SIGUSR1
        SIGUSR1,
#endif
#ifdef SIGUSR2
        SIGUSR2,
#endif

        // POSIX.2
#ifdef SIGALRM
        SIGALRM,
#endif
#ifdef SIGBUS
        SIGBUS,
#endif

        // BSD
#ifdef SIGIO
        SIGIO,
#endif
#ifdef SIGVTALRM
        SIGVTALRM,
#endif
#ifdef SIGXCPU
        SIGXCPU,
#endif
#ifdef SIGXFSZ
        SIGXFSZ
#endif
    };

    memset(&act, 0, sizeof(act));
    memset(&test, 0, sizeof(test));
    act.sa_handler = func;
    act.sa_flags = SA_RESTART;

    for (i = 0; i < ARRAY_LEN(term_sigs); ++i) {
        sigaction(term_sigs[i], NULL, &test);

        // Only set the handler for signals not ignored by default
        if (test.sa_handler != SIG_IGN) {
            sigaddset(&act.sa_mask, term_sigs[i]);
            sigaction(term_sigs[i], &act, NULL);
        }
    }
}

void reset_handler(int signo)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;

    sigaction(signo, &act, NULL);
}
