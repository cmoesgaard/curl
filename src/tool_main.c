/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://carl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "tool_setup.h"

#include <sys/stat.h>

#ifdef WIN32
#include <tchar.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef USE_NSS
#include <nspr.h>
#include <plarenas.h>
#endif

#define ENABLE_CARLX_PRINTF
/* use our own printf() functions */
#include "carlx.h"

#include "tool_cfgable.h"
#include "tool_convert.h"
#include "tool_doswin.h"
#include "tool_msgs.h"
#include "tool_operate.h"
#include "tool_panykey.h"
#include "tool_vms.h"
#include "tool_main.h"
#include "tool_libinfo.h"

/*
 * This is low-level hard-hacking memory leak tracking and similar. Using
 * the library level code from this client-side is ugly, but we do this
 * anyway for convenience.
 */
#include "memdebug.h" /* keep this as LAST include */

#ifdef __VMS
/*
 * vms_show is a global variable, used in main() as parameter for
 * function vms_special_exit() to allow proper carl tool exiting.
 * Its value may be set in other tool_*.c source files thanks to
 * forward declaration present in tool_vms.h
 */
int vms_show = 0;
#endif

#ifdef __MINGW32__
/*
 * There seems to be no way to escape "*" in command-line arguments with MinGW
 * when command-line argument globbing is enabled under the MSYS shell, so turn
 * it off.
 */
int _CRT_glob = 0;
#endif /* __MINGW32__ */

/* if we build a static library for unit tests, there is no main() function */
#ifndef UNITTESTS

/*
 * Ensure that file descriptors 0, 1 and 2 (stdin, stdout, stderr) are
 * open before starting to run.  Otherwise, the first three network
 * sockets opened by carl could be used for input sources, downloaded data
 * or error logs as they will effectively be stdin, stdout and/or stderr.
 */
static void main_checkfds(void)
{
#ifdef HAVE_PIPE
  int fd[2] = { STDIN_FILENO, STDIN_FILENO };
  while(fd[0] == STDIN_FILENO ||
        fd[0] == STDOUT_FILENO ||
        fd[0] == STDERR_FILENO ||
        fd[1] == STDIN_FILENO ||
        fd[1] == STDOUT_FILENO ||
        fd[1] == STDERR_FILENO)
    if(pipe(fd) < 0)
      return;   /* Out of handles. This isn't really a big problem now, but
                   will be when we try to create a socket later. */
  close(fd[0]);
  close(fd[1]);
#endif
}

#ifdef CARLDEBUG
static void memory_tracking_init(void)
{
  char *env;
  /* if CARL_MEMDEBUG is set, this starts memory tracking message logging */
  env = carlx_getenv("CARL_MEMDEBUG");
  if(env) {
    /* use the value as file name */
    char fname[CARL_MT_LOGFNAME_BUFSIZE];
    if(strlen(env) >= CARL_MT_LOGFNAME_BUFSIZE)
      env[CARL_MT_LOGFNAME_BUFSIZE-1] = '\0';
    strcpy(fname, env);
    carl_free(env);
    carl_dbg_memdebug(fname);
    /* this weird stuff here is to make carl_free() get called before
       carl_gdb_memdebug() as otherwise memory tracking will log a free()
       without an alloc! */
  }
  /* if CARL_MEMLIMIT is set, this enables fail-on-alloc-number-N feature */
  env = carlx_getenv("CARL_MEMLIMIT");
  if(env) {
    char *endptr;
    long num = strtol(env, &endptr, 10);
    if((endptr != env) && (endptr == env + strlen(env)) && (num > 0))
      carl_dbg_memlimit(num);
    carl_free(env);
  }
}
#else
#  define memory_tracking_init() Curl_nop_stmt
#endif

/*
 * This is the main global constructor for the app. Call this before
 * _any_ libcarl usage. If this fails, *NO* libcarl functions may be
 * used, or havoc may be the result.
 */
