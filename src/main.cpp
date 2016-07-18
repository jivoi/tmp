//#include <sys/types.h>
//#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <map>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <thread>
#include <fstream>



#define MAX_REQUEST_SIZE 1024


std::string http_parser(std::string request) {
    std::stringstream ss(request);
    std::string temp;
    ss >> temp;
    ss >> temp;
    size_t pos =  temp.find("html?");
    if (pos != std::string::npos)
        return temp.substr(1, pos + 4);
    else return temp.substr(1);
}

static const char* templ = "HTTP/1.0 200 OK\r\n"

                           "Content-length: %d\r\n"

                           "Connection: close\r\n"

                           "Content-Type: text/html\r\n""\r\n""%s";

static const char not_found[] = "HTTP/1.0 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n";

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch (sa->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                  s, maxlen);
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                  s, maxlen);
        break;

    default:
        strncpy(s, "Unknown AF", maxlen);
        return NULL;
    }

    return s;
}


int set_nonblock(int fd) {
    int flags;
#ifdef O_NONBLOCK
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK); //posix standartized
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);   // unix, may change its behavior depend on  OS and/or type of file descriptor
#endif

}

void worker(int SlaveSocket)
{
    char Buffer[1024];
    int size = recv(SlaveSocket, Buffer, 1024, MSG_NOSIGNAL);
    Buffer[size] = '\n';

    std::string name = http_parser(std::string(Buffer));
    std::ifstream ifs(name.c_str());
    if (ifs.good()) {
        std::string content( (std::istreambuf_iterator<char>(ifs) ), (std::istreambuf_iterator<char>()    ) );
        sprintf(Buffer, templ, content.length(), content.c_str());
        int result = send(SlaveSocket, Buffer, strlen(Buffer), MSG_NOSIGNAL);
    } else {
        int result = send(SlaveSocket, not_found, strlen(not_found), MSG_NOSIGNAL);
    }
    shutdown(SlaveSocket, SHUT_RDWR);
    close(SlaveSocket);
}

int main(int argc, char *argv[]) {
    int par = 0;
    char * ip;
    int port;
    char * dir;
    // if (argc != 7) {
    //     printf("Error num args!\n");
    //     return -2;
    // }

    while ( (par = getopt(argc, argv, ":h:p:d:")) != -1) {
        switch (par)
        {
        case 'h':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            dir = optarg;
            break;
        default:
            printf("Error!\n");
            return -3;
        };
    }
    int   pid = fork();

    if (pid == -1)
    {
        printf("Error: Start Daemon failed (%s)\n", strerror(errno));
        return -1;
    }
    else if (!pid) // potomok
    {
        setsid();
        chdir(dir);

        //we don't need it
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        int MasterSocket = socket( /* listen */
                               AF_INET/* IPv4*/,
                               SOCK_STREAM/* TCP */,
                               IPPROTO_TCP);
        struct sockaddr_in SockAddr;
        SockAddr.sin_family = AF_INET;
        SockAddr.sin_port = htons(port);
        SockAddr.sin_addr.s_addr = htons(INADDR_ANY); //0.0.0.0
        bind(MasterSocket, (struct sockaddr*)(&SockAddr), sizeof(SockAddr));
        //set_nonblock(MasterSocket);

        listen(MasterSocket, SOMAXCONN);

        while (1) {
            int SlaveSocket  = accept(MasterSocket, 0, 0);
            std::thread thr(worker, SlaveSocket);
            thr.detach();
        }
    }
    else // parent
    {
        return 0;
    }

    return 0;
}