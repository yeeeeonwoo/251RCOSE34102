#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TIME_UNIT 1000 // ��Ʈ��Ʈ �ִ����
#define MAX_PROCESS_NUM 30 // �ִ� ���μ��� ��
#define TIME_QUANTUM 3 // RR���� ����� Ÿ������

#define TRUE 1
#define FALSE 0

// �����ٸ� �˰��� �������(simulate()���� switch-case���� ����)
#define FCFS 0
#define NP_SJF 1
#define P_SJF 2
#define NP_PRIORITY 3
#define P_PRIORITY 4
#define RR 5
#define IASJF 6


// �����ٸ� �˰��� �̸� �迭(��¿�)
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin",
	"IO-Aware SJF",
};


// CPU ����ü ����
typedef struct Process {
	// ���μ��� �⺻����
	int pid;
	int arrivalTime;
	int priority; // ���� ���� ���� �켱������ ����

	// cpu burst ���� ����
	int cpuBurstTime;
	int cpuRemainingTime;
	int rrUsedTime; // RR �˰��򿡼� ���μ����� ���� ����� �ð�(Ÿ������ �������� Ȯ�ο�)

	// io burst ���� ����
	int ioRequestTime; // IO ��û ����(cpu�� ���� ���� ��������)
	int ioBurstTime;
	int ioRemainingTime;
	int isInIO; // ���� IO ��������� �ƴ���
	int hasRequestedIO; // ���� IO ��û���� �ִ���(IO�� �ѹ��� �������� �߻��Ѵ� ����)

	// evaluation ����
	int waitingTime; // �� ���ð�(���μ����� ready ť�� �ִ� �ð� ��)
	int turnaroundTime; // ����ð� - �����ð�

}Process;

// ����ü ������ ����
typedef struct Process* processPointer;

// �Լ� ���� ����
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();
processPointer ALG_IASJF();


// ť �� �������� ����

// job ť(���� �������� ���� ���μ�����)
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // ���� jobť�� �ִ� ���μ��� ����

// ready ť(cpu ��� ������� ���μ�����)
processPointer readyQueue[MAX_PROCESS_NUM];
int readySize = 0; // ���� readyť�� �ִ� ���μ��� ����

// waiting ť(io �߻��ؼ� ������� ���μ�����)
processPointer waitingQueue[MAX_PROCESS_NUM];
int waitingSize = 0;

// terminated ť(���� �Ϸ�� ���μ�����)
processPointer terminatedQueue[MAX_PROCESS_NUM];
int terminatedSize = 0; // ���� terminatedť�� �ִ� ���μ��� ����

// ���� ���� ���� ���μ���
processPointer runningProcess = NULL;

int currentTime = 0; // ���� �ð�
int cpuIdleTime = 0; // cpu idle �ð� ��
int simulationEndTime = 0; // ������ ���� ����
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid ���������� �� ������ pid���� ����(�ߺ�����)
int ganttChart[MAX_TIME_UNIT]; // ��Ʈ ��Ʈ�� ���� ��Ͽ�
int ioRequestedAt[MAX_TIME_UNIT]; // �ش� �ð��� � ���μ����� IO request�ߴ��� ���

// ť�� ���������� �ʱ�ȭ�ϴ� �Լ�
void initQueues() {
	jobSize = 0;
	readySize = 0;
	waitingSize = 0;
	terminatedSize = 0;
	runningProcess = NULL;
	currentTime = 0;
	cpuIdleTime = 0;
	simulationEndTime = 0;

	for (int i = 0; i < MAX_PROCESS_NUM; i++) {
		jobQueue[i] = NULL;
		readyQueue[i] = NULL;
		waitingQueue[i] = NULL;
		terminatedQueue[i] = NULL;
		usedPID[i] = 0;
	}

	for (int i = 0; i < MAX_TIME_UNIT; i++) {
		ganttChart[i] = -1;
		ioRequestedAt[i] = -1;  
	}
}


