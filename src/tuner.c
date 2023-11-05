#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#ifdef G_OS_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#include "tuner.h"
#include "log.h"
#include "stationlist.h"
#include "tuner-callbacks.h"
#include "ui-tuner-update.h"
#include "conf.h"
#include "rds-utils.h"

#define DEBUG_READ  1
#define DEBUG_WRITE 1

#define SERIAL_BUFFER 10000
tuner_t tuner;

#define CALLBACK_PRIORITY G_PRIORITY_HIGH_IDLE

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

    g_idle_add_full(CALLBACK_PRIORITY, tuner_disconnect, thread, NULL);
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
        g_idle_add_full(CALLBACK_PRIORITY, tuner_ready, NULL, NULL);
    }
    else if(c == 'X')
    {
        /* Tuner shutdown */
        return FALSE;
    }
    else if(c == 'T')
    {
        /* Tuned frequency */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_freq, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'V')
    {
        /* DAA tuning voltage */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_daa, GINT_TO_POINTER(atoi(msg)), NULL);
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
        g_idle_add_full(CALLBACK_PRIORITY, tuner_signal, data, NULL);

        if((ptr = strchr(msg, ',')))
        {
            g_idle_add_full(CALLBACK_PRIORITY, tuner_cci, GINT_TO_POINTER(atoi(ptr+1)), NULL);
            if((ptr = strchr(ptr+1, ',')))
                g_idle_add_full(CALLBACK_PRIORITY, tuner_aci, GINT_TO_POINTER(atoi(ptr+1)), NULL);
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

        g_idle_add_full(CALLBACK_PRIORITY, tuner_pi, GUINT_TO_POINTER(pi), NULL);
    }
    else if(c == 'R' && strlen(msg) == 14)
    {
        /* RDS data */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_rds_legacy, g_strdup(msg), NULL);
    }
    else if(c == 'R' && strlen(msg) == 18)
    {
        /* RDS data */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_rds_new, g_strdup(msg), NULL);
    }
    else if(c == 'U')
    {
        /* Spectral scan */
        tuner_scan_t *scan = tuner_scan_parse(msg);
        if(scan)
            g_idle_add_full(CALLBACK_PRIORITY, tuner_scan, (gpointer)scan, NULL);
    }
    else if(c == 'N')
    {
        /* Stereo pilot injection level estimation */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_pilot, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'Y')
    {
        /* Sound volume control */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_volume, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'A')
    {
        /* RF AGC threshold */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_agc, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'D')
    {
        /* De-emphasis */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_deemphasis, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'Z')
    {
        /* Antenna switch */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_antenna, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'G')
    {
        /* RF & IF gain setting */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_gain, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'M')
    {
        /* FM / AM mode */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_mode, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'F')
    {
        /* Filter */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_filter, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'W')
    {
        /* Bandwidth */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_bandwidth, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'Q')
    {
        /* Squelch */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_squelch, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'C')
    {
        /* Rotator control */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_rotator, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == 'I')
    {
        /* Custom signal level sampling interval */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_sampling_interval, GINT_TO_POINTER(atoi(msg)), NULL);
    }
    else if(c == '!')
    {
        /* External event */
        g_idle_add_full(CALLBACK_PRIORITY, tuner_event, NULL, NULL);
    }
    else if(c == 'o')
    {
        /* Online users (network) */
        gchar *ptr;
        g_idle_add_full(CALLBACK_PRIORITY, tuner_online, GINT_TO_POINTER(atoi(msg)), NULL);
        if((ptr = strchr(msg, ',')))
            g_idle_add_full(CALLBACK_PRIORITY, tuner_online_guests, GINT_TO_POINTER(atoi(ptr+1)), NULL);
    }
    else if(c == 'a')
    {
        /* Authorization (network) */
        gint auth = atoi(msg);
        if(!auth)
        {
            g_idle_add_full(CALLBACK_PRIORITY, tuner_unauthorized, NULL, NULL);
            return FALSE;
        }
        else if(auth == 1)
        {
            g_idle_add_full(CALLBACK_PRIORITY, tuner_ready, GINT_TO_POINTER(TRUE), NULL);
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

static void
callback_pty(rdsparser_t *rds,
             void        *user_data)
{
    ui_update_pty();
}

static void
callback_tp(rdsparser_t *rds,
            void        *user_data)
{
    ui_update_tp();
}

static void
callback_ta(rdsparser_t *rds,
            void        *user_data)
{
    ui_update_ta();
}

static void
callback_ms(rdsparser_t *rds,
            void        *user_data)
{
    ui_update_ms();
}

static void
callback_country(rdsparser_t *rds,
                 void        *user_data)
{
    ui_update_country();
}

static void
callback_af(rdsparser_t *rds,
            uint32_t     new_af,
            void        *user_data)
{
    ui_update_af(new_af);
}

static void
callback_ps(rdsparser_t *rds,
            void        *user_data)
{
    ui_update_ps();

    const rdsparser_string_t *ps = rdsparser_get_ps(rds);
    gchar *ps_text = rds_utils_text(ps);
    stationlist_ps(ps_text);

    gboolean error = FALSE;
    const rdsparser_string_error_t *errors = rdsparser_string_get_errors(ps);
    const uint8_t length = rdsparser_string_get_length(ps);
    for (gint i = 0; i < length; i++)
    {
        if (errors[i])
        {
            error = TRUE;
            break;
        }
    }

    log_ps(ps_text, error);
    g_free(ps_text);
}

static void
callback_rt(rdsparser_t         *rds,
            rdsparser_rt_flag_t  flag,
            void                *user_data)
{
    ui_update_rt(flag);
    const rdsparser_string_t *rt = rdsparser_get_rt(rds, flag);
    gchar *rt_text = rds_utils_text(rt);
    stationlist_rt(flag, rt_text);
    log_rt(flag, rt_text);
    g_free(rt_text);
}

static void
callback_ct(rdsparser_t          *rds,
            const rdsparser_ct_t *ct,
            void                 *user_data)
{
    int16_t offset = rdsparser_ct_get_offset(ct);
    char *datetime = g_strdup_printf("%04d-%02d-%02d %02d:%02d %c%02d:%02d",
                                     rdsparser_ct_get_year(ct),
                                     rdsparser_ct_get_month(ct),
                                     rdsparser_ct_get_day(ct),
                                     rdsparser_ct_get_hour(ct),
                                     rdsparser_ct_get_minute(ct),
                                     (offset >= 0) ? '+' : '-',
                                     abs(offset / 60),
                                     abs(offset % 60));

    log_ct(datetime);
    g_free(datetime);

}

void
tuner_rds_init()
{
    tuner.rds = rdsparser_new();

    tuner_rds_configure();

    rdsparser_register_pty(tuner.rds, callback_pty);
    rdsparser_register_tp(tuner.rds, callback_tp);
    rdsparser_register_ta(tuner.rds, callback_ta);
    rdsparser_register_ms(tuner.rds, callback_ms);
    rdsparser_register_country(tuner.rds, callback_country);
    rdsparser_register_af(tuner.rds, callback_af);
    rdsparser_register_ps(tuner.rds, callback_ps);
    rdsparser_register_rt(tuner.rds, callback_rt);
    rdsparser_register_ct(tuner.rds, callback_ct);
}

void
tuner_rds_configure()
{
    rdsparser_set_extended_check(tuner.rds, conf.rds_extended_check);

    rdsparser_block_error_t ps_info_error = (conf.rds_ps_progressive && conf.rds_ps_prog_override ? RDSPARSER_BLOCK_ERROR_LARGE : conf.rds_ps_info_error);
    rdsparser_block_error_t ps_data_error = (conf.rds_ps_progressive && conf.rds_ps_prog_override ? RDSPARSER_BLOCK_ERROR_LARGE : conf.rds_ps_data_error);
    rdsparser_set_text_correction(tuner.rds, RDSPARSER_TEXT_PS, RDSPARSER_BLOCK_TYPE_INFO, ps_info_error);
    rdsparser_set_text_correction(tuner.rds, RDSPARSER_TEXT_PS, RDSPARSER_BLOCK_TYPE_DATA, ps_data_error);
    rdsparser_set_text_progressive(tuner.rds, RDSPARSER_TEXT_PS, conf.rds_ps_progressive);

    rdsparser_block_error_t rt_info_error = (conf.rds_rt_progressive && conf.rds_rt_prog_override ? RDSPARSER_BLOCK_ERROR_LARGE : conf.rds_rt_info_error);
    rdsparser_block_error_t rt_data_error = (conf.rds_rt_progressive && conf.rds_rt_prog_override ? RDSPARSER_BLOCK_ERROR_LARGE : conf.rds_rt_data_error);
    rdsparser_set_text_correction(tuner.rds, RDSPARSER_TEXT_RT, RDSPARSER_BLOCK_TYPE_INFO, rt_info_error);
    rdsparser_set_text_correction(tuner.rds, RDSPARSER_TEXT_RT, RDSPARSER_BLOCK_TYPE_DATA, rt_data_error);
    rdsparser_set_text_progressive(tuner.rds, RDSPARSER_TEXT_RT, conf.rds_rt_progressive);
}

void tuner_clear_all()
{
    log_cleanup();

    tuner.freq = 0;
    tuner.prevfreq = 0;
    tuner.prevantenna = 0;
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
    tuner.rds_timeout = 0;
    ui_update_rds_flag();
    tuner.rds_reset_timer = 0;

    tuner.rds_pi = -1;
    tuner.rds_pi_err_level = G_MAXINT;
    ui_update_pi();

    rdsparser_clear(tuner.rds);

    ui_update_tp();
    ui_update_ta();
    ui_update_ms();
    ui_update_pty();
    ui_update_country();
    ui_update_ps();
    ui_update_rt(0);
    ui_update_rt(1);

    ui_clear_af();
}

gint tuner_get_freq()
{
    return tuner.freq - tuner.offset[tuner.antenna];
}

gint tuner_get_offset()
{
    return tuner.offset[tuner.antenna];
}

void tuner_set_offset(gint antenna,
                      gint offset)
{
    if(antenna >= 0 && antenna <= ANT_COUNT)
        tuner.offset[antenna] = offset;
}
