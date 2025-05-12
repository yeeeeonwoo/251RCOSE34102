#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TIME_UNIT 1000
#define MAX_PROCESS_NUM 30
#define TIME_QUANTUM 3

#define TRUE 1
#define FALSE 0


// CPU �����ٸ� �˰��� ���
#define FCFS 0
#define NP_SJF 1
#define P_SJF 2
#define NP_PRIORITY 3
#define P_PRIORITY 4
#define RR 5

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
	int priority; // ���� ���� ���� �켱������ ����
	int waitingTime;
	int turnaroundTime;
	int responseTime; 
	int rrUsedTime;
}Process;

// �Լ� ���� 
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();

// �Լ� �迭
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin"
};


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

int ganttChart[MAX_TIME_UNIT]; // ��Ʈ ��Ʈ display��


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
		usedPID[i] = 0;
		ganttChart[i] = -1;
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
	newProcess->rrUsedTime = 0;

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

// ��Ʈ ��Ʈ ��� �Լ�(�� �׸� 9��)
// pid�� �ٲ� ������ ����ؼ� ���μ��� ���۰� ���� ǥ����
void printGanttChart() {
	int previous = ganttChart[0]; // ���� ���μ����� pid�� ����
	int start = 0; // ���μ����� ������ ����

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < MAX_TIME_UNIT; i++) {
		if (ganttChart[i] != previous) { // ���μ����� �ٸ� ���μ����� �ٲ�� ���
			if (previous == -1) { 
				printf("|--idle--|"); // pid�� 0�̸� ���μ��� ���� cpu idle ����
			}
			else {
				printf("|--P%d--|", previous); // pid�� 0�� �ƴϸ� pid ���
			}
			printf(" %2d ", i);

			// ���ο� ���μ���(����) ����
			start = i; 
			previous = ganttChart[i];
		}
	}
	// ������ ���� ���
	if (previous == -1)
		printf("|--idle--|");
	else
		printf("|--P%d--|", previous);
	printf(" %2d |\n", MAX_TIME_UNIT);
	}
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
				selected = ALG_FCFS();
				break;
			case NP_SJF:
				selected = ALG_NP_SJF();
				break;
			case P_SJF:
				selected = ALG_P_SJF();
				break;
			case NP_PRIORITY:
				selected = ALG_NP_PRIORITY();
				break;
			case P_PRIORITY:
				selected = ALG_P_PRIORITY();
				break;
			case RR:
				selected = ALG_RR();
				break;
		}

		// ������ ���μ��� ������ cpu idle
		if (selected == NULL) {
			ganttChart[currentTime] = -1;
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		
		// ������ ���μ��� ������
		else {
			runningProcess = selected;
			ganttChart[currentTime] = runningProcess->pid;

			// response time ����
			if (runningProcess->responseTime == -1) {
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime; // response time�� ����ð� - �����ð�
			}

			// RR�� ��, time quantum ��� üũ
			runningProcess->rrTimeUsed++;

			// ����
			runningProcess->cpuRemainingTime--;
			runningProcess->turnaroundTime++;
			printf("%d: PID %d is running (remaining: %d)\n",
				currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

			// IO request �߻�(�� �׸� 2��)
			if (runningProcess->cpuRemainingTime > 0 &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {
				// RR�� ��
				runningProcess->rrTimeUsed = 0;
				// ���� ������ �ȳ����� io ��û �ð��� ����������
				insertWaitingQueue(runningProcess); // waiting ť�� ��������
				runningProcess = NULL; // cpu�� idle�� ����
				continue;  
			}

			// cpu burst �Ϸ�
			if (runningProcess->cpuRemainingTime == 0) { // cpu ���� �ð� ������
				// RR�� ��
				runningProcess->rrTimeUsed = 0;
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
processPointer ALG_FCFS() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready ť �� �� ���μ��� ��ȯ
	}
	return runningProcess; // ���� �������� ���μ��� �״�� ��ȯ
}

// Non-Preemptive SJF �˰���(���׸� 4��)
// ���� cpu time ª�� ���μ������� ����
// �ѹ� ���μ����� ���۵Ǹ� �������� �ʰ� �� �����
processPointer ALG_NP_SJF() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL
	if (runningProcess == NULL) {
		int min_index = 0;
		for (int i = 0; i < readySize; i++) {
			// ���� cpu time ���� ª�� ���μ��� ã��
			if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
				min_index = i;
			}
			// ���� cpu time�� ������ ���� �ð� ��������
			else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
				if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
					min_index = i;
				}
			}
		}
		return deleteReadyQueue(min_index);
	}
	else {
		return runningProcess;
	}
}
// Preemptive SJF �˰���(���׸� 5��)
// ���� cpu time ª�� ���μ������� ����
// �� �ð� cpu time�� üũ�ؼ� �� ���� ����ð��� ���� ���μ����� ������ ����
processPointer ALG_P_SJF() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL
	int min_index = 0;
	for (int i = 0; i < readySize; i++) {
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// ���� cpu time�� ������ ���� �ð� ��������
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// preemptive üũ
	if (runningProcess == NULL) { // ���� ���� ���� ���μ��� ������ �׳� ready ť���� �������� ��
		return deleteReadyQueue(min_index);
	}
	// ���� ���� ���μ������� ���� ���� ���μ����� ª���� ����
	if (runningProcess->cpuRemainingTime > readyQueue[min_index]->cpuRemainingTime) {
		insertReadyQueue(runningProcess);
		return deleteReadyQueue(min_index);
	}
	// �ƴϸ� �׳� ���� ���μ��� ��� ����
	return runningProcess;
	}
}

