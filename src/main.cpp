#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <set>
#include <algorithm>
#include <sys/epoll.h>
#include <thread>
#include <string.h>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define MAX_EVENTS 32
#define MAX_ARG 24


char *DIRECTORY = NULL;


int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

void f(int fd) {
    std::cout << "from thread: " << fd << std::endl;

    // Slave socket.
    // Receive a message and then send the message back.
//  static char Buffer[1024];
    char Buffer[1024] = {0};

    // Receive a message.
    int RecvSize = (int) recv(
                       fd,
                       Buffer,
                       1024,
                       MSG_NOSIGNAL
                   );

    std::stringstream ss_file_name(Buffer);

    std::string token = "";
    std::string path = "";

    while (ss_file_name >> token) {
//    http_request.push_back(token);

        std::size_t found = token.find("GET");

        if (found != std::string::npos) {
//      std::cout << found <<'\n';

            continue;
        }

        path = token;
        break;

        printf("%s\n", token.c_str());
    }

    std::size_t found = path.find('?');

    if (found != std::string::npos) {
        std::cout << found << '\n';

        path = path.substr (0, found);
    }

    std::cout << "path = " << path << '\n';


    std::stringstream ss;


    FILE *file_in = NULL;
    char buff[255] = {0};

    std::string file_in_name = DIRECTORY;
//  file_in_name += "/index.html";

    file_in_name += path;
    size_t size = 0;

    // Read from a file.
    file_in = fopen(file_in_name.c_str(), "r");

    if (file_in) {
        std::string tmp;
        fgets(buff, 255, file_in);

        tmp += buff;

//  std::string tmp = "<b>Hello world!</b>";

        fclose(file_in);

        ss << "HTTP/1.0 200 OK";
        ss << "\r\n";
        ss << "Content-length: ";
        ss << tmp.size();
        ss << "\r\n";
        ss << "Content-Type: text/html";
        ss << "\r\n\r\n";
        ss << tmp;

        printf("ss = %s", ss.str().c_str());

        size = ss.str().size();

        strncpy(Buffer, ss.str().c_str(), size);
    } else {
        ss << "HTTP/1.0 404 NOT FOUND";
        ss << "\r\n";
        ss << "Content-length: ";
        ss << 0;
        ss << "\r\n";
        ss << "Content-Type: text/html";
        ss << "\r\n\r\n";

        printf("ss = %s", ss.str().c_str());

        size = ss.str().size();

        strncpy(Buffer, ss.str().c_str(), size);
    }

    // In case of an error close the connection.
    // Otherwise send a received message back.
    if ((RecvSize == 0) && (errno != EAGAIN)) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    } else if (RecvSize > 0) {
//    send(fd, Buffer, RecvSize, MSG_NOSIGNAL);

        send(fd, Buffer, size, MSG_NOSIGNAL);
    }
}

int main(int argc, char **argv) {
    int opt;

    char *ip = new char[MAX_ARG];
    int port = 12345;
//  char *directory = new char[MAX_ARG];
    DIRECTORY = new char[MAX_ARG];

    //  -h<ip> -p<port> -d<directory>
    while ((opt = getopt(argc, argv, "h:p:d:")) != -1) {
        switch (opt) {
        case 'h':
            strncpy(ip, optarg, MAX_ARG);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            strncpy(DIRECTORY, optarg, MAX_ARG);
            break;
        default:
            fprintf(stderr, "Usage: %s -h<ip> -p<port> -d<directory>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("ip=%s; port=%d; directory=%s;\n",
           ip, port, DIRECTORY);

    // For demon.
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (pid == 0) {
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    int MasterSocket = socket(AF_INET,
                              SOCK_STREAM,
                              IPPROTO_TCP);

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons((uint16_t) port);
    SockAddr.sin_addr.s_addr = inet_addr(ip);

    int enable = 1;
    if (setsockopt(MasterSocket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &enable,
                   sizeof(int)) < 0) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    bind(MasterSocket,
         (struct sockaddr *) &SockAddr,
         sizeof(SockAddr));

    set_nonblock(MasterSocket);

    listen(MasterSocket, SOMAXCONN);

    int Epoll = epoll_create1(0);

    struct epoll_event Event;

    // Add master socket to epoll.
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN;

    epoll_ctl(Epoll, EPOLL_CTL_ADD, MasterSocket, &Event);

    // Loop.
    while (true) {
        struct epoll_event Events[MAX_EVENTS];

        int N = epoll_wait(Epoll, Events, MAX_EVENTS, -1);

        if (N == -1) {
            std::cout << strerror(errno) << std::endl;
            return 1;
        }


        for (unsigned int i = 0; i < N; i++) {
            if (Events[i].data.fd == MasterSocket) {
                // Master socket.
                // Accept a connection.
                int SlaveSocket = accept(MasterSocket, 0, 0);
                set_nonblock(SlaveSocket);

                // Add slave socket to epoll.
                struct epoll_event SlaveEvent;
                SlaveEvent.data.fd = SlaveSocket;
                SlaveEvent.events = EPOLLIN;
                epoll_ctl(Epoll, EPOLL_CTL_ADD, SlaveSocket, &SlaveEvent);
            } else {
                // Slave socket.
                // Receive a message and then send the message back.
                int fd = Events[i].data.fd;

                // Multithreading mode.
                epoll_ctl(Epoll, EPOLL_CTL_DEL, fd, 0);
                std::thread t(f, fd);
                t.detach();

                // Serial mode.
//        f(fd);
            }
        }
    }

    return 0;
}

#pragma clang diagnostic pop