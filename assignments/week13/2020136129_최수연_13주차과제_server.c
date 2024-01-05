#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>

#define MAX_SIZE 1024 // �迭 �ִ� ũ��
#define MAX_CLIENTS 10 // �ִ� Ŭ���̾�Ʈ ��
#define SV_FIFO "./sv_fifo" // ���� fifo ����
#define CL_FIFO "./cl_fifo_%d" // Ŭ���̾�Ʈ fifo ���� + %d�� Ŭ���̾�Ʈ pid ���� ��

struct ClientInfo {
	pid_t pid; // Ŭ���̾�Ʈ pid (Ŭ���̾�Ʈ ���� �̸� ����)
	int cl_fifo_fd; // Ŭ���̾�Ʈ fifo ���� ��ũ����
};

int main(int argc, char** argv)
{
	struct ClientInfo clients[MAX_CLIENTS] = { 0 }; // Ŭ���̾�Ʈ ���� �ʱ�ȭ
	int sv_fifo_fd; // ���� fifo ���� ��ũ����
	int num_clients = 0; // ������ ������ Ŭ���̾�Ʈ ��
	char cl_fifo_path[MAX_CLIENTS][MAX_SIZE] = { 0 }; // �� Ŭ���̾�Ʈ�� fifo ���� ��� �ʱ�ȭ

	printf("Server =====\n");

	// ���� fifo ���� ����
	if (mkfifo(SV_FIFO, 0666) == -1) {
		perror("mkfifo");
	}

	// ���� fifo ���� �б� �������� ����
	if ((sv_fifo_fd = open(SV_FIFO, O_RDONLY)) == -1) {
		perror("open1");
	}

	while (1) {
		// �ִ� Ŭ���̾�Ʈ �� ������ ����
		if (num_clients >= MAX_CLIENTS) {
			printf("�ִ� Ŭ���̾�Ʈ ���� �ʰ��Ͽ����ϴ�.\n");
			break;
		}

		char temp[MAX_SIZE] = { 0 }; // i.e, 'access ./cl_fifo_2250' �Ǵ� 'send hello I'm sooyeon.'
		if ((read(sv_fifo_fd, temp, sizeof(temp))) > 0) { // ���� fifo ���Ͽ� ���� ���� ������ ��

			char* command = strtok(temp, " "); // ���ڿ� �տ� 'access' �Ǵ� 'send' �и�

			// �� Ŭ���̾�Ʈ�� �������� ���, Ŭ���̾�Ʈ ������ �а� ���
			if (strcmp(command, "access") == 0) {
				char* message = strtok(NULL, ""); // access �ڿ� ���ڿ��� ����

				if (message != NULL) {
					num_clients++; // ������ ������ Ŭ���̾�Ʈ �� ����

					// �޽����κ��� Ŭ���̾�Ʈ pid ����
					pid_t new_client_pid;
					sscanf(message, "./cl_fifo_%d", &new_client_pid);
					clients[num_clients].pid = new_client_pid;

					// ���� ��� ����
					sprintf(cl_fifo_path[num_clients], CL_FIFO, new_client_pid);

					// �ش� ���� ��η� Ŭ���̾�Ʈ fifo ���� ���� �������� ����
					// ���� ���� �ش� Ŭ���̾�Ʈ�� ���� fifo ���� ��ũ���� ����
					if ((clients[num_clients].cl_fifo_fd = open(cl_fifo_path[num_clients], O_WRONLY)) == -1) {
						perror("open2");
					}
					else {
						// Ŭ���̾�Ʈ ���� ���
						printf("�� Ŭ���̾�Ʈ�� �����Ͽ����ϴ�. PID: %d, FD: %d\n", clients[num_clients].pid, clients[num_clients].cl_fifo_fd);

						if (close(clients[num_clients].cl_fifo_fd) == -1) {
							perror("close1");
						}
					}
				}
			}
			// �޽����� ������ ���� Ŭ���̾�Ʈ�� ������ ��� Ŭ���̾�Ʈ���� �޽��� ����
			else if (strcmp(command, "send") == 0) {
				char* cur_pid_str = strtok(NULL, " "); // ������ ���� Ŭ���̾�Ʈ �޽����κ��� �ش� Ŭ���̾�Ʈ�� pid ����
				pid_t cur_pid;
				sscanf(cur_pid_str, "%d", &cur_pid); // pid Ÿ�� ��ȯ

				char* message = strtok(NULL, ""); // ��� Ŭ���̾�Ʈ���� ���� ������ �޽��� �κ� ����

				if (message != NULL) {
					for (int i = 1; i <= num_clients; ++i) {
						if (clients[i].pid != cur_pid) { // �޽����� ���� Ŭ���̾�Ʈ ����
							
							// Ŭ���̾�Ʈ���� fifo ���� ��η� �ϳ��� �б� �������� ����
							if ((clients[i].cl_fifo_fd = open(cl_fifo_path[i], O_WRONLY)) == -1) {
								perror("open3");
							}
							else {
								// �� Ŭ���̾�Ʈ fifo ���Ͽ� �޽��� ����
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