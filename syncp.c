/* Based on GNU coreutils sync by Jim Meyering and Giuseppe Scrivano. */

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

#include <libintl.h>
#define _(String) gettext (String)

#define MEMINFO_STRLEN 4096

#ifndef HAVE_SYNCFS
#define HAVE_SYNCFS 0
#endif

enum sync_mode
{
  MODE_FILE,
  MODE_DATA,
  MODE_FILE_SYSTEM,
  MODE_SYNC
};

static void error_tail (int status, int errnum, const char *message, va_list args)
{
    vfprintf (stderr, message, args);
    if (errnum)
    {
        fprintf (stderr, ": %s", strerror (errnum));
    }
    putc ('\n', stderr);
    fflush (stderr);
    if (status)
    {
        exit (status);
    }
}

void error (int status, int errnum, const char *message, ...)
{
    va_list args;
    fflush (stdout);
    fprintf (stderr, "%s: ", PROG_NAME);
    va_start (args, message);
    error_tail (status, errnum, message, args);
    va_end (args);
}

void usage (int status)
{
    if (status != EXIT_SUCCESS)
    {
        fprintf (stderr, _("Try '%s --help' for more information.\n"), PROG_NAME);
    }
    else
    {
        printf (_("Usage: %s [OPTION] [FILE]...\n"), PROG_NAME);
        fputs (_("Synchronize cached writes to persistent storage\n\nIf one or more files are specified, sync only them,\nor their containing file systems.\n\nWhen syncing, display size of remaining data to sync.\n\n"), stdout);
        fputs (_("  -d, --data             sync only file data, no unneeded metadata\n"), stdout);
        fputs (_("  -f, --file-system      sync the file systems that contain the files\n"), stdout);
        fputs (_("  -t, --timeout N        timeout (in seconds) when to exit if sync still not finished\n"), stdout);
        fputs (_("  -p, --period N         period (in seconds) to check buffers size\n"), stdout);
        fputs (_("  -h, --help             display this help and exit\n"), stdout);
        fputs (_("  -v, --version          output version information and exit\n"), stdout);
    }
    exit (status);
}

/* Sync the specified FILE, or file systems associated with FILE.
   Return 1 on success.  */

