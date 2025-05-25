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


// �Լ� �迭
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin"
};


// CPU ����ü ����
typedef struct Process* processPointer;
typedef struct Process {
	int pid;
	int arrivalTime;
	int cpuBurstTime;
	int cpuRemainingTime;
	int priority; // ���� ���� ���� �켱������ ����
	int waitingTime;
	int turnaroundTime;
	int responseTime;
	int rrUsedTime;
	int ioRequestTime;
	int ioBurstTime;
	int ioRemainingTime;
	int isInIO;
	int hasRequestedIO;
}Process;


// �Լ� ���� 
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();


// �������� �� ť ����
// job ť
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // ���� jobť�� �ִ� ���μ��� ����

// ready ť
processPointer readyQueue[MAX_PROCESS_NUM];
int readySize = 0; // ���� readyť�� �ִ� ���μ��� ����

processPointer waitingQueue[MAX_PROCESS_NUM];
int waitingSize = 0;


// terminated ť
processPointer terminatedQueue[MAX_PROCESS_NUM];
int terminatedSize = 0; // ���� terminatedť�� �ִ� ���μ��� ����

// ���� ���� ���� ���μ���(running)
processPointer runningProcess = NULL;

int currentTime = 0; // ���� �ð�
int cpuIdleTime = 0; // cpu ���� �ð�
int simulationEndTime = 0;
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid ���������� ��, �ߺ������� ���� ������ pid���� ����
int ganttChart[MAX_TIME_UNIT]; // ��Ʈ ��Ʈ display��
int ioRequestedAt[MAX_TIME_UNIT];

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
	}

	for (int i = 0; i < MAX_TIME_UNIT; i++) {
		ganttChart[i] = -1;
		ioRequestedAt[i] = -1;  
	}
}


// pid�� �������� �����ϴ� �Լ�
int generatePID() {
	while (1) {
		int pid = rand() % 10 + 1; // 1~10����
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
	newProcess->cpuBurstTime = rand() % 20 + 2; // ���� 2~20���� ����
	newProcess->priority = rand() % 5 + 1; // ���� 1~5�� ����

	// ���� �ʱ�ȭ
	newProcess->cpuRemainingTime = newProcess->cpuBurstTime;
	newProcess->waitingTime = 0;
	newProcess->turnaroundTime = 0;
	newProcess->responseTime = -1; // ���� cpu �Ҵ���� �ʾ����ϱ� -1�� �ʱ�ȭ
	newProcess->rrUsedTime = 0;

	newProcess->ioRequestTime = rand() % (newProcess->cpuBurstTime - 1) + 1;
	newProcess->ioBurstTime = rand() % 5 + 2;
	newProcess->ioRemainingTime = 0;
	newProcess->isInIO = FALSE;
	newProcess->hasRequestedIO = FALSE;

	printf("PID %d: burst=%d, ioRequest=%d, ioBurst=%d\n",
	newProcess->pid,
	newProcess->cpuBurstTime,
	newProcess->ioRequestTime,
	newProcess->ioBurstTime);


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

	for (int i = index; i < jobSize - 1; i++) { // queue ��ĭ�� ������ ����
		jobQueue[i] = jobQueue[i + 1];
	}
	jobQueue[jobSize - 1] = NULL; // ť �������� NULL��
	jobSize--;

	return temp; // ���� ���μ��� ��ȯ
}

// job ť ���� ���
void printJobQueue() {
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
	printf("----------------------------------------------------------------------\n");
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

	for (int i = index; i < readySize - 1; i++) { // queue ��ĭ�� ������ ����
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
		printf("PID: %d Arrival time: %d CPU burst time: %d Priority: %d\n",
			readyQueue[i]->pid, readyQueue[i]->arrivalTime,
			readyQueue[i]->cpuBurstTime, readyQueue[i]->priority); 
	}
}

void insertWaitingQueue(processPointer p) {
	if (waitingSize < MAX_PROCESS_NUM) {
		waitingQueue[waitingSize++] = p;
	}
}

processPointer deleteWaitingQueue(int index) {
	if (index >= waitingSize) return NULL;
	processPointer temp = waitingQueue[index];
	for (int i = index; i < waitingSize - 1; i++) {
		waitingQueue[i] = waitingQueue[i + 1];
	}
	waitingQueue[--waitingSize] = NULL;
	return temp;
}

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
		terminatedQueue[terminatedSize++] = newProcess;
	}
	else {
		printf("Terminated Queue is full!\n");
	}
}