// pid�� �������� �����ϴ� �Լ�
int generatePID() {
	while (1) {
		int pid = rand() % 10 + 1; // 1~10 ����
		int duplicate = 0; // �ߺ�üũ(0�̸� �ߺ��ƴ�, 1�̸� �ߺ�)
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
	newProcess->cpuBurstTime = rand() % 20 + 2; // ���� 2~20���� ����
	newProcess->priority = rand() % 5 + 1; // ���� 1~5�� ����

	// ���� �ʱ�ȭ
	newProcess->cpuRemainingTime = newProcess->cpuBurstTime;
	newProcess->waitingTime = 0;
	newProcess->turnaroundTime = 0;
	newProcess->rrUsedTime = 0;

	newProcess->ioRequestTime = rand() % (newProcess->cpuBurstTime - 1) + 1; // IO ��û ������ 1 �̻�, cpuBurstTime-1 ���� (cpu �۾� ���̾� �߻�)
	newProcess->ioBurstTime = rand() % 5 + 2; // ���� 2~6���� ����
	newProcess->ioRemainingTime = 0;
	newProcess->isInIO = FALSE;
	newProcess->hasRequestedIO = FALSE;

	return newProcess; // newProcess�� ��ȯ�ؼ� job ť�� ���� 
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


// job ť���� ���μ��� ����(���� �ð� ���� ������ ���μ���(index)) -> ���� ���μ����� ready ť�� �̵�
processPointer deleteJobQueue(int index) {
	if (jobSize < 0 || index >= jobSize) return NULL;
	processPointer temp = jobQueue[index]; // index�� ���μ��� ����

	for (int i = index; i < jobSize - 1; i++) { // queue ��ĭ�� ������ ����
		jobQueue[i] = jobQueue[i + 1];
	}
	jobQueue[jobSize - 1] = NULL; // ť �������� NULL��
	jobSize--;

	return temp; // ���� ���μ��� ��ȯ
}

// job ť�� �ִ� ���μ����� ���(������)
void printJobQueue() {
	printf("\n--- Job Queue ---\n");
	if (jobSize == 0) printf("Job Queue is empty\n");
	printf("pid  arrival_time  CPU_burst  priority  IO_request_time  IO_burst_time\n");
	printf("-------------------------------------------------------------------------\n");

	for (int i = 0; i < jobSize; i++) {
		processPointer p = jobQueue[i];
		printf("%3d  %13d  %10d  %8d  %15d  %14d\n",
			p->pid,
			p->arrivalTime,
			p->cpuBurstTime,
			p->priority,
			p->ioRequestTime,
			p->ioBurstTime
		);
	}
	printf("-------------------------------------------------------------------------\n");
}

// job ť arrival time �������� �����ϴ� �Լ�(���� �ð� ������� ready ť�� �Űܾ� �ϴϱ�)
void sortJobQueue() { // insertion sort ���
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

// ready ť�� ���μ��� ����(from jobť or waiting ť)
void insertReadyQueue(processPointer newProcess) {
	if (readySize < MAX_PROCESS_NUM) {
		readyQueue[readySize] = newProcess; //queue�� �ϳ��� ���μ��� �ֱ�
		readySize++;
	}
	else {
		printf("Ready Queue is already full\n");
	}
}


// ready ť���� ���μ��� ����(to runningProcess)
processPointer deleteReadyQueue(int index) {
	if (readySize < 0 || index >= readySize) return NULL;

	processPointer temp = readyQueue[index]; // index�� ���μ��� ����

	for (int i = index; i < readySize - 1; i++) { // queue ��ĭ�� ������ ����
		readyQueue[i] = readyQueue[i + 1];
	}
	readyQueue[readySize - 1] = NULL; // ť �������� NULL��
	readySize--;

	return temp; // ���� ���μ��� ��ȯ
}


// ready ť ���� ���(������)
void printReadyQueue() {
	printf("\n--- Ready Queue ---\n");
	if (readySize == 0) printf("Ready Queue is empty\n");
	for (int i = 0; i < readySize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d Priority: %d\n",
			readyQueue[i]->pid, readyQueue[i]->arrivalTime,
			readyQueue[i]->cpuBurstTime, readyQueue[i]->priority); 
	}
}



// waiting ť

// ready ť�� ���μ��� ����(runningProcess���� io request ���� ��)
void insertWaitingQueue(processPointer newProcess) {
	if (waitingSize < MAX_PROCESS_NUM) {
		waitingQueue[waitingSize] = newProcess;
		waitingSize++;
	}
}

// ready ť���� ���μ��� ����(to runningprocess)
processPointer deleteWaitingQueue(int index) {
	if (index >= waitingSize) return NULL;

	processPointer temp = waitingQueue[index];

	for (int i = index; i < waitingSize - 1; i++) {
		waitingQueue[i] = waitingQueue[i + 1];
	}
	waitingQueue[waitingSize - 1] = NULL; // ť �������� NULL��
	waitingSize--;

	return temp;
}

// waiting ť ���� ���(������)
void printWaitingQueue() {
	printf("\n--- Waiting Queue ---\n");
	for (int i = 0; i < waitingSize; i++) {
		printf("PID %d, remaining I/O: %d\n",
			waitingQueue[i]->pid, waitingQueue[i]->ioRemainingTime);
	}
}


// terminatedť

// terminated ť�� ���μ��� ����
void insertTerminatedQueue(processPointer newProcess) {
	if (newProcess == NULL) return;

	// �ߺ� ���� ����
	for (int i = 0; i < terminatedSize; i++) {
		if (terminatedQueue[i]->pid == newProcess->pid)
			return;
	}

	if (terminatedSize < MAX_PROCESS_NUM) {
		terminatedQueue[terminatedSize] = newProcess; 
		terminatedSize++;
	}
	else {
		printf("Terminated Queue is full!\n");
	}
}


//  terminated ť ���� ���(������)
void printTerminatedQueue() {
	if (terminatedSize == 0) {
		printf("Terminated Queue is empty\n");
		return;
	}
	printf("\n-------------Terminated Processes-------------\n");
	printf("PID  Arrival  Burst  Priority  Waiting  Turnaround\n");
	for (int i = 0; i < terminatedSize; i++) {
		processPointer newProcess = terminatedQueue[i];
		printf("%3d  %7d  %5d  %8d  %7d  %10d\n",
			newProcess->pid, newProcess->arrivalTime, newProcess->cpuBurstTime, newProcess->priority,
			newProcess->waitingTime, newProcess->turnaroundTime);
	}
}


// ��Ʈ ��Ʈ ����Լ�
void printGanttChart(int endTime) {
	int previous = ganttChart[0]; // ���� �ð��� ���� ����(-1: idle, -2: io request, pid: �ش� ���μ����� cpu �����)
	int start = 0;

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < endTime; i++) {
		if (ganttChart[i] != previous) { // ���°� �ٲ���� ���� ���Ӱ� ���
			// ���� ���
			if (previous == -1) { // idle�̶��
				printf("|--idle--|");
			}
			else if (previous == -2) {
				if (ioRequestedAt[start] != -1) // IO request �߻��ߴٸ�
					printf("|--P%d(IO)--|", ioRequestedAt[start]);
				else
					printf("|--IO--|");  // ���� ����
			}
			else { // ���� ���μ����� �ٸ� ���μ����� ����Ǿ��ٸ�
				printf("|--P%d--|", previous);
			}
			printf(" %2d ", i);
			start = i;
			previous = ganttChart[i];
		}
	}

	// ������ ���� ���
	if (previous == -1) {
		printf("|--idle--|");
	}
	else if (previous == -2) {
		if (ioRequestedAt[start] != -1)
			printf("|--P%d(IO)--|", ioRequestedAt[start]);
		else
			printf("|--IO--|");  // ���� ����
	}
	else {
		printf("|--P%d--|", previous);
	}
	printf(" %2d |\n", endTime);
}


void simulate(int algorithm) {
	for (currentTime = 0; currentTime < MAX_TIME_UNIT; currentTime++) {

		// I/O �Ϸ�� ���μ��� �ִ��� Ȯ��(waiting ť -> ready ť)
		for (int i = 0; i < waitingSize; i++) {
			waitingQueue[i]->ioRemainingTime--;

			if (waitingQueue[i]->ioRemainingTime <= 0) { // IO ���� �ð��� ���ٸ�(IO �Ϸ���)
				waitingQueue[i]->isInIO = FALSE;

				printf("%d: PID %d I/O complete (back to ready queue)\n",
					currentTime, waitingQueue[i]->pid);

				insertReadyQueue(deleteWaitingQueue(i)); // waiting ť -> ready ť
				i--; //deleteWaitingQueue���� ��ĭ�� �о������ ����
			}
		}

		// ����� running ���μ��� ó��(running -> terminated ť)
		if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // CPU ���� �ð��� ���ٸ�(CPU �Ϸ���)
			insertTerminatedQueue(runningProcess);
			runningProcess = NULL;
		}

		// �� �ð����� �� ���μ����� �����ð� Ȯ���ϱ�(job ť -> ready ť)
		for (int i = 0; i < jobSize; i++) {
			if (jobQueue[i]->arrivalTime == currentTime) {
				insertReadyQueue(deleteJobQueue(i)); // job ť -> ready ť
				i--; //deleteJobQueue���� ��ĭ�� �о������ ����
			} 
		}

		// ��ü �ùķ��̼� ���� ���� Ȯ��
		if (jobSize == 0 && readySize == 0 && waitingSize == 0 && runningProcess == NULL) { // ��� ť�� ��� ���� ���� ���μ����� ������
			simulationEndTime = currentTime; // ����ð� ���
			break;
		}


		// �����ٸ� �˰��� ȣ��
		processPointer selected = NULL;
		switch (algorithm) {
			case FCFS: selected = ALG_FCFS(); break;
			case NP_SJF: selected = ALG_NP_SJF(); break;
			case P_SJF: selected = ALG_P_SJF(); break;
			case NP_PRIORITY: selected = ALG_NP_PRIORITY(); break;
			case P_PRIORITY: selected = ALG_P_PRIORITY(); break;
			case RR: selected = ALG_RR(); break;
			case IASJF: selected = ALG_IASJF(); break;
		}

		// ���μ��� ����
		if (selected == NULL) { // �ƹ��͵� ���õ��� ���� -> idle
			runningProcess = NULL;
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		else { // ���õǾ��ٸ� ����
			runningProcess = selected;

			// I/O ��û ����: ������ IO ��û�� ������ ioRequestTime �������� ��(runningProcess -> waitingť)
			if (!runningProcess->hasRequestedIO &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {

				ioRequestedAt[currentTime] = runningProcess->pid; // IO ��û ���� ���
				runningProcess->isInIO = TRUE; // IO ó���� 
				runningProcess->hasRequestedIO = TRUE; // IO ��û ���� ���
				runningProcess->ioRemainingTime = runningProcess->ioBurstTime; // ioRemainingTime �ʱ�ȭ

				printf("%d: PID %d is requesting I/O for %d units\n",
					currentTime, runningProcess->pid, runningProcess->ioBurstTime);

				insertWaitingQueue(runningProcess); 
				ganttChart[currentTime] = -2; // io request�϶��� ��Ʈ��Ʈ ���� -2�� ����
				runningProcess = NULL; // io request ���ȿ��� �ٸ� ���μ��� ó���Ұ�
				cpuIdleTime++; // cpu�� idle
				continue;
			}

			// IO ��û�� �ƴ϶��(�׳� cpu���� ���μ��� ����)
			else {
				// CPU���� ���� ����
				runningProcess->cpuRemainingTime--;
				runningProcess->turnaroundTime++;
				runningProcess->rrUsedTime++;

				printf("%d: PID %d is running (remaining: %d)\n",
					currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

				// cpu burst ���� �ð� ������ (running -> terminated ť)
				if (runningProcess->cpuRemainingTime == 0) {
					insertTerminatedQueue(runningProcess); // running -> terminated ť
					runningProcess = NULL;
				}
			}
		}


		// ready ť�� �ִ� ���μ����� ���/��ü �ð� ����
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}

		// ��Ʈ��Ʈ ���
		if (runningProcess != NULL) // �������� ���μ����� ������ pid
			ganttChart[currentTime] = runningProcess->pid;
		else if (ioRequestedAt[currentTime] != -1)
			ganttChart[currentTime] = -2;  // IO request��� -2
		else
			ganttChart[currentTime] = -1;  // idle�̸� -1

		simulationEndTime = currentTime; // �������ᰡ �ȵǰ� MAX_TIME_UNIT ���� ������ �����ϱ� ���� ����
	}

}


// FCFS �˰���(���׸� 3��)
// ready ť�� ���� ������� ����
// non-preemptive ���

processPointer ALG_FCFS() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		return NULL; // �̹� ���� �����̹Ƿ� simulate()���� idle�� ó��
	}
	if (readySize == 0) {
		return runningProcess; // ready ť�� ��������� �׳� �������̴� ���μ��� ��ȯ�ؼ� simulate���� ����
	}
	if (runningProcess == NULL) { // ���� ���� ���μ����� ������
		return deleteReadyQueue(0); // ready ť �� �� ���μ��� ��ȯ�ؼ� simulate���� ����
	}
	return runningProcess; // ���� �������� ���μ��� �״�� ��ȯ
}


