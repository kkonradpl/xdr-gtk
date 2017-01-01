#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#include "tuner.h"
#include "log.h"
#include "tuner-callbacks.h"
#include "ui-tuner-update.h"

#define DEBUG_READ  0
#define DEBUG_WRITE 1

#define SERIAL_BUFFER 10000

typedef struct tuner_thread
{
    gintptr fd;
    gint type;  /* TUNER_THREAD_SERIAL or TUNER_THREAD_SOCKET */
    volatile gboolean canceled;
} tuner_thread_t;

static gpointer tuner_thread(gpointer);
static gboolean tuner_parse(gchar, gchar*);
static void tuner_restart(gintptr);
static gboolean tuner_write_serial(gintptr, gchar*, int);

gpointer
tuner_thread_new(gint    type,
                 gintptr fd)
{
    tuner_thread_t *thread = g_malloc(sizeof(tuner_thread_t));
    g_assert(type == TUNER_THREAD_SERIAL ||
             type == TUNER_THREAD_SOCKET);

    thread->fd = fd;
    thread->type = type;
    thread->canceled = FALSE;

    g_thread_unref(g_thread_new("tuner", tuner_thread, (gpointer)thread));
    return thread;
}

void
tuner_thread_cancel(gpointer thread)
{
    ((tuner_thread_t*)thread)->canceled = TRUE;
}

static gpointer
tuner_thread(gpointer data)
{
    tuner_thread_t *thread = (tuner_thread_t*)data;
    gchar buffer[SERIAL_BUFFER];
    gint pos = 0;

    struct timeval timeout;
    fd_set input;
    gint n;

    g_print("thread start: %p\n", data);

#ifdef G_OS_WIN32
    DWORD len_in = 0;
    BOOL fWaitingOnRead = FALSE;
    DWORD state;
    OVERLAPPED osReader = {0};
    if(thread->type == TUNER_THREAD_SERIAL)
    {
        osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(osReader.hEvent == NULL)
            goto tuner_thread_cleanup;
    }
#endif

    /* Arduino may restart during port opening */
    if(thread->type == TUNER_THREAD_SERIAL)
        g_usleep(1750 * 1000);

    if(thread->canceled)
        goto tuner_thread_cleanup;

    tuner_write(thread, "x");

    while(!thread->canceled)
    {
#ifdef G_OS_WIN32
        if(thread->type == TUNER_THREAD_SOCKET)
        {
            FD_ZERO(&input);
            FD_SET(thread->fd, &input);
            timeout.tv_sec  = 0;
            timeout.tv_usec = 50000;
            n = select(thread->fd+1, &input, NULL, NULL, &timeout);
            if(!n)
                continue;
            if(n < 0 || recv(thread->fd, &buffer[pos], 1, 0) <= 0)
                break;
        }
        else
        {
            if (!fWaitingOnRead)
            {
                if (!ReadFile((HANDLE)thread->fd, &buffer[pos], 1, &len_in, &osReader))
                {
                    if (GetLastError() != ERROR_IO_PENDING)
                    {
                        CloseHandle(osReader.hEvent);
                        break;
                    }
                    else
                    {
                        fWaitingOnRead = TRUE;
                    }
                }
            }

            if (fWaitingOnRead)
            {
                state = WaitForSingleObject(osReader.hEvent, 200);
                if(state == WAIT_TIMEOUT)
                {
                    continue;
                }
                if(state != WAIT_OBJECT_0)
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }

                if (!GetOverlappedResult((HANDLE)thread->fd, &osReader, &len_in, FALSE))
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }

                fWaitingOnRead = FALSE;
            }
            if(len_in != 1)
            {
                continue;
            }
        }
#else
        FD_ZERO(&input);
        FD_SET(thread->fd, &input);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000;
        n = select(thread->fd+1, &input, NULL, NULL, &timeout);
        if(!n)
            continue;
        if(n < 0 || read(thread->fd, &buffer[pos], 1) <= 0)
            break;
#endif
        /* If this command is too long to
         * fit into a buffer, clip it */
        if(buffer[pos] != '\n')
        {
            if(pos != SERIAL_BUFFER-1)
                pos++;
            continue;
        }
        buffer[pos] = 0;
        pos = 0;

#if DEBUG_READ
        g_print("read: %s\n", buffer);
#endif
        if(!tuner_parse(buffer[0], buffer+1))
            break;
    }

tuner_thread_cleanup:
    if(thread->type == TUNER_THREAD_SOCKET)
    {
#ifdef G_OS_WIN32
        shutdown(thread->fd, 2);
        closesocket(thread->fd);
#else
        shutdown(thread->fd, 2);
        close(thread->fd);
#endif
    }
    else
    {
        tuner_restart(thread->fd);
#ifdef G_OS_WIN32
        CloseHandle((HANDLE)thread->fd);
#else
        close(thread->fd);
#endif
    }

    g_idle_add(tuner_disconnect, thread);
    g_print("thread stop: %p\n", data);
    return NULL;
}