// ���
void printTerminatedQueue() {
	if (terminatedSize == 0) {
		printf("Terminated Queue is empty\n");
		return;
	}
	printf("\n-------------Terminated Processes-------------\n");
	printf("PID  Arrival  Burst  Priority  Waiting  Turnaround  Response\n");
	for (int i = 0; i < terminatedSize; i++) {
		processPointer p = terminatedQueue[i];
		printf("%3d  %7d  %5d  %8d  %7d  %10d  %8d\n",
			p->pid, p->arrivalTime, p->cpuBurstTime, p->priority,
			p->waitingTime, p->turnaroundTime, p->responseTime);
	}
}


// ��Ʈ ��Ʈ ���
void printGanttChart(int endTime) {
	int previous = ganttChart[0]; // ���� ���μ����� pid
	int start = 0;

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < endTime; i++) {
		if (ganttChart[i] != previous) {
			// ���� ���
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

		// I/O �Ϸ�� ���μ��� �ִ���  Ȯ��
		for (int i = 0; i < waitingSize; i++) {
			waitingQueue[i]->ioRemainingTime--;

			if (waitingQueue[i]->ioRemainingTime <= 0) {
				waitingQueue[i]->isInIO = FALSE;

				printf("%d: PID %d I/O complete (back to ready queue)\n",
					currentTime, waitingQueue[i]->pid);

				insertReadyQueue(deleteWaitingQueue(i));
				i--;
			}
		}

		// ����� running ���μ��� ó��
		if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
			insertTerminatedQueue(runningProcess);
			runningProcess = NULL;
		}

		// jobQueue -> readyQueue
		for (int i = 0; i < jobSize; i++) {
			if (jobQueue[i]->arrivalTime == currentTime) {
				processPointer newProcess = deleteJobQueue(i);
				insertReadyQueue(newProcess);
				i--;
			}
		}

		
		// ��ü ���� ����
		if (jobSize == 0 && readySize == 0 && waitingSize == 0 && runningProcess == NULL) {
			simulationEndTime = currentTime;
			break;
		}


		// �����ٸ�
		processPointer selected = NULL;
		switch (algorithm) {
			case FCFS: selected = ALG_FCFS(); break;
			case NP_SJF: selected = ALG_NP_SJF(); break;
			case P_SJF: selected = ALG_P_SJF(); break;
			case NP_PRIORITY: selected = ALG_NP_PRIORITY(); break;
			case P_PRIORITY: selected = ALG_P_PRIORITY(); break;
			case RR: selected = ALG_RR(); break;
		}

		if (selected == NULL) {
			runningProcess = NULL;
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		else {
			runningProcess = selected;

			// response time ���
			if (runningProcess->responseTime == -1)
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime;

			// I/O ��û ����: ���� ��û���� �ʾҰ�, Ư�� �ð� ����
			if (!runningProcess->hasRequestedIO &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {

				ioRequestedAt[currentTime] = runningProcess->pid;
				runningProcess->isInIO = TRUE;
				runningProcess->hasRequestedIO = TRUE;  
				runningProcess->ioRemainingTime = runningProcess->ioBurstTime;

				printf("%d: PID %d is requesting I/O for %d units\n",
					currentTime, runningProcess->pid, runningProcess->ioBurstTime);

				insertWaitingQueue(runningProcess);
				ganttChart[currentTime] = -2;
				runningProcess = NULL;
				cpuIdleTime++;
				continue;
			}
			else {
				// CPU���� ���� ����
				runningProcess->cpuRemainingTime--;
				runningProcess->turnaroundTime++;
				runningProcess->rrUsedTime++;

				printf("%d: PID %d is running (remaining: %d)\n",
					currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

				// ���� ���� �� terminate
				if (runningProcess->cpuRemainingTime == 0) {
					insertTerminatedQueue(runningProcess);
					runningProcess = NULL;
				}
			}
		}


		// readyQueue ���ð� ����
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}

		//  ��Ʈ��Ʈ ����� �ùķ��̼� ������ ���������� ����
		if (runningProcess != NULL)
			ganttChart[currentTime] = runningProcess->pid;
		else if (ioRequestedAt[currentTime] != -1)
			ganttChart[currentTime] = -2;  // IO ��û ����
		else
			ganttChart[currentTime] = -1;  // idle

		simulationEndTime = currentTime;
		
	}

}


// FCFS �˰���(���׸� 3��)
// ready ť�� ���� ������� ����
processPointer ALG_FCFS() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		return NULL; // simulate()�� ���� ó�� ���
	}
	if (readySize == 0) return runningProcess;
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready ť �� �� ���μ��� ��ȯ
	}
	return runningProcess; // ���� �������� ���μ��� �״�� ��ȯ
}