// Non-Preemptive SJF �˰���(���׸� 4��)
// ���� cpu time ª�� ���μ������� ����
// �ѹ� ���μ����� ���۵Ǹ� �������� �ʰ� �� �����

processPointer ALG_NP_SJF() {
	if (runningProcess != NULL) { // ���� �������� ���μ����� �ִµ�
		if (runningProcess->cpuRemainingTime > 0) // ���� cpu �ð� �������� �״�� �������� ���μ��� ��ȯ�ؼ� simulate���� ����
			return runningProcess;
		else // cpu �ð� �ȳ�������
			return NULL; // �̹� ���� �����̹Ƿ� simulate()���� idle�� ó��
	}

	if (readySize == 0) { // �������� ���μ����� ���� readyť�� �������
		return NULL; // idle ó��
	}

	int min_index = 0; // cpuRemainingTime�� ���� ���� ���μ����� index ����

	for (int i = 1; i < readySize; i++) {
		// ���� cpuRemainingTime �������� ��
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i; 
		}
		// ������ ���� �ð� �������� tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// cpuRemainingTime�� ���ٸ� ���� �ð��� ���� ���μ����� ����
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// ������ ���μ����� ready ť���� �����ϰ� ��ȯ(simulate()���� readyť -> runningProcess)
	processPointer selected = readyQueue[min_index];
	deleteReadyQueue(min_index);
	return selected;
}