// Non-Preemptive PRIORITY �˰���(���׸� 6��)
// �ѹ� ���μ����� �����ϸ� �������� �ʰ� �� �����
// Priority�� ���� ������� ����
processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL{
	if (runningProcess == NULL) {
		int min_index = 0;
		for (int i = 0; i < readySize; i++) {
			//  priority ���� ���� ���μ��� ã��
			if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
				min_index = i;
			}
			// priority�� ������ ���� �ð� ��������
			else if (readyQueue[i]->priority == readyQueue[min_index]->priority) {
				if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
					min_index = i;
				}
			}
		}
		return deleteReadyQueue(min_index);
	}
	else {
		return runningProcess;
	}
}

// Non-Preemptive PRIORITY �˰���(���׸� 6��)
// Priority�� ���� ������� ����
// �� �ð� priority�� üũ�ؼ� �� ���� priority�� ���� ���μ����� ������ ����
processPointer ALG_P_PRIORITY() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL
	
	int min_index = 0;
	for (int i = 0; i < readySize; i++) {
		//  priority ���� ���� ���μ��� ã��
		if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
			min_index = i;
		}
		// priority�� ������ ���� �ð� ��������
		else if (readyQueue[i]->priority == readyQueue[min_index]->priority) {
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// preemptive üũ
	if (runningProcess == NULL) { 
		return deleteReadyQueue(min_index); // ���� ���� ���� ���μ��� ������ �׳� ready ť���� �������� ��
	}
	// ���� ���� ���μ������� ���� ���� ���μ����� priority�� ������ ����
	if (runningProcess->priority > readyQueue[min_index]->priority) {
		insertReadyQueue(runningProcess);
		return deleteReadyQueue(min_index);
	}
	// �ƴϸ� �׳� ���� ���μ��� ��� ����
	return runningProcess;
}


// Round-Robbin �˰���(���׸� 7��)
// �� ���μ����� time quantum��ŭ ����ǰ� �� ���� ���μ����� �Ѿ
// ���μ����� ������ ������ ready ť �� �ڷ�
processPointer ALG_RR() {
	if (readySize == 0) return NULL; // ready ť ��������� NULL

	// �������� ���μ����� ���� ���
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0);
		nextProcessor->rrTimeUsed = 0; // ���� ������ ���μ��� ��� �ð� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� ���� ���μ����� Ÿ�� ���Ҹ�ŭ ����� ���
	if (runningProcess->rrTimeUsed >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // �ٽ� ready ť��
		processPointer nextProcessor = deleteReadyQueue(0); // ���� ���μ��� ����
		nextProcessor->rrTimeUsed = 0; // ���� �����ϹǷ� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� Ÿ������ ������ ���̸�
	return runningProcess;
}


