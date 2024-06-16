#include <iostream>
#include <WinSock2.h>//ipv4套接字
//#include <WS2tcpip.h>//ivp6套接字
#include <string>//string
#include <fstream>//文件流
#include <Windows.h>//windows api


#pragma comment(lib,"WS2_32.lib")//套接字静态库

#define d_get __FUNCTION__//获取当前函数名
//函数声明
void Exception(const char* get, const char* information);//异常错误
SOCKET Initialization(short* port);//初始化套接字
void Typeface(int choose, const char* str1);//字体颜色
DWORD WINAPI Thread(void* client);//多线程函数
int Read_data(SOCKET client_socket, char* p_buff);//获取客户端响应包
void Analysis(SOCKET client_socket, char* p_buff);//解析并发送客户指定文件
int Sending(SOCKET client_socket, char* p_catalogue, char* p_document);//发送用户指定文件
int Response(SOCKET client_socket, unsigned long long* file_siz, char* p_catalogue, char* p_document);//发送响应包

//函数重载
void Typeface(int choose, const char* str1, const char* str2);
void Typeface(int choose, const char* str1, const char* str2, const char* str3);


int main() {
	short port = 80;
	SOCKET server_socket = Initialization(&port);
	Typeface(1, "正在监听", std::to_string(port).c_str(), "端口");
	//std::to_string(port).c_str()是将port转换为string再转换为char类型
	while (TRUE) {
		sockaddr_in client_addr{ 0 };
		int client_addr_len = sizeof(client_addr);
		SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
		if (client_socket == INVALID_SOCKET) {
			Typeface(0, "client套接字创建失败");
		}
		else {
			DWORD thread_id;
			HANDLE exception = NULL;
			exception = CreateThread(NULL, 0, Thread, (void*)client_socket, 0, &thread_id);
			if (exception == NULL) {
				closesocket(client_socket);
			}
			else {
				CloseHandle(exception);
			}
		}
	}

}
//多线程
DWORD WINAPI Thread(void* client) {
	char buff[1024] = { 0 };
	int exception = 0;
	SOCKET client_socket = (SOCKET)client;
	Typeface(1, std::to_string(client_socket).c_str(), ":正在连接");
	exception = Read_data(client_socket, buff);
	if (exception < 0) { Typeface(0, std::to_string(client_socket).c_str(), ":用户响应包解析失败"); return -1; }
	Analysis(client_socket, buff);
	shutdown(client_socket, SD_RECEIVE);
	closesocket(client_socket);
	return 0;
}
//解析函数
void Analysis(SOCKET client_socket, char* p_buff) {
	//GET / HTTP / 1.1\r\n
	int i = 0, j = 0;
	char catalogue[512] = { 0 };
	char document[512] = { 0 };
	int exception = 0;
	//默认参数
	char default_catalogue[] = "html";
	char default_document[] = "index.html";
	//跳过GET
	while (!isspace(p_buff[i]) && i < sizeof(p_buff) - 1) { i++; }
	//跳过空格
	while (isspace(p_buff[i]) && i < sizeof(p_buff)) { i++; }

	if (p_buff[i] == '/') {
		i++;
		while (p_buff[i] != '/' && p_buff[i] != ' ') {
			catalogue[j++] = p_buff[i++];
		}
		if (catalogue[0] == '\0') {
			//没有输入
			exception = Sending(client_socket, default_catalogue, default_document);
			if (exception < 0) {
				return ;
			}
		}
		else if (catalogue[0] == '?') {
			//解析?后用户返回的url信息
			//现在还没写
			exception = Sending(client_socket, default_catalogue, default_document);
			if (exception < 0) {
				return;
			}
		}
		else {
			j = 0;
			i++;
			while (p_buff[i] != '/' && p_buff[i] != ' ') {
				document[j++] = p_buff[i++];
			}
			if (document[0] == '\0') {
				//只输了目录没有具体文件
				exception = Sending(client_socket, default_catalogue, default_document);
				if (exception < 0) {
					return ;
				}
			}
			else {
				//有输入
				exception = Sending(client_socket, catalogue, document);
				if (exception < 0) {
					return ;
				}
			}
		}
	}
}
//发送请求文件
int Sending(SOCKET client_socket, char* p_catalogue, char* p_document) {
	char buff[1024] = { 0 };
	char addr[2048] = "A:/Document/";
	char slash[] = "/";
	//连接
	strcat_s(addr, sizeof(addr), p_catalogue);
	strcat_s(addr, sizeof(addr), (const char*)slash);
	strcat_s(addr, sizeof(addr), p_document);
	std::ifstream file;
	file.open(addr, std::ios::binary);
	//int a = file.is_open();
	//获取文件大小
	file.seekg(0, std::ios::end);
	unsigned long long file_siz = file.tellg();
	file.seekg(0, std::ios::beg);
	//发送响应文件
	int exception = Response(client_socket, &file_siz, p_catalogue, p_document);
	if (exception < 0) {
		Typeface(0, std::to_string(client_socket).c_str(), ":用户指定文件错误");
		return -1;
	}
	//正式发送文件
	while (TRUE) {
		file.read(buff, sizeof(buff));
		if (file.gcount() <= 0) {//判断文件是否读完
			break;
		}
		send(client_socket, buff, file.gcount(), 0);
	}
	file.close();
	Typeface(2, std::to_string(client_socket).c_str(), ":发送完毕");
	return 0;
}
//发送响应包
int Response(SOCKET client_socket, unsigned long long* file_siz, char* p_catalogue, char* p_document) {
	char buff[2048] = { 0 };
	if (strcmp(p_catalogue, "html") == 0) {
		sprintf_s(buff, "HTTP/1.1 200 OK\r\n"
			"Server: yangye308/0.1\r\n"
			"Content-type: text/html\r\n"
		    "\r\n");
		send(client_socket, buff, strlen(buff), 0);
		return 0;
	}
	else if (strcmp(p_catalogue, "fix") == 0) {
		sprintf_s(buff,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: application/octet-stream\r\n"
			"Content-Disposition: attachment; filename=\"%s\"\r\n"
			"Content-Length: %llu\r\n"
			"Connection: close\r\n"
			"\r\n",
			p_document, *file_siz);
		send(client_socket, buff, strlen(buff), 0);
		return 0;
	}
	else if (strcmp(p_catalogue, "picture") == 0) {
		sprintf_s(buff,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: image/jpeg\r\n"
			"Content-Disposition: attachment; filename=\"%s\"\r\n"
			"Content-Length: %llu\r\n"
			"Connection: close\r\n"
			"\r\n",
			p_document, *file_siz);
		send(client_socket, buff, strlen(buff), 0);
		return 0;
	}
	else {
		return -1;
	}
}
//获取请求
int Read_data(SOCKET client_socket, char* p_buff) {
	char buff[2048]{ 0 };
	recv(client_socket, buff, sizeof(buff) - 1, 0);
	int index = 0;
	if (strcmp(buff, "\0") == 0) {
		return -1;
	}
	while (index < sizeof(buff) - 1 && buff[index] != '\n') {
		p_buff[index] = buff[index];
		index++;
	}
	p_buff[index] = '\n';
	return 0;
}