// Preemptive SJF �˰���(���׸� 5��)
// ���� cpu time ª�� ���μ������� ����
// �� �ð� cpu time�� üũ�ؼ� �� ���� ����ð��� ���� ���μ����� ������ ����

processPointer ALG_P_SJF() {
	if (readySize == 0) // ready ť ���������
		return (runningProcess != NULL) ? runningProcess : NULL; // ���� �������� ���μ��� ��ȯ

	int min_index = 0;  // cpuRemainingTime�� ���� ���� ���μ����� index ����

	for (int i = 0; i < readySize; i++) {
		// ���� cpuRemainingTime �������� ��
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// ������ ���� �ð� �������� tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// cpuRemainingTime�� ���ٸ� ���� �ð��� ���� ���μ����� ����
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
	// preemptive üũ
	if (runningProcess == NULL) { // ���� ���� ���� ���μ��� ������ �׳� ready ť���� �������� ��
		return deleteReadyQueue(min_index);
	}
	// ���� ���� ���μ������� ���� ���� ���μ����� ª���� ����
	if (runningProcess->cpuRemainingTime > readyQueue[min_index]->cpuRemainingTime) {
		insertReadyQueue(runningProcess); // ���� ���� ���� ���μ����� ready ť�� ������ (runningProcess -> ready ť)
		return deleteReadyQueue(min_index); // ������ ���μ����� ready ť���� ���� (simulate()���� readyť -> runningProcess)
	}
	// �ƴϸ� �׳� ���� ���μ��� ��� ����
	return runningProcess;
}


// Non-Preemptive PRIORITY �˰���(���׸� 6��)
// �ѹ� ���μ����� �����ϸ� �������� �ʰ� �� �����
// Priority�� ���� ������� ����

processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) // ready ť ���������
		return (runningProcess != NULL) ? runningProcess : NULL;  // ���� �������� ���μ��� ��ȯ
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // ���� ���� ���μ����� ����� ���
		runningProcess = NULL;
	}
	if (runningProcess == NULL) { // ���� ���� ���μ����� ���ٸ�
		int min_index = 0; // priority�� ���� ���� ���μ��� ã��(priority ���� �۾ƾ� priority�� ����)
		for (int i = 0; i < readySize; i++) {
			//  priority ���� ���� ���μ��� ã��
			if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
				min_index = i;
			}
			// ������ ���� �ð� �������� tie-break
			else if (readyQueue[i]->priority == readyQueue[min_index]->priority) {
				if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
					min_index = i;
				}
			}
		}
		return deleteReadyQueue(min_index); // ready ť���� ���� (simulate()���� readyť -> runningProcess)
	}
	// �������� ���μ����� ������ �״�� ����
	return runningProcess;
}