// ���� ���μ����� ���ؾ� �ϴϱ� ���� �Լ� �ʿ�(job ť����)
void cloneProcessesfromJQ() {
	readySize = 0;
	for (int i = 0; i < jobSize; i++) {
		processPointer original = jobQueue[i];
		processPointer cloned = (processPointer)malloc(sizeof(Process));

		// ��� �ʵ� ����
		cloned->pid = original->pid;
		cloned->arrivalTime = original->arrivalTime;
		cloned->cpuBurstTime = original->cpuBurstTime;
		cloned->cpuRemainingTime = original->cpuBurstTime; // �׻� ���� ����
		cloned->ioBurstTime = original->ioBurstTime;
		cloned->ioRemainingTime = original->ioBurstTime;
		cloned->ioRequestTime = original->ioRequestTime;
		cloned->priority = original->priority;

		// �ʱ�ȭ
		cloned->waitingTime = 0;
		cloned->turnaroundTime = 0;
		cloned->responseTime = -1;
		cloned->rrTimeUsed = 0;

		readyQueue[readySize++] = cloned; // ready ť �ٽ� ä���
	}
}

// ��(waiting time, turnaround time, response time) ���׸� 10��
void evaluate() {
	int totalWaitingTime = 0;
	int totalTurnaroundTime = 0;
	int totalResponseTime = 0;

	// terminated ť�� �ִ� ��� ���μ��� ��ȸ
	for (int i = 0; i < terminatedSize; i++) {
		totalWaitingTime += terminatedQueue[i]->waitingTime;
		totalTurnaroundTime += terminatedQueue[i]->turnaroundTime;
		totalResponseTime += terminatedQueue[i]->responseTime;
	}
	
	// ��� ���(average waiting time, average turnaround time, average response time)
	float avgWaitingTime = totalWaitingTime / (float)terminatedSize;
	float avgTurnaroundTime = totalTurnaroundTime / (float)terminatedSize;
	float avgResponseTime = totalResponseTime / (float)terminatedSize;

	// ��� ���
	printf("\n-------------Evaluation Result-------------\n");
	printf("avgWaitingTime: %.2f\n", avgWaitingTime);
	printf("avgTurnaroundTime: %.2f\n", avgTurnaroundTime);
	printf("avgResponseTime: %.2f\n", avgResponseTime);
}




// �����Լ�
int main() {
	srand(time(NULL)); 
	int numProcesses = 5; // ���μ��� ���� 5���� ����

	// ���� ���μ��� ���� ����(������ �־�� ���� ���μ������ �� ����)
	processPointer baseProcesses[MAX_PROCESS_NUM];
	int baseSize = 0;
	for (int i = 0; i < numProcesses; i++) {
		baseProcesses[baseSize++] = createProcess();
	}

	// ��� �˰��� ���� �� ��
	for (int alg = FCFS; alg <= RR; alg++) {
		printf("\n============================================\n");
		printf("%s\n", algorithmNames[alg]);

		// �ʱ�ȭ
		initQueues();

		// ���� ���μ��� ���� �� jobQueue�� ����
		for (int i = 0; i < baseSize; i++) {
			processPointer original = baseProcesses[i];
			processPointer clone = (processPointer)malloc(sizeof(Process));

			// ��� ���� ����
			clone->pid = original->pid;
			clone->arrivalTime = original->arrivalTime;
			clone->cpuBurstTime = original->cpuBurstTime;
			clone->cpuRemainingTime = original->cpuBurstTime;
			clone->ioBurstTime = original->ioBurstTime;
			clone->ioRemainingTime = original->ioBurstTime;
			clone->ioRequestTime = original->ioRequestTime;
			clone->priority = original->priority;

			clone->waitingTime = 0;
			clone->turnaroundTime = 0;
			clone->responseTime = -1;
			clone->rrTimeUsed = 0;

			insertJobQueue(clone); // job ť�� ����
		}

		// �ùķ��̼� ����
		simulate(alg);

		// ��Ʈ��Ʈ ���
		printGanttChart();

		// ��
		evaluate();

		// terminated ť �޸� ����
		for (int i = 0; i < terminatedSize; i++) {
			free(terminatedQueue[i]);
		}
	}

	// base ���μ��� �޸� ����
	for (int i = 0; i < baseSize; i++) {
		free(baseProcesses[i]);
	}
}

