/*
 * Copyright (C) 2023 Emil Overbeck <emil.a.overbeck at gmail dot com>
 * Subject to the MIT License. See LICENSE.txt for more information.
 *
 * Library containing various *ex*tensions to `stdio.h`.
 *
 * Define the macro `EXIO_USE_COLOUR` to enable coloured output with the user IO
 * functions.
 */

#ifndef EXIO_H
#define EXIO_H

#include <stdbool.h>
#include <stdio.h>

/* ANSI colour codes. */
#ifdef EXIO_USE_COLOUR
#  define C_NORMAL    "\033[0m"       /* Reset.       */
#  define C_ERROR     "\033[31;1m"    /* Bold red.    */
#  define C_WARNING   "\033[33;1m"    /* Bold yellow. */
#  define C_INFO      "\033[34;1m"    /* Bold blue.   */
#  define C_TITLE     "\033[36;1m"    /* Bold cyan.   */
#  define C_HEADING   "\033[32;1m"    /* Bold green.  */
#else
#  define C_NORMAL
#  define C_ERROR
#  define C_WARNING
#  define C_INFO
#  define C_TITLE
#  define C_HEADING
#endif /* EXIO_USE_COLOUR */

/* Whether or not to echo user input when with 'getusrln()'. */
enum input_mode {
    IN_HIDE,
    IN_SHOW
};

/*
 * Write formatted messages to stderr.
 *
 * 'format' must be a null-terminated string; the syntax is the same as with
 * 'printf'. These functions write a trailing newline.
 *
 * Return true on success.
 * Return false on output failure.
 *
 */
bool err(const char *restrict format, ...);
bool warn(const char *restrict format, ...);
bool info(const char *restrict format, ...);

/*
 * Obtain confirmation from the user while displaying 'prompt'.
 *
 * Uses the characters 'y' and 'n' (case sensitive) for assent and denial
 * respectively.
 *
 * 'prompt' must be a null-terminated string.
 *
 * Returns true on user assent.
 * Returns false on user denial.
 *
 */
bool confirm(const char *prompt);

/*
 * Securely read a line from the user.
 *
 * Displays 'prompt' on 'stderr' while reading user input and sets 'input_len'
 * to the read data length (without the trailing newline). The read input is
 * treated as a C string if 'input_len' is NULL. Whether the input is echoed or
 * not depends on 'mode'.
 *
 * 'prompt' must be a null-terminated string.
 *
 * Returns pointer to read data on success.
 * Returns NULL and sets errno on failure.
 * Returns NULL and sets the 'stdin' EOF indicator if EOF was encountered.
 *
 * The returned pointer should be freed after use.
 *
 */
char *getusrln(const char *prompt, size_t *input_len, enum input_mode mode);

/*
 * Build a standard application path and copy it into 'path'.
 *
 * Uses the standard XDG directory specified in 'xdg_dir' if set, and
 * 'fallback_dir' appended to '$HOME' otherwise. 'sub_path' is an
 * application-specific path within the standard directory appended last.
 *
 * 'path' must be an array of minimum size 'PATH_MAX' + 1. Directory parameters
 * can be passed without leading or trailing path separator '/' characters.
 *
 * Returns 0 on success.
 * Returns -1 and sets errno on overlong path error.
 * Returns -2 on environment error.
 *
 */
int get_xdg_path(char *restrict path, const char *sub_dir,
                 const char *xdg_dir, const char *fallback_dir);

/*
 * Recursively create 'path' à la 'mkdir -p'.
 *
 * Already existing directories in the path (or its entirety) are ignored, and
 * directories are created with permissions 0777 - umask.
 *
 * 'path' must be a null-terminated string representing an absolute path. It is
 * modified during execution, and restored afterwards regardless of errors.
 *
 * Returns true on success.
 * Returns false and sets errno on failure.
 *
 */
bool mkpath(char *path);

/*
 * Obtain the file size of 'fd'.
 *
 * 'fd' must be a valid file descriptor.
 *
 * Returns a positive number on success.
 * Returns -1 and may set errno on failure.
 *
 */
off_t fsize(int fd);

/*
 * Set 'func' as the handler for SIGSEGV, with the signal as argument.
 *
 * All possible signals are blocked while 'func' runs. The behaviour is
 * undefined if the handler does not terminate the program.
 *
 */
void set_handler_segv(void (*func)(int signo));

/*
 * Set 'func' as the handler for all fatal signals except for SIGSEGV, with the
 * signal as argument.
 *
 * All possible signals are blocked while 'func' runs. The behaviour is
 * undefined if the handler does not terminate the program and the signal is
 * SIGABRT.
 *
 */
void set_handler_term(void (*func)(int signo));

/*
 * Reset the handling for signal 'sig'.
 *
 */
void reset_handler(int signo);

#endif /* EXIO_H */
