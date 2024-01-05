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

#define MAX_SIZE 1024 // �迭 �ִ� ũ��
#define MAX_CLIENTS 10 // �ִ� Ŭ���̾�Ʈ ��
#define SV_FIFO "./sv_fifo" // ���� fifo ����
#define CL_FIFO "./cl_fifo_%d" // Ŭ���̾�Ʈ fifo ���� + %d�� Ŭ���̾�Ʈ pid ���� ��

int sv_fifo_fd, cl_fifo_fd; // ���� ��ũ����
int access_flag = false; // ���� Ȯ�� ����
char cl_fifo_path[512]; // Ŭ���̾�Ʈ fifo ���� ���
pid_t client_pid; // Ŭ���̾�Ʈ pid

void send()
{
	while (1)
	{
		if (!access_flag) { // Ŭ���̾�Ʈ�� �������� �� ������ Ŭ���̾�Ʈ ���� ����
			char message[MAX_SIZE];
			sprintf(message, "access %s", cl_fifo_path); // ���ϰ�ο� "access" ���ڿ� ����
			access_flag = true; // �ش� Ŭ���̾�Ʈ�� ���� ���� true

			// ���� fifo ���� ���� message �ۼ� �� �ݱ�
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
		else { // ������ �޽��� ����
			char message[MAX_SIZE];
			char temp[MAX_SIZE - 100];

			// ������ ���� �޽��� �Է�
			if (fgets(temp, sizeof(temp), stdin) == NULL) {
				continue;
			}

			temp[strcspn(temp, "\n")] = 0; // \n ��ġ�� �ι���(\0) ����
			snprintf(message, sizeof(message), "send %d %s", client_pid, temp); // ������ ���� �޽��� ����("send" + Ŭ���̾�Ʈ pid + �޽���)

			// ���� fifo ���� ���� message �ۼ� �� �ݱ�
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

		// �ش� Ŭ���̾�Ʈ fifo ���� �б� �������� ���� �б�
		if ((cl_fifo_fd = open(cl_fifo_path, O_RDONLY)) != -1) {
			ssize_t bytes_read;
			
			// �����κ��� ���� �޽��� ȭ�鿡 ���
			if ((bytes_read = read(cl_fifo_fd, message, sizeof(message))) > 0) {
				message[bytes_read] = '\0'; // ������ �ι��� ����(����)
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
	client_pid = getppid(); // Ŭ���̾�Ʈ pid ����
	sprintf(cl_fifo_path, CL_FIFO, client_pid); // Ŭ���̾�Ʈ ���� ��� ����

	// Ŭ���̾�Ʈ ���� ��η� fifo ���� ����
	if (mkfifo(cl_fifo_path, 0666) == -1) {
		perror("mkfifo");
	}

	printf("Client =====\n");

	if (fork() == 0) receive(); // �ڽ� ���μ����� �޽��� �޴� ����
	else send(); // �θ� ���μ����� �Է� �޽��� ���� ����

	unlink(cl_fifo_path);
	return 0;
}