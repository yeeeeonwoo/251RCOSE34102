#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS_NUM 30
#define TIME_QUANTUM 3

#define TRUE 1
#define FALSE 0

// CPU �����ٸ� �˰��� ���
#define FCFS 0
#define SJF 1
#define PRIORITY 2
#define RR 3

// CPU ����ü ����
typedef struct Process* processPointer;
typedef struct Process {
	int pid;
	int arrivalTime;
	int cpuBurstTime;
	int cpuRemainingTime;
	int ioBurstTime;
	int ioRemainingTime;
	int ioRequestTime; // �ϴ��� io �ѹ��� �߻��ϰ� ����(���׸� 2�� ��������/ ���Ŀ� �����ϱ�)
	int priority;
	int waitingTime;
	int turnaroundTime;
	int responseTime; 
}Process;

// �������� �� ť ����
// ready ť
processPointer readyQueue[MAX_PROCESS_NUM]; 
int readySize = 0; // ���� readyť�� �ִ� ���μ��� ����

// waiting ť
processPointer waitingQueue[MAX_PROCESS_NUM]; 
int waitingSize = 0; // ���� waitingť�� �ִ� ���μ��� ����

// terminated ť
processPointer terminatedQueue[MAX_PROCESS_NUM]; 
int terminatedSize = 0; // ���� terminatedť�� �ִ� ���μ��� ����

// ���� ���� ���� ���μ���(running)
processPointer runningProcess = NULL; 

int currentTime = 0; // �� �ð�
int cpuIdleTime = 0; // cpu ���� �ð�
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid ���������� ��, �ߺ������� ���� ������ pid���� ����

// ť�� �ʱ�ȭ�ϴ� �Լ�
void initQueues() {
	readySize = 0;
	waitingSize = 0;
	terminatedSize = 0;
	runningProcess = NULL;
	currentTime = 0;
	cpuIdleTime = 0;

	for (int i = 0; i < MAX_PROCESS_NUM; i++) {
		readyQueue[i] = NULL;
		waitingQueue[i] = NULL;
		terminatedQueue[i] = NULL;
	}
}

// pid�� �������� �����ϴ� �Լ�
int generatePID() {
	while (1) {
		int pid = rand() % 1000 + 1; // 1~100����
		int duplicate = 0; // �ߺ�üũ
		for (int i = 0; i < MAX_PROCESS_NUM; i++) {
			if (usedPID[i] == pid) { //�̹� ���� pid�̸� break
				duplicate = 1;
				break;
			}
		}
		if (!duplicate) { // ������ ���� pid�̸� usedPID�� �����ϰ� break
			for (int i = 0; i < MAX_PROCESS_NUM; i++) {
				if (usedPID[i] == 0) {
					usedPID[i] = pid;
					break;
				}
			}
			return pid;
		}
	}
}



// ���μ��� �����ϴ� �Լ�
void createProcess() {
	processPointer newProcess = (processPointer)malloc(sizeof(Process));

	// random data �ο�(���׸� 1��)
	newProcess->pid = generatePID();
	newProcess->arrivalTime = rand() % 10 + 1; // ���� 1~10���� ����
	newProcess->cpuBurstTime = rand() % 20 + 1; // ���� 1~20���� ����
	newProcess->ioBurstTime = rand() % 8 + 3; // ������ ���� 3~10���� ���� 
	newProcess->priority = rand() % 5 + 1; // ���� 1~5�� ����
	newProcess->ioRequestTime = rand() % (newProcess->cpuBurstTime - 1) + 1; // ���� 1~cpu burst time - 1�� ����

	// ���� �ʱ�ȭ
	newProcess->cpuRemainingTime = newProcess->cpuBurstTime;
	newProcess->ioRemainingTime = newProcess->ioBurstTime;
	newProcess->waitingTime = 0;
	newProcess->turnaroundTime = 0;
	newProcess->responseTime = -1; // ���� cpu �Ҵ���� �ʾ����ϱ� -1�� �ʱ�ȭ

	return newProcess;
}

