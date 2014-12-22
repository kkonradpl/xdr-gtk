#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "gui-connect.h"
#include "gui-update.h"
#include "connection.h"
#include "tuner.h"
#include "graph.h"
#include "settings.h"
#include "scan.h"
#include "gui.h"
#include "rdsspy.h"
#include "sig.h"
#include "version.h"
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#endif

gint filters[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 29, 2, 28, 1, 26, 0, 24, 23, 22, 21, 20, 19, 18, 17, 16, 31, -1};
gint filters_bw[][FILTERS_N] =
{
    {309000, 298000, 281000, 263000, 246000, 229000, 211000, 194000, 177000, 159000, 142000, 125000, 108000, 95000, 90000, 83000, 73000, 63000, 55000, 48000, 42000, 36000, 32000, 27000, 24000, 20000, 17000, 15000, 9000, -1},
    {38600, 37300, 35100, 32900, 30800, 28600, 26400, 24300, 22100, 19900, 17800, 15600, 13500, 11800, 11300, 10400, 9100, 7900, 6900, 6000, 5200, 4600, 3900, 3400, 2900, 2500, 2200, 1900, 1100, 0}
};

gpointer tuner_read(gpointer nothing)
{
    gchar c, buffer[SERIAL_BUFFER];
    gint n, pos = 0;
    fd_set input;
    struct timeval timeout;

#ifdef G_OS_WIN32
    DWORD len_in = 0;
    BOOL fWaitingOnRead = FALSE;
    DWORD state;
    OVERLAPPED osReader = {0};
    if (tuner.socket == INVALID_SOCKET)
    {
        osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (osReader.hEvent == NULL)
        {
            tuner.thread = FALSE;
        }
    }
#endif
    if(tuner.thread)
    {
        g_usleep(1750000); // arduino may restart during port opening
        tuner_write("x");
        g_usleep(50000);
    }

    while(tuner.thread)
    {
#ifdef G_OS_WIN32
        if (tuner.socket != INVALID_SOCKET)
        {
            FD_ZERO(&input);
            FD_SET(tuner.socket, &input);
            timeout.tv_sec  = 0;
            timeout.tv_usec = 50000;
            if(! (n = select(tuner.socket+1, &input, NULL, NULL, &timeout)))
            {
                continue;
            }
            if(n < 0 || recv(tuner.socket, &c, 1, 0) <= 0)
            {
                break;
            }
        }
        else
        {
            if (!fWaitingOnRead)
            {
                if (!ReadFile(tuner.serial, &c, 1, &len_in, &osReader))
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

                if (!GetOverlappedResult(tuner.serial, &osReader, &len_in, FALSE))
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
        FD_SET(tuner.serial, &input);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000;
        if(!(n = select(tuner.serial+1, &input, NULL, NULL, &timeout)))
        {
            continue;
        }
        if(n < 0 || read(tuner.serial, &c, 1) <= 0)
        {
            break;
        }
#endif
        if(c != '\n' && pos != SERIAL_BUFFER-1)
        {
            buffer[pos++] = c;
            continue;
        }
        buffer[pos] = 0;
        pos = 0;

#if DEBUG_READ
        g_print("read: %s\n", buffer);
#endif

        if(buffer[0] == 'O' && buffer[1] == 'K') // OK
        {
            tuner.ready = TRUE;
        }
        else if(buffer[0] == 'X')
        {
            break;
        }
        else if(buffer[0] == 'a') // socket auth
        {
            gint auth = atoi(buffer+1);
            if(!auth)
            {
                g_idle_add(connection_socket_auth_fail, NULL);
                break;
            }
            else if(auth == 1)
            {
                tuner.guest = TRUE;
            }
        }
        else
        {
            tuner_parse(buffer[0], buffer+1);
        }
    }

// restart Arduino using RTS & DTR lines
#ifdef G_OS_WIN32
    if(tuner.socket != INVALID_SOCKET)
    {
        closesocket(tuner.socket);
    }
    else
    {
        EscapeCommFunction(tuner.serial, CLRDTR);
        EscapeCommFunction(tuner.serial, CLRRTS);
        g_usleep(10000);
        EscapeCommFunction(tuner.serial, SETDTR);
        EscapeCommFunction(tuner.serial, SETRTS);
        CloseHandle(tuner.serial);
    }
#else
    // is this really a serial port?
    if(ioctl(tuner.serial, TIOCMGET, &n) != -1)
    {
        n &= ~(TIOCM_DTR | TIOCM_RTS);
        ioctl(tuner.serial, TIOCMSET, &n);
        g_usleep(10000);
        n |=  (TIOCM_DTR | TIOCM_RTS);
        ioctl(tuner.serial, TIOCMSET, &n);
    }

    close(tuner.serial);
#endif
    tuner.online = 0;
    tuner.guest = FALSE;
    tuner.thread = FALSE;
    g_idle_add(gui_clear_power_off, NULL);
    return NULL;
}

void tuner_parse(gchar c, gchar msg[])
{
    if(c == 'S') // signal level + stereo/mono
    {
        s_data_t* data = g_new(s_data_t, 1);
        sscanf(msg+1, "%f", &data->value);
        data->stereo = (msg[0]=='s');
        data->rds = s.rds_timer;

        s.rds_timer = (s.rds_timer ? (s.rds_timer-1) : 0);
        g_idle_add(gui_update_signal, data);
    }
    else if(c == 'T') // tuned frequency
    {
        gint newfreq = atoi(msg);
        if(newfreq != tuner.freq)
        {
            tuner.prevfreq = tuner.freq;
        }
        tuner.freq = newfreq;
        rdsspy_reset();

        g_idle_add(gui_update_freq, GINT_TO_POINTER(newfreq));
    }
    else if(c == 'V') // alignment data
    {
        g_idle_add(gui_update_alignment, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'P') // PI code
    {
        guint16 pi = strtoul(msg, NULL, 16);
        gboolean checked = (msg[4] != '?');

        s.rds_timer = conf.rds_timeout;
        if(conf.rds_reset)
        {
            s.rds_reset_timer = g_get_real_time();
        }

        if(tuner.pi != pi || tuner.pi_checked != checked)
        {
            tuner.pi = pi;
            tuner.pi_checked = checked;

            pi_t* p = g_new(pi_t, 1);
            p->pi = pi;
            p->checked = checked;
            g_idle_add(gui_update_pi, (gpointer)p);
        }
    }
    else if(c == 'R') // RDS data
    {
        guint data[3], errors, i;

        for(i=0; i<3; i++)
        {
            gchar hex[5];
            strncpy(hex, msg+i*4, 4);
            hex[4] = 0;
            sscanf(hex, "%x", &data[i]);
        }
        sscanf(msg+12, "%x", &errors);

        if(tuner.pi >= 0)
        {
            rdsspy_send(tuner.pi, msg, errors);
        }

        if((!conf.rds_discard && tuner.pi>=0) || s.data[s.pos].rds)
        {
            guchar group = (data[BLOCK_B] & 0xF000) >> 12;
            gboolean flag = (data[BLOCK_B] & 0x0800) >> 11;
            guchar err[] = { (errors&3), ((errors&12)>>2), ((errors&48)>>4) };

            // PTY, TP, TA, MS, AF, ECC: error-free blocks
            if(!err[BLOCK_B])
            {
                gchar pty = (data[BLOCK_B] & 0x03E0) >> 5;
                gchar tp = (data[BLOCK_B] & 0x400) >> 10;
                if(tuner.pty != pty)
                {
                    tuner.pty = pty;
                    g_idle_add(gui_update_pty, GINT_TO_POINTER(pty));
                }

                if(tuner.tp != tp)
                {
                    tuner.tp = tp;
                    g_idle_add(gui_update_tp, GINT_TO_POINTER(tp));
                }

                if(group == 0)
                {
                    gchar ta = (data[BLOCK_B] & 0x10) >> 4;
                    gchar ms = (data[BLOCK_B] & 0x8) >> 3;
                    if(tuner.ta != ta)
                    {
                        tuner.ta = ta;
                        g_idle_add(gui_update_ta, GINT_TO_POINTER(ta));
                    }
                    if(tuner.ms != ms)
                    {
                        tuner.ms = ms;
                        g_idle_add(gui_update_ms, GINT_TO_POINTER(ms));
                    }

                    // AF
                    if(!err[BLOCK_C] && !flag)
                    {
                        guchar af[] = { data[BLOCK_C] >> 8, data[BLOCK_C] & 0xFF };
                        for(i=0; i<2; i++)
                        {
                            if(af[i]>0 && af[i]<205)
                            {
                                g_idle_add(gui_update_af, GINT_TO_POINTER(af[i]));
                            }
                        }
                    }
                }

                // ECC
                if(group == 1 && !flag && !err[BLOCK_C])
                {
                    if(!(data[BLOCK_C] >> 12))
                    {
                        guchar ecc = data[BLOCK_C] & 255;
                        if(tuner.ecc != ecc)
                        {
                            tuner.ecc = ecc;
                            g_idle_add(gui_update_ecc, GINT_TO_POINTER(ecc));
                        }
                    }
                }
            }

            // PS: user-defined error correction
            if(conf.rds_ps_progressive || err[BLOCK_B] <= conf.rds_info_error)
            {
                if(err[BLOCK_B] < 3 && err[BLOCK_D] < 3 && group == 0 && (conf.rds_ps_progressive || (err[BLOCK_D] <= conf.rds_data_error)))
                {
                    gchar pos = data[BLOCK_B] & 3;
                    gchar ps[] = { data[BLOCK_D] >> 8, data[BLOCK_D] & 0xFF };
                    gboolean changed = FALSE;
                    guchar e = 2*err[BLOCK_B] + 3*err[BLOCK_D];

                    for(i=0; i<2; i++)
                    {
                        // only ASCII printable characters
                        if(ps[i] >= 32 && ps[i] < 127)
                        {
                            gint p = pos*2+i;
                            if(!conf.rds_ps_progressive || tuner.ps_err[p] >= e)
                            {
                                if(tuner.ps[p] != ps[i] || tuner.ps_err[p] > e)
                                {
                                    tuner.ps[p] = ps[i];
                                    tuner.ps_err[p] = e;
                                    changed = TRUE;
                                }
                            }
                        }
                    }

                    if(changed)
                    {
                        tuner.ps_avail = TRUE;
                        g_idle_add(gui_update_ps, NULL);
                    }
                }
            }

            // RT: user-defined error correction
            if(err[BLOCK_B] <= conf.rds_info_error)
            {
                if(group == 2)
                {
                    short pos  = (data[BLOCK_B] & 15);
                    gboolean flag = (data[BLOCK_B] & 16) >> 4;
                    gchar rt[] = { data[BLOCK_C]>>8, data[BLOCK_C]&0xFF, data[BLOCK_D]>>8, data[BLOCK_D]&0xFF };
                    gboolean changed = FALSE;

                    for(i=0; i<4; i++)
                    {
                        if(tuner.rt[flag][pos*4+i] == rt[i])
                            continue;

                        if(rt[i] == 0x0D) // end of the RadioText message
                        {
                            if ((i <= 1 && (errors&15) == 0) || (i >= 2 && (errors&51) == 0))
                            {
                                tuner.rt[flag][pos*4+i] = 0;
                                changed = TRUE;
                            }
                        }
                        // only ASCII printable characters
                        else if(rt[i] >= 32 && rt[i] < 127)
                        {
                            if( (i <= 1 && ((errors&12)>>2) <= conf.rds_data_error) || (i >= 2 && ((errors&48)>>4) <= conf.rds_data_error) )
                            {
                                tuner.rt[flag][pos*4+i] = rt[i];
                                changed = TRUE;
                            }
                        }
                    }

                    if(changed)
                    {
                        g_idle_add(gui_update_rt, GINT_TO_POINTER(flag));
                    }
                }
            }
        }
    }
    else if(c == 'U') // Spectral Scan
    {
        gchar tmp[10];
        gint i, j = 0, k = 0, n = 0;
        for(i=0; i<strlen(msg); i++)
            if(msg[i] == ',')
                n++;
        scan_data_t* data = g_new(scan_data_t, 1);
        data->len = n;
        data->min = 100;
        data->max = 0;
        data->signals = g_new(scan_node_t, data->len);
        for(i=0; i<strlen(msg); i++)
        {
            if(msg[i] == '=')
            {
                tmp[j] = 0;
                data->signals[k].freq = atoi(tmp);
                j = 0;
            }
            else if(msg[i] == ',')
            {
                tmp[j] = 0;
                sscanf(tmp, "%f", &data->signals[k].signal);
                //data->signals[k].signal = atoi(tmp);
                j = 0;
                if(data->signals[k].signal > data->max)
                    data->max = data->signals[k].signal;
                if(data->signals[k].signal < data->min)
                    data->min = data->signals[k].signal;
                k++;
            }
            else
                tmp[j++] = msg[i];
        }
        g_idle_add(scan_update, (gpointer)data);
    }
    else if(c == 'N') // stereo pilot level test
    {
        g_idle_add(gui_update_pilot, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Y')  // Sound volume control (network)
    {
        g_idle_add(gui_update_volume, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'A') // RF AGC threshold (network)
    {
        g_idle_add(gui_update_agc, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'D') // De-emphasis (network)
    {
        g_idle_add(gui_update_deemphasis, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Z') // Antenna switch (network)
    {
        g_idle_add(gui_update_ant, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'G') // RF/IF Gain (network)
    {
        g_idle_add(gui_update_gain, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'M') // FM/AM mode (network)
    {
        g_idle_add(((msg[0] == '0') ? gui_mode_FM : gui_mode_AM), NULL);
    }
    else if(c == 'F') // Filter (network)
    {
        g_idle_add(gui_update_filter, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Q') // Squelch (network)
    {
        g_idle_add(gui_update_squelch, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'C') // Rotator control (network + serial)
    {
        g_idle_add(gui_update_rotator, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'o') // On-line users (network)
    {
        tuner.online = atoi(msg);
    }
}

void tuner_write(gchar* command)
{
    gchar *msg;
    gint len;

    if(tuner.thread)
    {
        msg = g_strdup(command);
        len = strlen(command);
        msg[len++] = '\n';

#ifdef G_OS_WIN32
        if(tuner.socket == INVALID_SOCKET)
        {
            // serial connection
            OVERLAPPED osWrite = {0};
            DWORD dwWritten;
            osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if(osWrite.hEvent == NULL)
            {
                g_free(msg);
                return;
            }
            if (!WriteFile(tuner.serial, msg, len, &dwWritten, &osWrite))
            {
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    if(WaitForSingleObject(osWrite.hEvent, INFINITE) == WAIT_OBJECT_0)
                    {
                        GetOverlappedResult(tuner.serial, &osWrite, &dwWritten, FALSE);
                    }
                }
            }
            CloseHandle(osWrite.hEvent);
        }
        else
        {
            tuner_write_socket(tuner.socket, msg, len);
        }
#else
        tuner_write_socket(tuner.serial, msg, len);
#endif
        g_free(msg);
#if DEBUG_WRITE
        g_print("write: %s\n", command);
#endif
    }
}

gboolean tuner_write_socket(int fd, gchar* msg, int len)
{
    // writes to a socket on WIN32
    // or socket/serial on *UNIX
    gint sent, n;
    sent = 0;
    do
    {
#ifdef G_OS_WIN32
        n = send(fd, msg+sent, len-sent, 0);
#else
        n = write(fd, msg+sent, len-sent);
#endif
        if(n < 0)
        {
            closesocket(fd);
            return FALSE;
        }
        sent += n;
    }
    while(sent<len);
    return TRUE;
}

void tuner_poweroff()
{
    tuner_write("X");
}