static int sync_arg (enum sync_mode mode, char const *file)
{
    int ret = 1;
    int open_flags = O_RDONLY | O_NONBLOCK;
    int fd;

#ifdef _AIX
    /* AIX 7.1 fsync requires write access to file.  */
    if (mode == MODE_FILE)
    {
        open_flags = O_WRONLY | O_NONBLOCK;
    }
#endif

    /* Note O_PATH might be supported with syncfs(), though as of Linux 3.18 is not.  */
    fd = open (file, open_flags);
    if (fd < 0)
    {
        /* Use the O_RDONLY errno, which is significant with directories for example.  */
        int rd_errno = errno;
        if (open_flags != (O_WRONLY | O_NONBLOCK))
        {
            fd = open (file, O_WRONLY | O_NONBLOCK);
        }
        if (fd < 0)
        {
            error (0, rd_errno, _("error opening \"%s\""), file);
            return 0;
        }
    }

    /* We used O_NONBLOCK above to not hang with fifos, so reset that here.  */
    int fdflags = fcntl (fd, F_GETFL);
    if (fdflags == -1 || fcntl (fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0)
    {
        error (0, errno, _("couldn't reset non-blocking mode \"%s\""), file);
        ret = 0;
    }

    if (ret == 1)
    {
        int sync_status = -1;

        switch (mode)
        {
            case MODE_DATA:
                sync_status = fdatasync (fd);
                break;

            case MODE_FILE:
                sync_status = fsync (fd);
                break;

#if HAVE_SYNCFS
            case MODE_FILE_SYSTEM:
                sync_status = syncfs (fd);
                break;
#endif

            default:
                assert ("invalid sync_mode");
        }

        if (sync_status < 0)
        {
            error (0, errno, _("error syncing \"%s\""), file);
            ret = 0;
        }
    }

    if (close (fd) < 0)
    {
        error (0, errno, _("failed to close \"%s\""), file);
        ret = 0;
    }

    return ret;
}

static int ret = 0;
static int childs = 0;

void child_exit(int sig, siginfo_t *info, void *ucontext)
{
    ret |= info->si_status;
    --childs;
}

void check_string (const char *pattern, const char *source, char *target)
{
    if (!strncmp (source, pattern, strlen (pattern)))
    {
        for(source += strlen (pattern); *source == ' '; ++source);
        strncpy(target, source, MEMINFO_STRLEN);
        for(; *target; ++target)
        {
            if (*target == '\n')
            {
                *target = '\0';
                break;
            }
        }
    }
}

int main (int argc, char **argv)
{
    int c;
    int args_specified;
    int arg_data = 0, arg_file_system = 0;
    enum sync_mode mode;
    char *endp;
    int timeout = 0;
    int period = 1;
    time_t starttime;
    int timeout_exceeded = 0;
    static struct option const long_options[] =
    {
        {"data", no_argument, 0, 'd'},
        {"file-system", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 't'},
        {"period", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {NULL, 0, 0, 0}
    };
    static struct sigaction act =
    {
        .sa_sigaction = child_exit,
        .sa_flags = SA_NOCLDSTOP | SA_SIGINFO
    };

    while ((c = getopt_long (argc, argv, "dft:p:hv", long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                arg_data = 1;
                break;

            case 'f':
                arg_file_system = 1;
                break;

            case 't':
                timeout = strtol(optarg,&endp,0);
                if(!endp || !*endp)
                {
                    error (0, errno, _("wrong timeout argument %s"), optarg);
                }
                break;

            case 'p':
                period = strtol(optarg,&endp,0);
                if(!endp || !*endp)
                {
                    error (0, errno, _("wrong period argument %s"), optarg);
                }
                break;

            case 'h':
                usage (EXIT_SUCCESS);
                break;

            case 'v':
                printf (_("%s version %s\n"), PROG_NAME, PROG_VERSION);
                exit (EXIT_SUCCESS);
                break;

            default:
                usage (EXIT_FAILURE);
        }
    }

    args_specified = optind < argc;

    if (arg_data && arg_file_system)
    {
        error (EXIT_FAILURE, 0, _("cannot specify both --data and --file-system"));
    }

    if (!args_specified && arg_data)
    {
        error (EXIT_FAILURE, 0, _("--data needs at least one argument"));
    }

    if (! args_specified || (arg_file_system && ! HAVE_SYNCFS))
    {
        mode = MODE_SYNC;
    }
    else if (arg_file_system)
    {
        mode = MODE_FILE_SYSTEM;
    }
    else if (! arg_data)
    {
        mode = MODE_FILE;
    }
    else
    {
        mode = MODE_DATA;
    }

    sigemptyset(&act.sa_mask);
    if (sigaction (SIGCHLD, &act, NULL) != 0)
    {
        error (0, errno, _("can't register signal hanlder"));
    }

    if (mode == MODE_SYNC)
    {
        pid_t pid = fork();
        if (!pid)
        {
            sync ();
            return 0;
        }
        ++childs;
    }
    else
    {
        for (; optind < argc; optind++)
        {
            pid_t pid = fork();
            if (!pid)
            {
                return sync_arg (mode, argv[optind]);
            }
            ++childs;
        }
    }

    starttime = time(NULL);
    for(;;)
    {
        const char *unknown = _("unknown");
        char dirty[MEMINFO_STRLEN], writeback[MEMINFO_STRLEN], buf[MEMINFO_STRLEN];

        strcpy(dirty, unknown);
        strcpy(writeback, unknown);

        FILE *f = fopen ("/proc/meminfo", "r");
        if (f != NULL)
        {
            while (!feof (f))
            {
                if (fgets (buf, MEMINFO_STRLEN, f) != NULL)
                {
                    check_string ("Dirty:", buf, dirty);
                    check_string ("Writeback:", buf, writeback);
                }
            }
            fclose (f);
        }
        printf (_("\rDirty: %s, Writeback: %s, processes: %d"), dirty, writeback, childs);
        if (childs < 1)
        {
            break;
        }
        if (timeout && difftime (time (NULL), starttime) > 0.0 + timeout)
        {
            timeout_exceeded = 1;
            break;
        }
        sleep(period);
    }

    printf (_("\n"));

    if (timeout_exceeded)
    {
        error (EXIT_FAILURE, 0, _("timeout is exceeded, probably still syncing"));
    }

    if (ret)
    {
        error (EXIT_FAILURE, 0, _("can't sync some data"));
    }

    return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