SOCKET Initialization(short* port) {
	//初始化套接字
	WSADATA was;
	short exception = WSAStartup(MAKEWORD(2, 2), &was);
	if (exception != 0) { Exception(d_get, "套接字初始化失败"); }
	//设置套接字类型
	SOCKET server_socket = { 0 };
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//设置套接字地址族
	sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//绑定套接字
	exception = bind(server_socket, (const sockaddr*)&server_addr, sizeof(server_addr));
	if (exception < 0) { Exception(d_get, "绑定套接字失败"); }
	//监听套接字
	exception = listen(server_socket, 5);
	if (exception < 0) { Exception(d_get, "监听套接字失败"); }
	return server_socket;
}
//异常处理
void Exception(const char* get, const char* information) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
	std::cout << get;
	perror(information);
	exit(1);
}
//选择颜色
int Choose(int* choose) {
	enum { RED, GREEN, BLUE };
	switch (*choose) {
	case 0:return FOREGROUND_RED;//FOREGROUND_RED红色
	case 1:return FOREGROUND_GREEN;//FOREGROUND_GREEN绿色
	case 2:return FOREGROUND_BLUE;//FOREGROUND_BLUE蓝色
	default:return -1;
	}
}

void Typeface(int choose, const char* str1) {
    short color = Choose(&choose);
    if (color < 0) { Exception(d_get, "未知颜色"); }
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(Console, color);
    std::cout << str1 << std::endl;
    SetConsoleTextAttribute(Console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);//回到白色
}

//函数重载
void Typeface(int choose, const char* str1, const char* str2) {
    short color = Choose(&choose);
    if (color < 0) { Exception(d_get, "未知颜色"); }
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(Console, color);
    std::cout << str1 << str2 << std::endl;
    SetConsoleTextAttribute(Console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);//回到白色
}

void Typeface(int choose, const char* str1, const char* str2, const char* str3) {
    short color = Choose(&choose);
    if (color < 0) { Exception(d_get, "未知颜色"); }
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(Console, color);
    std::cout << str1 << str2 << str3 << std::endl;
    SetConsoleTextAttribute(Console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);//回到白色
}