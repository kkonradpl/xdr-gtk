#include <string.h>
#include <stdlib.h>
#include "ui-connect.h"
#include "tuner-conn.h"
#include "tuner.h"
#ifdef G_OS_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "win32.h"
#else
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <netinet/tcp.h>

#endif

#define SOCKET_TCP_KEEPCNT    2
#define SOCKET_TCP_KEEPINTVL 10
#define SOCKET_TCP_KEEPIDLE  30

gint
tuner_open_serial(const gchar *serial_port,
                  gintptr     *fd)
{
    gchar path[100];
#ifdef G_OS_WIN32
    HANDLE serial;
    DCB dcbSerialParams = {0};

    g_snprintf(path, sizeof(path), "\\\\.\\%s", serial_port);
    serial = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(serial == INVALID_HANDLE_VALUE)
        return CONN_SERIAL_FAIL_OPEN;
    if(!GetCommState(serial, &dcbSerialParams))
    {
        CloseHandle(serial);
        return CONN_SERIAL_FAIL_PARM_R;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(!SetCommState(serial, &dcbSerialParams))
    {
        CloseHandle(serial);
        return CONN_SERIAL_FAIL_PARM_W;
    }
    *fd = (gintptr)serial;
#else
    struct termios options;
    g_snprintf(path, sizeof(path), "/dev/%s", serial_port);
    *fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    if(*fd < 0)
        return CONN_SERIAL_FAIL_OPEN;
    fcntl(*fd, F_SETFL, 0);
    tcflush(*fd, TCIOFLUSH);
    if(tcgetattr(*fd, &options))
    {
        close(*fd);
        return CONN_SERIAL_FAIL_PARM_R;
    }
    if(cfsetispeed(&options, B115200) || cfsetospeed(&options, B115200))
    {
        close(*fd);
        return CONN_SERIAL_FAIL_SPEED;
    }
    options.c_iflag &= ~(BRKINT | ICRNL | IXON | IMAXBEL);
    options.c_iflag |= IGNBRK;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | ECHOK | ECHOCTL | ECHOKE);
    options.c_oflag &= ~(OPOST | ONLCR);
    options.c_oflag |= NOFLSH;
    options.c_cflag |= CS8;
    options.c_cflag &= ~(CRTSCTS);
    if(tcsetattr(*fd, TCSANOW, &options))
    {
        close(*fd);
        return CONN_SERIAL_FAIL_PARM_W;
    }
#endif
    return CONN_SUCCESS;
}

gpointer
tuner_open_socket(gpointer ptr)
{
    conn_t *data = (conn_t*)ptr;
    struct addrinfo hints = {0}, *result;
    struct timeval timeout = {0};
    fd_set input;
    gchar salt[SOCKET_SALT_LEN+1], msg[42];
    GChecksum *sha1;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Resolve the hostname */
    data->state = CONN_SOCKET_STATE_RESOLV;
    g_idle_add(connection_socket_callback_info, data);
    if(getaddrinfo(data->hostname, data->port, &hints, &result))
    {
        data->state = CONN_SOCKET_FAIL_RESOLV;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }

    if(data->canceled)
    {
        freeaddrinfo(result);
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }

    data->state = CONN_SOCKET_STATE_CONN;
    g_idle_add(connection_socket_callback_info, data);
    data->socketfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(connect(data->socketfd, result->ai_addr, result->ai_addrlen) < 0 || data->canceled)
    {
        closesocket(data->socketfd);
        freeaddrinfo(result);
        data->state = CONN_SOCKET_FAIL_CONN;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }
    freeaddrinfo(result);

    data->state = CONN_SOCKET_STATE_AUTH;
    g_idle_add(connection_socket_callback_info, data);
    FD_ZERO(&input);
    FD_SET(data->socketfd, &input);
    timeout.tv_sec = SOCKET_AUTH_TIMEOUT;
    /* Wait SOCKET_AUTH_TIMEOUT seconds for the salt */
    if(select(data->socketfd+1, &input, NULL, NULL, &timeout) <= 0 || data->canceled)
    {
        closesocket(data->socketfd);
        data->state = CONN_SOCKET_FAIL_AUTH;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }

    /* Receive SOCKET_SALT_LENGTH+1 bytes (with \n) */
    if(recv(data->socketfd, salt, SOCKET_SALT_LEN+1, 0) != SOCKET_SALT_LEN+1 || data->canceled)
    {
        closesocket(data->socketfd);
        data->state = CONN_SOCKET_FAIL_AUTH;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }

    /* Calculate the SHA1 checksum of salt and password concatenation */
    sha1 = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(sha1, (guchar*)salt, SOCKET_SALT_LEN);
    if(data->password && strlen(data->password))
    {
        g_checksum_update(sha1, (guchar*)data->password, strlen(data->password));
    }
    g_snprintf(msg, sizeof(msg), "%s\n", g_checksum_get_string(sha1));
    g_checksum_free(sha1);

    /* Send the hash */
    if(!tuner_write_socket(data->socketfd, msg, strlen(msg)) || data->canceled)
    {
        closesocket(data->socketfd);
        data->state = CONN_SOCKET_FAIL_WRITE;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }

#ifdef G_OS_WIN32
    DWORD ret;
    struct tcp_keepalive ka =
    {
        .onoff = 1,
        .keepaliveinterval = SOCKET_TCP_KEEPINTVL * SOCKET_TCP_KEEPCNT * 1000 / 10,
        .keepalivetime = SOCKET_TCP_KEEPIDLE * 1000
    };
    WSAIoctl(data->socketfd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &ret, NULL, NULL);
#else
    gint opt = 1;
    if(setsockopt(data->socketfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) >= 0)
    {
        opt = SOCKET_TCP_KEEPCNT;
        setsockopt(data->socketfd, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));

        opt = SOCKET_TCP_KEEPINTVL;
        setsockopt(data->socketfd, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));

        opt = SOCKET_TCP_KEEPIDLE;
        setsockopt(data->socketfd, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
    }
#endif

    data->state = CONN_SUCCESS;
    g_idle_add(connection_socket_callback, data);
    return NULL;
}

conn_t*
conn_new(const gchar *hostname,
         const gchar *port,
         const gchar *password)
{
    conn_t *ptr = g_new(conn_t, 1);
    ptr->hostname = g_strdup(hostname);
    ptr->port = g_strdup(port);
    ptr->password = g_strdup(password);
    ptr->canceled = FALSE;
    ptr->state = CONN_SOCKET_STATE_UNDEF;
    return ptr;
}

void
conn_free(conn_t *ptr)
{
    g_free(ptr->hostname);
    g_free(ptr->port);
    g_free(ptr->password);
    g_free(ptr);
}