// Preemptive PRIORITY �˰���(���׸� 6��)
// Priority�� ���� ������� ����
// �� �ð� priority�� üũ�ؼ� �� ���� priority�� ���� ���μ����� ������ ����

processPointer ALG_P_PRIORITY() {
	if (readySize == 0) // ready ť ���������
		return (runningProcess != NULL) ? runningProcess : NULL;  // ���� �������� ���μ��� ��ȯ
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // ���� ���� ���μ����� ����� ���
		runningProcess = NULL;
	}
	int min_index = 0; // priority�� ���� ���� ���μ��� ã��(priority ���� �۾ƾ� priority�� ����)
	for (int i = 0; i < readySize; i++) {
		//  priority ���� ���� ���μ��� ã��
		if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
			min_index = i;
		}
		// ������ ���� �ð� �������� tie-break
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
		insertReadyQueue(runningProcess); // ���� ���� ���� ���μ����� ready ť�� ������ (runningProcess -> ready ť)
		return deleteReadyQueue(min_index); // ������ ���μ����� ready ť���� ���� (simulate()���� readyť -> runningProcess)
	}
	// �ƴϸ� �׳� ���� ���μ��� ��� ����
	return runningProcess;
}


// Round-Robbin �˰���(���׸� 7��)
// �� ���μ����� time quantum��ŭ ����ǰ� �� ���� ���μ����� �Ѿ
// ���μ����� ������ ������ ready ť �� �ڷ�

