## 一个简单的Webserver

## build

```
make
```

## run

```
./server port
```

## Implementation

* 采用reactor模型，主线程监听连接，对于已连接的请求，提交到线程池处理
* 线程池维护一个任务队列，采用生产者消费者模型，使用互斥锁+信号量进行同步

* 采用IO多路复用技术，使用epoll监听端口，epoll设置为ET+EPOLLONESHOT
* 当epoll监听到EPOLLIN 或者 EPOLLOUT时，由于采用ET，所以需要一次性将数据读写完
* 使用状态机模型解析http报文，解析完http报文后，对于要访问的资源，建立内存映射
* 使用聚集写writev将响应数据写入文件描述符
* 目前仅支持http中的GET请求