static gboolean
tuner_parse(gchar  c,
            gchar *msg)
{
    if(c == 'O' && msg[0] == 'K')
    {
        /* Tuner startup */
        g_idle_add(tuner_ready, NULL);
    }
    else if(c == 'X')
    {
        /* Tuner shutdown */
        return FALSE;
    }
    else if(c == 'T')
    {
        /* Tuned frequency */
        g_idle_add(tuner_freq, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'V')
    {
        /* DAA tuning voltage */
        g_idle_add(tuner_daa, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'S' && strlen(msg) >= 2)
    {
        /* Signal strength and quality indicators */
        tuner_signal_t *data = g_malloc(sizeof(tuner_signal_t));
        gchar *ptr;
        switch(msg[0])
        {
            case 's':
                data->stereo = SIGNAL_STEREO;
                break;
            case 'S':
                data->stereo = SIGNAL_STEREO | SIGNAL_FORCED_MONO;
                break;
            case 'M':
                data->stereo = SIGNAL_FORCED_MONO;
                break;
            default:
                data->stereo = SIGNAL_MONO;
                break;
        }
        data->value = g_ascii_strtod(msg+1, NULL);
        g_idle_add(tuner_signal, data);

        if((ptr = strchr(msg, ',')))
        {
            g_idle_add(tuner_cci, GINT_TO_POINTER(atoi(ptr+1)));
            if((ptr = strchr(ptr+1, ',')))
                g_idle_add(tuner_aci, GINT_TO_POINTER(atoi(ptr+1)));
        }
    }
    else if(c == 'P' && strlen(msg) >= 4)
    {
        /* PI code */
        guint pi = strtoul(msg, NULL, 16);
        guint8 err = 0;
        gchar *ptr;

        for(ptr = msg+4; *ptr; ptr++)
            if(*ptr == '?')
                err++;
        pi |= (((err > 3) ? 3 : err) << 16);

        g_idle_add(tuner_pi, GUINT_TO_POINTER(pi));
    }
    else if(c == 'R' && strlen(msg) == 14)
    {
        /* RDS data */
        g_idle_add(tuner_rds, g_strdup(msg));
    }
    else if(c == 'U')
    {
        /* Spectral scan */
        tuner_scan_t *scan = tuner_scan_parse(msg);
        if(scan)
            g_idle_add(tuner_scan, (gpointer)scan);
    }
    else if(c == 'N')
    {
        /* Stereo pilot injection level estimation */
        g_idle_add(tuner_pilot, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Y')
    {
        /* Sound volume control */
        g_idle_add(tuner_volume, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'A')
    {
        /* RF AGC threshold */
        g_idle_add(tuner_agc, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'D')
    {
        /* De-emphasis */
        g_idle_add(tuner_deemphasis, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Z')
    {
        /* Antenna switch */
        g_idle_add(tuner_antenna, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'G')
    {
        /* RF & IF gain setting */
        g_idle_add(tuner_gain, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'M')
    {
        /* FM / AM mode */
        g_idle_add(tuner_mode, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'F')
    {
        /* Filter */
        g_idle_add(tuner_filter, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Q')
    {
        /* Squelch */
        g_idle_add(tuner_squelch, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'C')
    {
        /* Rotator control */
        g_idle_add(tuner_rotator, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'I')
    {
        /* Custom signal level sampling interval */
        g_idle_add(tuner_sampling_interval, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == '!')
    {
        /* External event */
        g_idle_add(tuner_event, NULL);
    }
    else if(c == 'o')
    {
        /* Online users (network) */
        gchar *ptr;
        g_idle_add(tuner_online, GINT_TO_POINTER(atoi(msg)));
        if((ptr = strchr(msg, ',')))
            g_idle_add(tuner_online_guests, GINT_TO_POINTER(atoi(ptr+1)));
    }
    else if(c == 'a')
    {
        /* Authorization (network) */
        gint auth = atoi(msg);
        if(!auth)
        {
            g_idle_add(tuner_unauthorized, NULL);
            return FALSE;
        }
        else if(auth == 1)
        {
            g_idle_add(tuner_ready, GINT_TO_POINTER(TRUE));
        }
    }
    return TRUE;
}

static void
tuner_restart(gintptr fd)
{
#ifdef G_OS_WIN32
    EscapeCommFunction((HANDLE)fd, CLRDTR);
    EscapeCommFunction((HANDLE)fd, CLRRTS);
    g_usleep(10000);
    EscapeCommFunction((HANDLE)fd, SETDTR);
    EscapeCommFunction((HANDLE)fd, SETRTS);
#else
    gint n;
    if(ioctl(fd, TIOCMGET, &n) == -1)
        return;
    n &= ~(TIOCM_DTR | TIOCM_RTS);
    ioctl(fd, TIOCMSET, &n);
    g_usleep(10000);
    n |=  (TIOCM_DTR | TIOCM_RTS);
    ioctl(fd, TIOCMSET, &n);
#endif
}

void
tuner_write(gpointer  ptr,
            gchar    *command)
{
    tuner_thread_t *thread;
    gchar *msg;
    gint len;
    gboolean ret;

    if(!ptr)
        return;

    thread = (tuner_thread_t*)ptr;
    msg = g_strdup(command);
    len = strlen(command);
    msg[len++] = '\n';

    if(thread->type == TUNER_THREAD_SERIAL)
        ret = tuner_write_serial(thread->fd, msg, len);
    else
        ret = tuner_write_socket(thread->fd, msg, len);

    g_free(msg);
#if DEBUG_WRITE
    g_print("write%s: %s\n",
            (!ret ? " ERROR" : ""),
            command);
#endif
}

#ifdef G_OS_WIN32
static gboolean
tuner_write_serial(gintptr  fd,
                   gchar   *msg,
                   gint     len)
{
    OVERLAPPED osWrite = {0};
    DWORD dwWritten;
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(osWrite.hEvent == NULL)
        return FALSE;
    if (!WriteFile((HANDLE)fd, msg, len, &dwWritten, &osWrite))
        if (GetLastError() == ERROR_IO_PENDING)
            if(WaitForSingleObject(osWrite.hEvent, INFINITE) == WAIT_OBJECT_0)
                GetOverlappedResult((HANDLE)fd, &osWrite, &dwWritten, FALSE);
    CloseHandle(osWrite.hEvent);
    return TRUE;
}
#else
static gboolean
tuner_write_serial(gintptr  fd,
                   gchar   *msg,
                   gint     len)
{
    gint sent = 0;
    gint n;
    do
    {
        n = write(fd, msg+sent, len-sent);
        if(n < 0)
            return FALSE;
        sent += n;
    }
    while(sent < len);
    return TRUE;
}
#endif

gboolean
tuner_write_socket(gintptr  fd,
                   gchar   *msg,
                   gint     len)
{
    gint sent = 0;
    gint n;

    do
    {
#ifdef G_OS_WIN32
        n = send(fd, msg+sent, len-sent, 0);
#else
        n = send(fd, msg+sent, len-sent, MSG_NOSIGNAL);
#endif
        if(n < 0)
        {
            shutdown(fd, 2);
            return FALSE;
        }
        sent += n;
    }
    while(sent < len);
    return TRUE;
}

void tuner_clear_all()
{
    log_cleanup();

    tuner.freq = 0;
    tuner.prevfreq = 0;
    ui_update_freq();

    tuner.sampling_interval = 0;
    tuner.signal = NAN;
    tuner.forced_mono = FALSE;
    tuner_clear_signal();
    ui_update_signal();

    tuner.cci = -1;
    ui_update_cci();
    tuner.aci = -1;
    ui_update_aci();

    tuner_clear_rds();

    tuner.ready = FALSE;
    tuner.ready_tuned = FALSE;
    tuner.guest = FALSE;
    tuner.online = 0;
    tuner.online_guests = 0;
    tuner.send_settings = TRUE;
    tuner.thread = NULL;

    tuner.rotator = 0;
    ui_update_rotator();

    ui_update_disconnected();
}

void tuner_clear_signal()
{
    tuner.signal_max = NAN;
    tuner.signal_sum = 0.0;
    tuner.signal_samples = 0;

    tuner.stereo = FALSE;
    ui_update_stereo_flag();
}

void tuner_clear_rds()
{
    gint i;

    tuner.rds = 0;
    ui_update_rds_flag();
    tuner.rds_reset_timer = 0;

    tuner.rds_pi = -1;
    tuner.rds_pi_err_level = G_MAXINT;
    ui_update_pi();

    tuner.rds_tp = -1;
    ui_update_tp();

    tuner.rds_ta = -1;
    ui_update_ta();

    tuner.rds_ms = -1;
    ui_update_ms();

    tuner.rds_pty = -1;
    ui_update_pty();

    tuner.rds_ecc = -1;
    ui_update_ecc();

    sprintf(tuner.rds_ps, "%8s", "");
    for(i=0; i<8; i++)
        tuner.rds_ps_err[i] = 0xFF;
    tuner.rds_ps_avail = FALSE;
    ui_update_ps(FALSE);

    sprintf(tuner.rds_rt[0], "%64s", "");
    tuner.rds_rt_avail[0] = FALSE;
    ui_update_rt(0);
    sprintf(tuner.rds_rt[1], "%64s", "");
    tuner.rds_rt_avail[1] = FALSE;
    ui_update_rt(1);

    ui_clear_af();
}
