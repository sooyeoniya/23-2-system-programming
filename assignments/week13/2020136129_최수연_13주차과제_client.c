#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <memory.h>
#include <stdbool.h>

#define MAX_SIZE 1024 // 배열 최대 크기
#define MAX_CLIENTS 10 // 최대 클라이언트 수
#define SV_FIFO "./sv_fifo" // 서버 fifo 파일
#define CL_FIFO "./cl_fifo_%d" // 클라이언트 fifo 파일 + %d는 클라이언트 pid 넣을 값

int sv_fifo_fd, cl_fifo_fd; // 파일 디스크립터
int access_flag = false; // 접속 확인 여부
char cl_fifo_path[512]; // 클라이언트 fifo 파일 경로
pid_t client_pid; // 클라이언트 pid

void send()
{
	while (1)
	{
		if (!access_flag) { // 클라이언트가 접속했을 때 서버로 클라이언트 정보 보냄
			char message[MAX_SIZE];
			sprintf(message, "access %s", cl_fifo_path); // 파일경로에 "access" 문자열 붙임
			access_flag = true; // 해당 클라이언트의 접속 여부 true

			// 서버 fifo 파일 열고 message 작성 후 닫기
			if ((sv_fifo_fd = open(SV_FIFO, O_WRONLY)) == -1) {
				perror("open1");
			}

			if (write(sv_fifo_fd, message, sizeof(message)) == -1) {
				perror("write1");
			}

			if (close(sv_fifo_fd) == -1) {
				perror("close1");
			}
		}
		else { // 서버에 메시지 전송
			char message[MAX_SIZE];
			char temp[MAX_SIZE - 100];

			// 서버에 보낼 메시지 입력
			if (fgets(temp, sizeof(temp), stdin) == NULL) {
				continue;
			}

			temp[strcspn(temp, "\n")] = 0; // \n 위치에 널문자(\0) 대입
			snprintf(message, sizeof(message), "send %d %s", client_pid, temp); // 서버로 보낼 메시지 매핑("send" + 클라이언트 pid + 메시지)

			// 서버 fifo 파일 열고 message 작성 후 닫기
			if ((sv_fifo_fd = open(SV_FIFO, O_WRONLY)) == -1) {
				perror("open2");
			}

			if (write(sv_fifo_fd, message, sizeof(message)) == -1) {
				perror("write2");
			}

			if (close(sv_fifo_fd) == -1) {
				perror("close2");
			}
		}
	}
}

void receive()
{
	while (1)
	{
		char message[MAX_SIZE];

		// 해당 클라이언트 fifo 파일 읽기 전용으로 열어 읽기
		if ((cl_fifo_fd = open(cl_fifo_path, O_RDONLY)) != -1) {
			ssize_t bytes_read;
			
			// 서버로부터 받은 메시지 화면에 출력
			if ((bytes_read = read(cl_fifo_fd, message, sizeof(message))) > 0) {
				message[bytes_read] = '\0'; // 마지막 널문자 대입(종료)
				printf("\nReceived message: %s\n", message);
			}

			if (close(cl_fifo_fd) == -1) {
				perror("close3");
			}

		}
		else {
			perror("open3");
		}
	}
}

int main(int argc, char** argv)
{
	client_pid = getppid(); // 클라이언트 pid 생성
	sprintf(cl_fifo_path, CL_FIFO, client_pid); // 클라이언트 파일 경로 생성

	// 클라이언트 파일 경로로 fifo 파일 생성
	if (mkfifo(cl_fifo_path, 0666) == -1) {
		perror("mkfifo");
	}

	printf("Client =====\n");

	if (fork() == 0) receive(); // 자식 프로세스는 메시지 받는 역할
	else send(); // 부모 프로세스는 입력 메시지 전달 역할

	unlink(cl_fifo_path);
	return 0;
}