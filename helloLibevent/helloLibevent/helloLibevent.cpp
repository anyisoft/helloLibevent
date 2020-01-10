// helloLibevent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <io.h>
#include <event.h>

void accept_cb(evutil_socket_t fd, short events, void* arg);
void socket_read_cb(evutil_socket_t fd, short events, void* arg);
int tcp_server_init(int port, int listen_num);

int main()
{
	int err;
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(1, 1);
	err = ::WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		perror("WSAStartup error.\n");
		return -1;
	}

	int listener = tcp_server_init(4931, 10);
	if (-1 == listener) {
		perror("tcp_server_init error");
		return -2;
	}

	struct event_base* base = event_base_new();

	struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST, accept_cb, base);

	event_add(ev_listen, NULL);
	printf("libevent server startup finished.\n");
	event_base_dispatch(base);

	::WSACleanup();

	return 0;
}

void accept_cb(evutil_socket_t fd, short events, void* arg)
{
	evutil_socket_t sockfd;

	struct sockaddr_in client;
	ev_socklen_t len = sizeof(client);

	sockfd = ::accept(fd, (struct sockaddr*) & client, &len);
	evutil_make_socket_nonblocking(sockfd);

	printf("accept a client %I64d\n", sockfd);

	struct event_base* base = (event_base*)arg;

	struct event* ev = event_new(NULL, -1, 0, NULL, NULL);
	event_assign(ev, base, sockfd, EV_READ | EV_PERSIST, socket_read_cb, (void*)ev);

	event_add(ev, NULL);
}

void socket_read_cb(evutil_socket_t fd, short events, void* arg)
{
	DWORD dwBytesRead, dwBytesWrite;
	char msg[4096] = { 0 };
	struct event* ev = (struct event*)arg;
/*	int len = _read(fd, msg, sizeof(msg) - 1);
	if (len <= 0) {
		printf("some error happen when read\n");
		event_free(ev);
		_close(fd);
		return;
	}
	msg[len] = '\0';
	*/
	int len = ReadFile((HANDLE)fd, msg, sizeof(msg) - 1, &dwBytesRead, NULL);
	if (len && 0 == dwBytesRead) {
		fprintf(stderr, "End of File");
		event_del(ev);
		return;
	}

	if (dwBytesRead > 0) {
		msg[dwBytesRead] = '\0';
		printf("recv the client msg: %s\n", msg);

		char reply_msg[4096] = "I have receive the msg: ";
		strcat_s(reply_msg + strlen(reply_msg), strlen(reply_msg), msg);

		//_write(fd, reply_msg, strlen(reply_msg));
		WriteFile((HANDLE)fd, reply_msg, strlen(reply_msg), &dwBytesWrite, NULL);
	}
}

typedef struct sockaddr SA;
int tcp_server_init(int port, int listen_num)
{
	int errno_save;
	evutil_socket_t listener;

	listener = ::socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listener) {
		return -1;
	}

	evutil_make_listen_socket_reuseable(listener);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(port);

	if (::bind(listener, (SA*)&sin, sizeof(sin)) < 0) {
		goto error;
	}

	if (::listen(listener, listen_num) < 0) {
		goto error;
	}

	evutil_make_socket_nonblocking(listener);

	return listener;

error:
	errno_save = errno;
	evutil_closesocket(listener);
	errno = errno_save;

	return -1;
}
