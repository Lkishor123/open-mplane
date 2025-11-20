#include "sim/alarm_ipc.h"
#include "MplaneAlarms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

// Unix socket for alarm IPC
#define ALARM_SOCKET_PATH "/tmp/mplane_alarm_socket"

// Module state
static pthread_t g_alarm_listener_thread;
static volatile int g_alarm_listener_running = 0;
static halmplane_oran_alarm_cb_t g_alarm_cb = NULL;

// Alarm message structure for IPC
struct alarm_ipc_msg {
    uint16_t fault_id;
    char fault_source[64];
    int severity;
    int is_cleared;
    char fault_text[256];
    uint64_t fault_time;
};

// Socket listener thread function
static void* alarm_socket_listener(void* arg) {
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_un addr;
    struct alarm_ipc_msg msg;

    // Create socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("alarm_socket_listener: socket");
        return NULL;
    }

    // Remove old socket file if it exists
    unlink(ALARM_SOCKET_PATH);

    // Bind socket
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ALARM_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("alarm_socket_listener: bind");
        close(server_fd);
        return NULL;
    }

    // Listen
    if (listen(server_fd, 5) < 0) {
        perror("alarm_socket_listener: listen");
        close(server_fd);
        unlink(ALARM_SOCKET_PATH);
        return NULL;
    }

    // Make socket accessible by all users
    chmod(ALARM_SOCKET_PATH, 0666);

    fprintf(stderr, "[alarm_ipc] Listening on %s\n", ALARM_SOCKET_PATH);

    g_alarm_listener_running = 1;

    while (g_alarm_listener_running) {
        // Accept connection with timeout
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("alarm_socket_listener: select");
            break;
        }
        if (ret == 0) continue;  // Timeout, check if still running

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("alarm_socket_listener: accept");
            continue;
        }

        // Read alarm message
        ssize_t n = recv(client_fd, &msg, sizeof(msg), 0);
        if (n == sizeof(msg)) {
            // Call the registered callback
            if (g_alarm_cb) {
                halmplane_oran_alarm_t alarm;
                memset(&alarm, 0, sizeof(alarm));
                alarm.fault_id = msg.fault_id;
                alarm.fault_source = msg.fault_source;
                alarm.fault_severity = (halmplane_oran_fault_severity_t)msg.severity;
                alarm.is_cleared = msg.is_cleared;
                alarm.fault_text = msg.fault_text;
                alarm.fault_time = msg.fault_time;
                alarm.event_time = msg.fault_time;

                g_alarm_cb(&alarm, NULL);

                // Send acknowledgment
                const char* ack = "OK";
                send(client_fd, ack, strlen(ack), 0);
            } else {
                // No callback registered
                const char* err = "ERR:no_callback";
                send(client_fd, err, strlen(err), 0);
            }
        }

        close(client_fd);
    }

    close(server_fd);
    unlink(ALARM_SOCKET_PATH);
    fprintf(stderr, "[alarm_ipc] Stopped\n");
    return NULL;
}

// Public API implementation

int alarm_ipc_init(void) {
    if (g_alarm_listener_running) {
        return 0;  // Already running
    }

    if (pthread_create(&g_alarm_listener_thread, NULL, alarm_socket_listener, NULL) != 0) {
        perror("alarm_ipc_init: pthread_create");
        return 1;
    }

    // Detach thread so it cleans up automatically
    pthread_detach(g_alarm_listener_thread);

    return 0;
}

void alarm_ipc_cleanup(void) {
    if (g_alarm_listener_running) {
        g_alarm_listener_running = 0;
        // Thread will exit on next select timeout (1 second max)
    }
    g_alarm_cb = NULL;
}

void alarm_ipc_set_callback(halmplane_oran_alarm_cb_t cb) {
    g_alarm_cb = cb;
}

int alarm_ipc_send_alarm(
    uint16_t fault_id,
    const char* fault_source,
    int severity,
    bool is_cleared,
    const char* fault_text) {

    // Send alarm via Unix socket to the server process
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("alarm_ipc_send_alarm: socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ALARM_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("alarm_ipc_send_alarm: connect");
        close(sock_fd);
        return 1;
    }

    // Prepare alarm message
    struct alarm_ipc_msg msg;
    memset(&msg, 0, sizeof(msg));
    msg.fault_id = fault_id;
    strncpy(msg.fault_source, fault_source ? fault_source : "SIM", sizeof(msg.fault_source) - 1);
    msg.severity = severity;
    msg.is_cleared = is_cleared;
    strncpy(msg.fault_text, fault_text ? fault_text : "sim alarm", sizeof(msg.fault_text) - 1);
    msg.fault_time = (uint64_t)time(NULL);

    // Send message
    if (send(sock_fd, &msg, sizeof(msg), 0) != sizeof(msg)) {
        perror("alarm_ipc_send_alarm: send");
        close(sock_fd);
        return 1;
    }

    // Wait for acknowledgment
    char ack[32];
    ssize_t n = recv(sock_fd, ack, sizeof(ack) - 1, 0);
    if (n > 0) {
        ack[n] = '\0';
    }

    close(sock_fd);
    return 0;
}

bool alarm_ipc_is_running(void) {
    return g_alarm_listener_running != 0;
}