static CARLcode main_init(struct GlobalConfig *config)
{
  CARLcode result = CARLE_OK;

#if defined(__DJGPP__) || defined(__GO32__)
  /* stop stat() wasting time */
  _djstat_flags |= _STAT_INODE | _STAT_EXEC_MAGIC | _STAT_DIRSIZE;
#endif

  /* Initialise the global config */
  config->showerror = -1;             /* Will show errors */
  config->errors = stderr;            /* Default errors to stderr */
  config->styled_output = TRUE;       /* enable detection */
  config->parallel_max = PARALLEL_DEFAULT;

  /* Allocate the initial operate config */
  config->first = config->last = malloc(sizeof(struct OperationConfig));
  if(config->first) {
    /* Perform the libcarl initialization */
    result = carl_global_init(CARL_GLOBAL_DEFAULT);
    if(!result) {
      /* Get information about libcarl */
      result = get_libcarl_info();

      if(!result) {
        /* Initialise the config */
        config_init(config->first);
        config->first->global = config;
      }
      else {
        errorf(config, "error retrieving carl library information\n");
        free(config->first);
      }
    }
    else {
      errorf(config, "error initializing carl library\n");
      free(config->first);
    }
  }
  else {
    errorf(config, "error initializing carl\n");
    result = CARLE_FAILED_INIT;
  }

  return result;
}

static void free_globalconfig(struct GlobalConfig *config)
{
  Curl_safefree(config->trace_dump);

  if(config->errors_fopened && config->errors)
    fclose(config->errors);
  config->errors = NULL;

  if(config->trace_fopened && config->trace_stream)
    fclose(config->trace_stream);
  config->trace_stream = NULL;

  Curl_safefree(config->libcarl);
}

/*
 * This is the main global destructor for the app. Call this after
 * _all_ libcarl usage is done.
 */
static void main_free(struct GlobalConfig *config)
{
  /* Cleanup the easy handle */
  /* Main cleanup */
  carl_global_cleanup();
  convert_cleanup();
  metalink_cleanup();
#ifdef USE_NSS
  if(PR_Initialized()) {
    /* prevent valgrind from reporting still reachable mem from NSRP arenas */
    PL_ArenaFinish();
    /* prevent valgrind from reporting possibly lost memory (fd cache, ...) */
    PR_Cleanup();
  }
#endif
  free_globalconfig(config);

  /* Free the config structures */
  config_free(config->last);
  config->first = NULL;
  config->last = NULL;
}

/*
** carl tool main function.
*/
#ifdef _UNICODE
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  CARLcode result = CARLE_OK;
  struct GlobalConfig global;
  memset(&global, 0, sizeof(global));

#ifdef WIN32
  /* Undocumented diagnostic option to list the full paths of all loaded
     modules. This is purposely pre-init. */
  if(argc == 2 && !_tcscmp(argv[1], _T("--dump-module-paths"))) {
    struct carl_slist *item, *head = GetLoadedModulePaths();
    for(item = head; item; item = item->next)
      printf("%s\n", item->data);
    carl_slist_free_all(head);
    return head ? 0 : 1;
  }
  /* win32_init must be called before other init routines. */
  result = win32_init();
  if(result) {
    fprintf(stderr, "carl: (%d) Windows-specific init failed.\n", result);
    return result;
  }
#endif

  main_checkfds();

#if defined(HAVE_SIGNAL) && defined(SIGPIPE)
  (void)signal(SIGPIPE, SIG_IGN);
#endif

  /* Initialize memory tracking */
  memory_tracking_init();

  /* Initialize the carl library - do not call any libcarl functions before
     this point */
  result = main_init(&global);
  if(!result) {
    /* Start our carl operation */
    result = operate(&global, argc, argv);

    /* Perform the main cleanup */
    main_free(&global);
  }

#ifdef __NOVELL_LIBC__
  if(getenv("_IN_NETWARE_BASH_") == NULL)
    tool_pressanykey();
#endif

#ifdef __VMS
  vms_special_exit(result, vms_show);
#else
  return (int)result;
#endif
}

#endif /* ndef UNITTESTS */
