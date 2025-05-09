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
// job ť
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // ���� jobť�� �ִ� ���μ��� ����

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

int currentTime = 0; // ���� �ð�
int cpuIdleTime = 0; // cpu ���� �ð�
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid ���������� ��, �ߺ������� ���� ������ pid���� ����


// ť�� �ʱ�ȭ�ϴ� �Լ�
void initQueues() {
	jobSize = 0;
	readySize = 0;
	waitingSize = 0;
	terminatedSize = 0;
	runningProcess = NULL;
	currentTime = 0;
	cpuIdleTime = 0;

	for (int i = 0; i < MAX_PROCESS_NUM; i++) {
		jobQueue[i] = NULL;
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
processPointer createProcess() {
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

// job ť
// job ť�� ���μ��� ����
void insertJobQueue(processPointer newProcess) {
	if (jobSize < MAX_PROCESS_NUM) {
		jobQueue[jobSize] = newProcess; //queue�� �ϳ��� ���μ��� �ֱ�
		jobSize++;
	}
	else {
		printf("Job Queue is already full\n");
	}
}
// job ť���� ���μ��� ����(���� �ð��� arrive�� ���μ���)
processPointer deleteJobQueue(int index) {
	if (jobSize < 0 || index >= jobSize) return NULL;
	processPointer temp = jobQueue[index]; // index�� ���μ��� ����

	for (int i = 0; i < jobSize - 1; i++) { // queue ��ĭ�� ������ ����
		jobQueue[i] = jobQueue[i + 1];
	}
	jobQueue[jobSize - 1] = NULL; // ť �������� NULL��
	jobSize--;

	return temp; // ���� ���μ��� ��ȯ
}

// job ť ���� ���
void printJobQueue() {
	if (jobSize == 0) printf("Job Queue is empty\n");
	for (int i = 0; i < jobSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n", 
			jobQueue[i]->pid, jobQueue[i]->arrivalTime, jobQueue[i]->cpuBurstTime, jobQueue[i]->ioBurstTime, jobQueue[i]->priority, jobQueue[i]->ioRequestTime);
	}
}

// job ť arrival time �������� �����ϴ� �Լ�
void sortJobQueue() {
	for (int i = 1; i < jobSize; i++) {
		processPointer key = jobQueue[i];
		int j = i - 1;

		while (j >= 0 && jobQueue[j]->arrivalTime > key->arrivalTime) {
			jobQueue[j + 1] = jobQueue[j];
			j--;
		}
		jobQueue[j + 1] = key;
	}
}




// ready ť
// ready ť�� ���μ��� ����
void insertReadyQueue(processPointer newProcess) {
	if (readySize < MAX_PROCESS_NUM) {
		readyQueue[readySize] = newProcess; //queue�� �ϳ��� ���μ��� �ֱ�
		readySize++;
	}
	else {
		printf("Ready Queue is already full\n");
	}
}

// ready ť���� ���μ��� ����
processPointer deleteReadyQueue(int index) {
	if (readySize < 0 || index >= readySize) return NULL;
	processPointer temp = readyQueue[index]; // index�� ���μ��� ����

	for (int i = 0; i < readySize - 1; i++) { // queue ��ĭ�� ������ ����
		readyQueue[i] = readyQueue[i + 1];
	}
	readyQueue[readySize - 1] = NULL; // ť �������� NULL��
	readySize--;

	return temp; // ���� ���μ��� ��ȯ
}
// ready ť ���� ���
void printReadyQueue() {
	if (readySize == 0) printf("Ready Queue is empty\n");
	for (int i = 0; i < readySize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n", 
			readyQueue[i]->pid, readyQueue[i]->arrivalTime, readyQueue[i]->cpuBurstTime, readyQueue[i]->ioBurstTime, readyQueue[i]->priority, readyQueue[i]->ioRequestTime);
	}
}





// waiting ť
// waiting ť�� ���μ��� ����
void insertWaitingQueue(processPointer newProcess) {
	if (waitingSize < MAX_PROCESS_NUM) {
		waitingQueue[waitingSize] = newProcess; //queue�� �ϳ��� ���μ��� �ֱ�
		waitingSize++;
	}
	else {
		printf("Waiting Queue is already full\n");
	}
}

// waiting ť���� ���μ��� ����
processPointer deleteWaitingQueue(int index) {
	if (waitingSize < 0 || index >= waitingSize) return NULL;
	processPointer temp = waitingQueue[index]; // index�� ���μ��� ����
	for (int i = 0; i < waitingSize - 1; i++) { // queue ��ĭ�� ������ ����
		waitingQueue[i] = waitingQueue[i + 1];
	}
	waitingQueue[waitingSize - 1] = NULL; // ť �������� NULL��
	waitingSize--;

	return temp; // ���� ���μ��� ��ȯ
}

// waiting ť ���� ���
void printWaitingQueue() {
	if (waitingSize == 0) printf("Waiting Queue is empty\n");
	for (int i = 0; i < waitingSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n",
			waitingQueue[i]->pid, waitingQueue[i]->arrivalTime, waitingQueue[i]->cpuBurstTime, waitingQueue[i]->ioBurstTime, waitingQueue[i]->priority, waitingQueue[i]->ioRequestTime);
	}
}



