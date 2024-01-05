#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>

#define MAX_SIZE 1024 // 배열 최대 크기
#define MAX_CLIENTS 10 // 최대 클라이언트 수
#define SV_FIFO "./sv_fifo" // 서버 fifo 파일
#define CL_FIFO "./cl_fifo_%d" // 클라이언트 fifo 파일 + %d는 클라이언트 pid 넣을 값

struct ClientInfo {
	pid_t pid; // 클라이언트 pid (클라이언트 고유 이름 역할)
	int cl_fifo_fd; // 클라이언트 fifo 파일 디스크립터
};

int main(int argc, char** argv)
{
	struct ClientInfo clients[MAX_CLIENTS] = { 0 }; // 클라이언트 정보 초기화
	int sv_fifo_fd; // 서버 fifo 파일 디스크립터
	int num_clients = 0; // 서버에 접속한 클라이언트 수
	char cl_fifo_path[MAX_CLIENTS][MAX_SIZE] = { 0 }; // 각 클라이언트별 fifo 파일 경로 초기화

	printf("Server =====\n");

	// 서버 fifo 파일 생성
	if (mkfifo(SV_FIFO, 0666) == -1) {
		perror("mkfifo");
	}

	// 서버 fifo 파일 읽기 전용으로 열기
	if ((sv_fifo_fd = open(SV_FIFO, O_RDONLY)) == -1) {
		perror("open1");
	}

	while (1) {
		// 최대 클라이언트 수 넘으면 종료
		if (num_clients >= MAX_CLIENTS) {
			printf("최대 클라이언트 수를 초과하였습니다.\n");
			break;
		}

		char temp[MAX_SIZE] = { 0 }; // i.e, 'access ./cl_fifo_2250' 또는 'send hello I'm sooyeon.'
		if ((read(sv_fifo_fd, temp, sizeof(temp))) > 0) { // 서버 fifo 파일에 읽을 값이 존재할 때

			char* command = strtok(temp, " "); // 문자열 앞에 'access' 또는 'send' 분리

			// 새 클라이언트가 접속했을 경우, 클라이언트 정보를 읽고 출력
			if (strcmp(command, "access") == 0) {
				char* message = strtok(NULL, ""); // access 뒤에 문자열만 추출

				if (message != NULL) {
					num_clients++; // 서버에 접속한 클라이언트 수 증가

					// 메시지로부터 클라이언트 pid 추출
					pid_t new_client_pid;
					sscanf(message, "./cl_fifo_%d", &new_client_pid);
					clients[num_clients].pid = new_client_pid;

					// 파일 경로 생성
					sprintf(cl_fifo_path[num_clients], CL_FIFO, new_client_pid);

					// 해당 파일 경로로 클라이언트 fifo 파일 쓰기 전용으로 열기
					// 서버 측에 해당 클라이언트에 대한 fifo 파일 디스크립터 저장
					if ((clients[num_clients].cl_fifo_fd = open(cl_fifo_path[num_clients], O_WRONLY)) == -1) {
						perror("open2");
					}
					else {
						// 클라이언트 정보 출력
						printf("새 클라이언트가 접속하였습니다. PID: %d, FD: %d\n", clients[num_clients].pid, clients[num_clients].cl_fifo_fd);

						if (close(clients[num_clients].cl_fifo_fd) == -1) {
							perror("close1");
						}
					}
				}
			}
			// 메시지를 서버로 보낸 클라이언트를 제외한 모든 클라이언트에게 메시지 전송
			else if (strcmp(command, "send") == 0) {
				char* cur_pid_str = strtok(NULL, " "); // 서버가 받은 클라이언트 메시지로부터 해당 클라이언트의 pid 추출
				pid_t cur_pid;
				sscanf(cur_pid_str, "%d", &cur_pid); // pid 타입 변환

				char* message = strtok(NULL, ""); // 모든 클라이언트에게 보낼 나머지 메시지 부분 추출

				if (message != NULL) {
					for (int i = 1; i <= num_clients; ++i) {
						if (clients[i].pid != cur_pid) { // 메시지를 보낸 클라이언트 제외
							
							// 클라이언트들의 fifo 파일 경로로 하나씩 읽기 전용으로 열기
							if ((clients[i].cl_fifo_fd = open(cl_fifo_path[i], O_WRONLY)) == -1) {
								perror("open3");
							}
							else {
								// 각 클라이언트 fifo 파일에 메시지 쓰기
								if (write(clients[i].cl_fifo_fd, message, strlen(message)) == -1) {
									perror("write");
								}

								if (close(clients[i].cl_fifo_fd) == -1) {
									perror("close2");
								}
							}
						}
					}
				}
			}
		}
	}

	close(sv_fifo_fd);
	unlink(SV_FIFO);

	return 0;
}