processPointer ALG_RR() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // ���� ���� ���μ����� ��������
		runningProcess = NULL; // NULL ��ȯ
	}

	if (readySize == 0) { // ready ť ���������
		return (runningProcess != NULL) ? runningProcess : NULL; // ���� �������� ���μ��� ��ȯ
	}
	// �������� ���μ����� ���� ���
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0); // ready ť �Ǿտ��� ��������
		nextProcessor->rrUsedTime = 0; // ���� ������ ���μ��� ��� �ð� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� ���� ���μ����� Ÿ�� ���Ҹ�ŭ ����� ���
	if (runningProcess->rrUsedTime >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // �ٽ� ready ť��(running -> ready ť)
		processPointer nextProcessor = deleteReadyQueue(0); // ���� ���μ���(ready ť �Ǿ�) ����
		nextProcessor->rrUsedTime = 0; // ���� �����ϹǷ� ���ð� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� Ÿ������ ������ ���̸�
	return runningProcess;
}

// IO-Aware SJF �˰���(�߰��Լ�, ���׸� 11��)
// non-preemptive ���
// SJF�� �޸� IO ��û �� ���μ����� IO burst time���� ���ؼ� ����ð� ����
// ��, cpu burst time (+ io burst time)�� ª�� ���μ��� �켱 ����

processPointer ALG_IASJF() {
	// ���� ���� ���μ����� ������ ��ȯ
	if (runningProcess != NULL && runningProcess->cpuRemainingTime > 0)
		return runningProcess;

	if (readySize == 0) // ready ť�� ������� NULL
		return NULL;

	int bestIndex = 0; // ���� ª�� effective time(cpu burst time + io burst time)�� ���� ���μ����� index
	int bestEffective = readyQueue[0]->cpuRemainingTime +
		(readyQueue[0]->hasRequestedIO ? 0 : readyQueue[0]->ioBurstTime); // IO ��û �� ���μ����� IO burst time���� ���ؼ�

	for (int i = 1; i < readySize; i++) {
		int effective = readyQueue[i]->cpuRemainingTime +
			(readyQueue[i]->hasRequestedIO ? 0 : readyQueue[i]->ioBurstTime);

		if (effective < bestEffective) { // effective time�� ª�� ���μ��� ã��
			bestEffective = effective;
			bestIndex = i;
		}
		else if (effective == bestEffective) { // ���� �ð��� ���� ������ tie-break
			if (readyQueue[i]->arrivalTime < readyQueue[bestIndex]->arrivalTime) {
				bestIndex = i;
			}
		}
	}

	return deleteReadyQueue(bestIndex); // effective time�� ª�� ���μ����� ready ť���� ���� (simulate()���� readyť -> runningProcess)
}