// terminatedť
// terminated ť�� ���μ��� ����
void insertTerminatedQueue(processPointer newProcess) {
	if (terminatedSize < MAX_PROCESS_NUM) {
		terminatedQueue[terminatedSize] = newProcess; //queue�� �ϳ��� ���μ��� �ֱ�
		terminatedSize++;
	}
	else {
		printf("Terminated Queue is already full\n");
	}
}

// terminated ť ���� ���
void printTerminatedQueue() {
	if (terminatedSize == 0) printf("Terminated Queue is empty\n");
	for (int i = 0; i < terminatedSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n",
			terminatedQueue[i]->pid, terminatedQueue[i]->arrivalTime, terminatedQueue[i]->cpuBurstTime, terminatedQueue[i]->ioBurstTime, terminatedQueue[i]->priority, terminatedQueue[i]->ioRequestTime);
	}
}




// ���μ��� ���� �� �����ϴ� �Լ�
void createProcesses(int n) {
	for (int i = 0; i < n; i++) {
		processPointer newProcess = createProcess(); //createProcess()�Լ� ���� �� ȣ���ؼ�!
		insertJobQueue(newProcess);
	}
	sortJobQueue();
}

// �ùķ��̼� �Լ�
void simulate(int algorithm) {
	for (currentTime = 0; currentTime < MAX_TIME_UNIT; currentTime++) {
		// job ť -> ready ť
		for (int i = 0; i < jobSize; i++) {
			if (jobQueue[i]->arrivalTime == currentTime) { // ����ð� = ���μ��� �����ð�, job ť -> ready ť
				processPointer newProcess = deleteJobQueue(i);
				insertReadyQueue(newProcess);
				i--; // job ť���� �ϳ� �����ϱ�
			}
			else if (jobQueue[i]->arrivalTime > currentTime) break; // ���� ���μ��� ���� ��
		}

		// waiting ť -> ready ť
		for (int i = 0; i < waitingSize; i++) {
			waitingQueue[i]->ioRemainingTime--;
			waitingQueue[i]->turnaroundTime++;

			if (waitingQueue[i]->ioRemainingTime == 0) {
				processPointer newProcess = deleteWaitingQueue(i); // waiting ť���� ����
				insertReadyQueue(newProcess); // ready ť�� �ֱ�
				i--;// ready ť���� �ϳ� �����ϱ�
			}
		}

		// ������ ���μ��� ����
		processPointer selected = NULL;
		// �˰��� ����
		switch (algorithm) {
			case FCFS:
				selected = FCFS();
				break;
		}

		// ������ ���μ��� ������ cpu idle
		if (selected == NULL) {
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		
		// ������ ���μ��� ������
		else {
			runningProcess = selected;

			// response time ����
			if (runningProcess->responseTime == -1) {
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime; // response time�� ����ð� - �����ð�
			}

			// ����
			runningProcess->cpuRemainingTime--;
			runningProcess->turnaroundTime++;
			printf("%d: PID %d is running (remaining: %d)\n",
				currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

			// IO request �߻�
			if (runningProcess->cpuRemainingTime > 0 &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {
				// ���� ������ �ȳ����� io ��û �ð��� ����������
				insertWaitingQueue(runningProcess); // waiting ť�� ��������
				runningProcess = NULL; // cpu�� idle�� ����
				continue;  
			}

			// cpu burst �Ϸ�
			if (runningProcess->cpuRemainingTime == 0) { // cpu ���� �ð� ������
				insertTerminatedQueue(runningProcess); // terminated ť�� �̵�
				runningProcess = NULL;
			}
		}
		// ��� ���� �ٸ� ���μ����� waitingTime ����
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}
	}
}

// FCFS �˰���(���׸� 3��)
// ready ť�� ���� ������� ����
// cpu burst ������ terminated ť��
// �����ϴٰ� io request ������ waiting ť��

processPointer FCFS() {
	if (readySize == 0) return NULL;
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready ť �� �� ���μ��� ��ȯ
	}
	return runningProcess; // ���� �������� ���μ��� �״�� ��ȯ
}