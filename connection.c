#define _WIN32_WINNT 0x0501
#include <string.h>
#include <stdlib.h>
#include "connection.h"
#include "gui-connect.h"
#include "tuner.h"
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
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
#endif

gint open_serial(const gchar* serial_port)
{
    gchar path[32];
#ifdef G_OS_WIN32
    DCB dcbSerialParams = {0};

    g_snprintf(path, sizeof(path), "\\\\.\\%s", serial_port);
    tuner.socket = INVALID_SOCKET;
    tuner.serial = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(tuner.serial == INVALID_HANDLE_VALUE)
    {
        return CONN_SERIAL_FAIL_OPEN;
    }
    if(!GetCommState(tuner.serial, &dcbSerialParams))
    {
        CloseHandle(tuner.serial);
        return CONN_SERIAL_FAIL_PARM_R;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(!SetCommState(tuner.serial, &dcbSerialParams))
    {
        CloseHandle(tuner.serial);
        return CONN_SERIAL_FAIL_PARM_W;
    }
#else
    struct termios options;
    g_snprintf(path, sizeof(path), "/dev/%s", serial_port);
    if((tuner.serial = open(path, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
    {
        return CONN_SERIAL_FAIL_OPEN;
    }
    fcntl(tuner.serial, F_SETFL, 0);
    tcflush(tuner.serial, TCIOFLUSH);
    if(tcgetattr(tuner.serial, &options))
    {
        close(tuner.serial);
        return CONN_SERIAL_FAIL_PARM_R;
    }
    if(cfsetispeed(&options, B115200) || cfsetospeed(&options, B115200))
    {
        close(tuner.serial);
        return CONN_SERIAL_FAIL_SPEED;
    }
    options.c_iflag &= ~(BRKINT | ICRNL | IXON | IMAXBEL);
    options.c_iflag |= IGNBRK;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | ECHOK | ECHOCTL | ECHOKE);
    options.c_oflag &= ~(OPOST | ONLCR);
    options.c_oflag |= NOFLSH;
    options.c_cflag |= CS8;
    options.c_cflag &= ~(CRTSCTS);
    if(tcsetattr(tuner.serial, TCSANOW, &options))
    {
        close(tuner.serial);
        return CONN_SERIAL_FAIL_PARM_W;
    }
#endif
    return CONN_SUCCESS;
}

gpointer open_socket(gpointer ptr)
{
    conn_t* data = (conn_t*)ptr;
    struct addrinfo hints = {0}, *result;
    struct timeval timeout = {0};
    fd_set input;
    gchar salt[SOCKET_SALT_LEN+1], *msg;
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

    data->socketfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    data->state = CONN_SOCKET_STATE_CONN;
    g_idle_add(connection_socket_callback_info, data);
    if(connect(data->socketfd, result->ai_addr, result->ai_addrlen) < 0  || data->canceled)
    {
        freeaddrinfo(result);
        data->state = CONN_SOCKET_FAIL_CONN;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }
    freeaddrinfo(result);

    FD_ZERO(&input);
    FD_SET(data->socketfd, &input);
    timeout.tv_sec = SOCKET_AUTH_TIMEOUT;
    /* Wait SOCKET_AUTH_TIMEOUT seconds for the salt */
    if(select(data->socketfd+1, &input, NULL, NULL, &timeout) <= 0  || data->canceled)
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
    if(strlen(data->password))
    {
        g_checksum_update(sha1, (guchar*)data->password, strlen(data->password));
    }
    msg = g_strdup_printf("%s\n", g_checksum_get_string(sha1));
    g_checksum_free(sha1);

    /* Send the hash */
    if(!tuner_write_socket(data->socketfd, msg, strlen(msg)) || data->canceled)
    {
        g_free(msg);
        closesocket(data->socketfd);
        data->state = CONN_SOCKET_FAIL_WRITE;
        g_idle_add(connection_socket_callback, data);
        return NULL;
    }
    g_free(msg);

    data->state = CONN_SUCCESS;
    g_idle_add(connection_socket_callback, data);
    return NULL;
}

conn_t* conn_new(const gchar* hostname, const gchar* port, const gchar* password)
{
    conn_t* ptr = g_new(conn_t, 1);
    ptr->hostname = g_strdup(hostname);
    ptr->port = g_strdup(port);
    ptr->password = g_strdup(password);
    ptr->canceled = FALSE;
    ptr->state = CONN_SOCKET_STATE_UNDEF;
    return ptr;
}

void conn_free(conn_t *ptr)
{
    g_free(ptr->hostname);
    g_free(ptr->port);
    g_free(ptr->password);
    g_free(ptr);
}