// ��(waiting time, turnaround time) ���׸� 10��
void evaluate() {
	int totalWaitingTime = 0; // ready ť�� �־��� �ð��� ����
	int totalTurnaroundTime = 0; // ���μ����� �����ؼ� �����Ҷ����� �ɸ� �� �ð�

	// terminated ť�� �ִ� ��� ���μ��� ��ȸ
	for (int i = 0; i < terminatedSize; i++) {
		totalWaitingTime += terminatedQueue[i]->waitingTime;
		totalTurnaroundTime += terminatedQueue[i]->turnaroundTime;
	}

	// ��� ���(average waiting time, average turnaround time)
	float avgWaitingTime = totalWaitingTime / (float)terminatedSize;
	float avgTurnaroundTime = totalTurnaroundTime / (float)terminatedSize;

	// ��� ���
	printf("\n-------------Evaluation Result-------------\n");
	printf("avgWaitingTime: %.2f\n", avgWaitingTime);
	printf("avgTurnaroundTime: %.2f\n", avgTurnaroundTime);
}


// �����Լ�
int main() {
	srand(time(NULL)); // ������ ������ �ٸ� rand�� �����ϰ�
	int numProcesses = 5; // �˰��� �񱳸� ���� ���μ��� ���� 5���� ����(���� ���� -> but, priority ������ ���� �����ؾ� ��)

	// ���� ���μ��� ���� ����(������ �־�� ��� �˰����� ���� ���μ��� �������� �� ����)
	processPointer baseProcesses[MAX_PROCESS_NUM];
	int baseSize = 0;
	for (int i = 0; i < numProcesses; i++) {
		baseProcesses[baseSize++] = createProcess();
	}

	// ��� �˰��� �ùķ��̼� �� ��
	for (int alg = FCFS; alg <= IASJF; alg++) {
		printf("\n==================================================================================\n");
		printf("%s\n", algorithmNames[alg]); // �˰��� �̸� ���(�Ʊ� �����ص� const char* algorithmNames[]���� ����)

		// ť �� �������� �ʱ�ȭ
		initQueues();

		// ���� ���μ��� ���� -> jobQueue�� ����
		for (int i = 0; i < baseSize; i++) {
			processPointer original = baseProcesses[i];
			processPointer clone = (processPointer)malloc(sizeof(Process));

			// original(�������μ���)�� ��� ���� clone�� ����
			clone->pid = original->pid;
			clone->arrivalTime = original->arrivalTime;
			clone->cpuBurstTime = original->cpuBurstTime;
			clone->cpuRemainingTime = original->cpuBurstTime;
			clone->priority = original->priority;

			clone->waitingTime = 0;
			clone->turnaroundTime = 0;
			clone->rrUsedTime = 0;

			clone->ioRequestTime = original->ioRequestTime;
			clone->ioBurstTime = original->ioBurstTime;
			clone->ioRemainingTime = 0;
			clone->isInIO = FALSE;
			clone->hasRequestedIO = FALSE;

			insertJobQueue(clone); // job ť�� ����
		}

		// job ť ����(arrival time ��������)
		sortJobQueue();

		// jobť ���(������)
		printJobQueue();

		// �ùķ��̼� ����(�˰��� ��ȣ�� ���� ȣ��)
		simulate(alg);

		// ��Ʈ��Ʈ ���
		printGanttChart(simulationEndTime);

		// ��(��� waiting/turnaround time)
		evaluate();

		// ������ terminated ť ���
		//printTerminatedQueue();

		// terminated ť �޸� ����
		for (int i = 0; i < terminatedSize; i++) {
			free(terminatedQueue[i]);
		}
	}

	// ���� ���μ��� �޸� ����
	for (int i = 0; i < baseSize; i++) {
		free(baseProcesses[i]);
	}
}