// Non-Preemptive SJF �˰���(���׸� 4��)
// ���� cpu time ª�� ���μ������� ����
// �ѹ� ���μ����� ���۵Ǹ� �������� �ʰ� �� �����
processPointer ALG_NP_SJF() {
	if (runningProcess != NULL) {
		if (runningProcess->cpuRemainingTime > 0)
			return runningProcess;
		else
			return NULL;
	}

	if (readySize == 0) return NULL;

	int min_index = 0;
	for (int i = 1; i < readySize; i++) {
		// ���� burstTime �������� ��
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// ������ ���� �ð� �������� tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// burst time�� ���ٸ� ���� �ð��� ���� ��
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// ������ ���μ����� �ӽ� ���� �� ť���� ����
	processPointer selected = readyQueue[min_index];
	deleteReadyQueue(min_index);
	return selected;
}


// Preemptive SJF �˰���(���׸� 5��)
// ���� cpu time ª�� ���μ������� ����
// �� �ð� cpu time�� üũ�ؼ� �� ���� ����ð��� ���� ���μ����� ������ ����
processPointer ALG_P_SJF() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready ť ��������� NULL
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
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
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


// Non-Preemptive PRIORITY �˰���(���׸� 6��)
// �ѹ� ���μ����� �����ϸ� �������� �ʰ� �� �����
// Priority�� ���� ������� ����
processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready ť ��������� NULL
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
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
	return runningProcess;
}


// Preemptive PRIORITY �˰���(���׸� 6��)
// Priority�� ���� ������� ����
// �� �ð� priority�� üũ�ؼ� �� ���� priority�� ���� ���μ����� ������ ����
processPointer ALG_P_PRIORITY() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready ť ��������� NULL
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
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
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}

	if (readySize == 0) {
		return (runningProcess != NULL) ? runningProcess : NULL;
	}
	// �������� ���μ����� ���� ���
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0);
		nextProcessor->rrUsedTime = 0; // ���� ������ ���μ��� ��� �ð� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� ���� ���μ����� Ÿ�� ���Ҹ�ŭ ����� ���
	if (runningProcess->rrUsedTime >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // �ٽ� ready ť��
		processPointer nextProcessor = deleteReadyQueue(0); // ���� ���μ��� ����
		nextProcessor->rrUsedTime = 0; // ���� �����ϹǷ� �ʱ�ȭ
		return nextProcessor;
	}
	// ���� Ÿ������ ������ ���̸�
	return runningProcess;
}



// ��(waiting time, turnaround time, response time) ���׸� 10��
void evaluate() {
	int totalWaitingTime = 0;
	int totalTurnaroundTime = 0;

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
			clone->priority = original->priority;

			clone->waitingTime = 0;
			clone->turnaroundTime = 0;
			clone->responseTime = -1;
			clone->rrUsedTime = 0;

			clone->ioRequestTime = original->ioRequestTime;
			clone->ioBurstTime = original->ioBurstTime;
			clone->ioRemainingTime = 0;
			clone->isInIO = FALSE;
			clone->hasRequestedIO = FALSE;

			insertJobQueue(clone); // job ť�� ����
		}

		// job ť ����
		sortJobQueue();

		// jobť ���
		printJobQueue();

		// �ùķ��̼� ����
		simulate(alg);

		// ��Ʈ��Ʈ ���
		printGanttChart(simulationEndTime);

		// ��
		evaluate();

		// ������ terminated ť ���
		printTerminatedQueue();

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