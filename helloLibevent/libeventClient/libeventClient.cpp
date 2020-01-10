// libeventClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <io.h>
#include <event.h>
#include <winsock2.h>

int tcp_connect_server(const char* server_ip, int port);

void cmd_msg_cb(evutil_socket_t fd, short events, void* arg);
void socket_read_cb(evutil_socket_t fd, short events, void* arg);

int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("please input 2 parameter.\n");
		return -1;
	}

	
	int err;
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(1, 1);
	err = ::WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		perror("WSAStartup error.\n");
		return -1;
	}


	int sockfd = tcp_connect_server(argv[1], atoi(argv[2]));
	if (-1 == sockfd) {
		perror("tcp_connect_server error.\n");
		return -1;
	}

	printf("connect to server successful.\n");

	struct event_base* base = event_base_new();
	struct event* ev_sockfd = event_new(base, sockfd, 
		EV_READ | EV_PERSIST, socket_read_cb, NULL);
	event_add(ev_sockfd, NULL);
	
	// 监听终端输入事件
	struct event* ev_cmd = event_new(base, 0,
		EV_READ | EV_PERSIST, cmd_msg_cb, (void*)&sockfd);
	event_add(ev_cmd, NULL);
	
	event_base_dispatch(base);

	printf("finished.\n");

	::WSACleanup();

	return 0;
}

void cmd_msg_cb(evutil_socket_t fd, short events, void* arg)
{
	DWORD dwBytesRead, dwBytesWrite;
	char msg[1024] = { 0 };
/*	int ret = _read(fd, msg, sizeof(msg));
	if (ret <= 0) {
		perror("read fail ");
		exit(1);
	}
	*/
	int len = ReadFile((HANDLE)fd, msg, sizeof(msg) - 1, &dwBytesRead, NULL);
	if (len && 0 == dwBytesRead) {
		fprintf(stderr, "End of File");
		return;
	}

	if (dwBytesRead > 0) {
		int sockfd = *((int*)arg);
		//_write(sockfd, msg, ret);
		msg[dwBytesRead] = '\0';
		WriteFile((HANDLE)fd, msg, strlen(msg), &dwBytesWrite, NULL);
	}
}

void socket_read_cb(evutil_socket_t fd, short events, void* arg)
{
	DWORD dwBytesRead, dwBytesWrite;
	char msg[1024] = { 0 };
	
/*	int len = _read(fd, msg, sizeof(msg) - 1);
	if (len <= 0) {
		perror("read fail ");
		exit(2);
	}*/
	int len = ReadFile((HANDLE)fd, msg, sizeof(msg) - 1, &dwBytesRead, NULL);
	if (len && 0 == dwBytesRead) {
		fprintf(stderr, "End of File");
		return;
	}

	if (dwBytesRead > 0) {
		msg[dwBytesRead] = '\0';
		int sockfd = *((int*)arg);
		printf("recv %s from server\n", msg);
	}

}

typedef struct sockaddr SA;
int tcp_connect_server(const char* server_ip, int port)
{
	int sockfd, status, save_errno;
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	//server_addr.sin_addr.S_un.S_addr = inet_addr(server_ip);
	status = ::evutil_inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
	if (-1 == status) {
		errno = EINVAL;
		return -1;
	}
	
	sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		return sockfd;
	}

	status = ::connect(sockfd, (SA*)&server_addr, sizeof(server_addr));
	if (-1 == status) {
		save_errno = errno;
		::_close(sockfd);
		errno = save_errno;
		return -2;
	}

	evutil_make_socket_nonblocking(sockfd);

	return sockfd;
}