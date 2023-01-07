#ifndef HTTPCONNECTION
#define HTTPCONNECTION

#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>
#include <stdlib.h>

class http_conn{
public:

    static int m_epollfd; //所有的socket
    static int m_user_count; //统计用户数目
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    static const int FILENAME_LEN = 200;//文件名最大长度

    http_conn(){}
    ~http_conn(){}
    void process();//处理客户端的请求
    void init(int connfd, const sockaddr_in & addr);//初始化新的连接
    void close_conn();//关闭连接
    bool read();//非阻塞读，一次性读完所有数据
    bool write();//非阻塞写
    void unmap();

    //http请求方法
    enum METHOD {
        GET = 0,
        POST,
        HEAD, 
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT
    };
    //解析客户端请求时，主状态机的状态
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    // 从状态机读取到的三种可能的状态
    //1. 读取到一个完整的行 
    //2. 行出错
    //3. 行数据尚且不完整
    enum LINE_STATUS{
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    //http状态码，返回HTTP请求的可能结果
    enum HTTP_CODE{
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FILE_REQUEST,
        FORBIDDEN_REQUEST,
        INTERNAL_ERROR, 
        CLOSED_CONNECTION 
    };
public:
    HTTP_CODE process_read();   //解析http请求
    HTTP_CODE parse_request_line(char *text);//解析请求行
    HTTP_CODE parse_headers(char *text);//解析请求头
    HTTP_CODE parse_content(char *text);//解析请求体
    HTTP_CODE do_request(); //处理请求
    LINE_STATUS parse_line();//解析单行
public:
    bool process_write(HTTP_CODE ret);//填充http应答
    bool add_response(const char *format, ...);
    bool add_status_line(int status, const char * title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();
    bool add_content(const char * content);
    bool add_content_type();

private:
    void init();
    int m_sockfd; //http连接的socket
    struct sockaddr_in m_address;//通信的socket地址

    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;//下一次要读取的位置
    int m_parsed_index;//已经解析完的位置
    int m_start_line;

    char * m_url;//请求目标文件的文件名
    char * m_version;//协议版本
    METHOD m_method;//请求方法
    char * m_host;//主机名
    bool m_linger;//判断http请求是否要保持连接
    int m_content_length;//请求体长度，从请求头获得
    char m_real_file[FILENAME_LEN];//请求资源的绝对路径
    struct stat m_file_stat;//目标文件的状态
    char *m_file_address;//客户请求的目标文件被mmap到内存中的起始位置

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    struct iovec m_iv[2];//使用writev执行写操作，m_iv[0]写响应行+响应头，m_iv[1]写响应体
    int m_iv_count;
    int byte_to_send; //将要发送的数据
    int bytes_have_send;//已经发送的数据
    CHECK_STATE m_check_state;
    char * getline() {
        return m_read_buf + m_start_line;
    }
};

#endif