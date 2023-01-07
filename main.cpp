#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "locker.h"
#include "threadpool.h"
#include <sys/epoll.h>
#include <signal.h>
#include "http_conn.h"

#define MAX_FD 65535 
#define MAX_EVENT_NUMBER 10000

//添加信号捕捉
void addsig(int sig, void(handler)(int)){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

//添加文件描述符到epoll
extern void addfd(int epollfd, int fd, bool one_shot);
//从epoll中删除文件描述符
extern void removefd(int epollfd, int fd);

extern void modifyfd(int epollfd, int fd, int ev);

int main(int argc, char *argv[]){
    if(argc <= 1){
        printf("%s port_number\n", basename(argv[0]));
        exit(-1);
    }
    //获取端口号
    int port = atoi(argv[1]);
    
    //处理SIGPIPE信号
    
    addsig(SIGPIPE, SIG_IGN);
    
    //创建线程池，使用
    threadpool<http_conn> * pool = NULL;
    try{
        pool = new threadpool<http_conn>;
    }catch(...){
        exit(-1);
    }

    // 创建一个数组用于保存所有的客户信息

    http_conn *users = new http_conn[MAX_FD];

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    //设置端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    //绑定
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bind(listenfd, (struct sockaddr *)&address, sizeof(address));

    //监听
    listen(listenfd, 5);

    //创建epoll对象，事件数组, 添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    addfd(epollfd, listenfd, false);

    http_conn::m_epollfd = epollfd;
    http_conn::m_user_count = 0;

    //循环检测有哪些事件发生
    while(true){
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(num < 0 && (errno != EINTR)){
            printf("epoll failure\n");
            break;
        }
        //循环遍历事件数组
        for(int i = 0; i < num; i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){//有客户端连接进来
                struct sockaddr_in clientaddr;
                socklen_t clientaddr_len = sizeof(clientaddr);
                int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddr_len);
                if(http_conn::m_user_count >= MAX_FD){
                    //todo: 给客户端回写信息，服务器内部正忙
                    close(connfd);
                    continue;
                }
                //新的客户的数据初始化放入user数组中，并设置监听
                users[connfd].init(connfd, clientaddr);
            } else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //对方异常，关闭连接
                users[sockfd].close_conn();
            } else if(events[i].events & EPOLLIN){
                if(users[sockfd].read()){//读取完数据
                    pool->append(users + sockfd);//将任务添加到线程池
                }else{
                    users[sockfd].close_conn();
                }
            }else if(events[i].events & EPOLLOUT){
                if(!users[sockfd].write()){//写完所有的数据
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}