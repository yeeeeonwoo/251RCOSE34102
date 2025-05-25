#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TIME_UNIT 1000
#define MAX_PROCESS_NUM 30
#define TIME_QUANTUM 3

#define TRUE 1
#define FALSE 0

// CPU 스케줄링 알고리즘 상수
#define FCFS 0
#define NP_SJF 1
#define P_SJF 2
#define NP_PRIORITY 3
#define P_PRIORITY 4
#define RR 5


// 함수 배열
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin"
};


// CPU 구조체 정의
typedef struct Process* processPointer;
typedef struct Process {
	int pid;
	int arrivalTime;
	int cpuBurstTime;
	int cpuRemainingTime;
	int priority; // 낮을 수록 높은 우선순위라 가정
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


// 함수 원형 
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();


// 전역변수 및 큐 정의
// job 큐
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // 현재 job큐에 있는 프로세스 개수

// ready 큐
processPointer readyQueue[MAX_PROCESS_NUM];
int readySize = 0; // 현재 ready큐에 있는 프로세스 개수

processPointer waitingQueue[MAX_PROCESS_NUM];
int waitingSize = 0;


// terminated 큐
processPointer terminatedQueue[MAX_PROCESS_NUM];
int terminatedSize = 0; // 현재 terminated큐에 있는 프로세스 개수

// 현재 실행 중인 프로세스(running)
processPointer runningProcess = NULL;

int currentTime = 0; // 현재 시간
int cpuIdleTime = 0; // cpu 유휴 시간
int simulationEndTime = 0;
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid 랜덤생성할 때, 중복방지를 위해 생성된 pid들은 저장
int ganttChart[MAX_TIME_UNIT]; // 간트 차트 display용
int ioRequestedAt[MAX_TIME_UNIT];

// 큐를 초기화하는 함수
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


// pid를 랜덤으로 생성하는 함수
int generatePID() {
	while (1) {
		int pid = rand() % 10 + 1; // 1~10범위
		int duplicate = 0; // 중복체크
		for (int i = 0; i < MAX_PROCESS_NUM; i++) {
			if (usedPID[i] == pid) { //이미 사용된 pid이면 break
				duplicate = 1;
				break;
			}
		}
		if (!duplicate) { // 사용되지 않은 pid이면 usedPID에 저장하고 break
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


// 프로세스 생성하는 함수
processPointer createProcess() {
	processPointer newProcess = (processPointer)malloc(sizeof(Process));

	// random data 부여(평가항목 1번)
	newProcess->pid = generatePID();
	newProcess->arrivalTime = rand() % 10 + 1; // 범위 1~10으로 설정
	newProcess->cpuBurstTime = rand() % 20 + 2; // 범위 2~20으로 설정
	newProcess->priority = rand() % 5 + 1; // 범위 1~5로 설정

	// 상태 초기화
	newProcess->cpuRemainingTime = newProcess->cpuBurstTime;
	newProcess->waitingTime = 0;
	newProcess->turnaroundTime = 0;
	newProcess->responseTime = -1; // 아직 cpu 할당받지 않았으니까 -1로 초기화
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

// job 큐
// job 큐에 프로세스 삽입
void insertJobQueue(processPointer newProcess) {
	if (jobSize < MAX_PROCESS_NUM) {
		jobQueue[jobSize] = newProcess; //queue에 하나씩 프로세스 넣기
		jobSize++;
	}
	else {
		printf("Job Queue is already full\n");
	}
}


// job 큐에서 프로세스 빼기(현재 시간에 arrive한 프로세스)
processPointer deleteJobQueue(int index) {
	if (jobSize < 0 || index >= jobSize) return NULL;
	processPointer temp = jobQueue[index]; // index의 프로세스 빼기

	for (int i = index; i < jobSize - 1; i++) { // queue 한칸씩 앞으로 당기기
		jobQueue[i] = jobQueue[i + 1];
	}
	jobQueue[jobSize - 1] = NULL; // 큐 마지막은 NULL로
	jobSize--;

	return temp; // 빼둔 프로세스 반환
}

// job 큐 상태 출력
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

// job 큐 arrival time 기준으로 정렬하는 함수
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




// ready 큐
// ready 큐에 프로세스 삽입
void insertReadyQueue(processPointer newProcess) {
	if (readySize < MAX_PROCESS_NUM) {
		readyQueue[readySize] = newProcess; //queue에 하나씩 프로세스 넣기
		readySize++;
	}
	else {
		printf("Ready Queue is already full\n");
	}
}


// ready 큐에서 프로세스 빼기
processPointer deleteReadyQueue(int index) {
	if (readySize < 0 || index >= readySize) return NULL;

	processPointer temp = readyQueue[index]; // index의 프로세스 빼기

	for (int i = index; i < readySize - 1; i++) { // queue 한칸씩 앞으로 당기기
		readyQueue[i] = readyQueue[i + 1];
	}
	readyQueue[readySize - 1] = NULL; // 큐 마지막은 NULL로
	readySize--;

	return temp; // 빼둔 프로세스 반환
}


// ready 큐 상태 출력
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


// terminated큐
// terminated 큐에 프로세스 삽입
void insertTerminatedQueue(processPointer newProcess) {
	if (newProcess == NULL) return;

	// 중복 삽입 방지
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


// 출력
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


// 간트 차트 출력
void printGanttChart(int endTime) {
	int previous = ganttChart[0]; // 이전 프로세스의 pid
	int start = 0;

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < endTime; i++) {
		if (ganttChart[i] != previous) {
			// 구간 출력
			if (previous == -1) {
				printf("|--idle--|");
			}
			else if (previous == -2) {
				if (ioRequestedAt[start] != -1)
					printf("|--P%d(IO)--|", ioRequestedAt[start]);
				else
					printf("|--IO--|");  // 예외 방지
			}
			else {
				printf("|--P%d--|", previous);
			}
			printf(" %2d ", i);
			start = i;
			previous = ganttChart[i];
		}
	}

	// 마지막 구간 출력
	if (previous == -1) {
		printf("|--idle--|");
	}
	else if (previous == -2) {
		if (ioRequestedAt[start] != -1)
			printf("|--P%d(IO)--|", ioRequestedAt[start]);
		else
			printf("|--IO--|");  // 예외 방지
	}
	else {
		printf("|--P%d--|", previous);
	}
	printf(" %2d |\n", endTime);
}


void simulate(int algorithm) {
	for (currentTime = 0; currentTime < MAX_TIME_UNIT; currentTime++) {

		// I/O 완료된 프로세스 있는지  확인
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

		// 종료된 running 프로세스 처리
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

		
		// 전체 종료 조건
		if (jobSize == 0 && readySize == 0 && waitingSize == 0 && runningProcess == NULL) {
			simulationEndTime = currentTime;
			break;
		}


		// 스케줄링
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

			// response time 기록
			if (runningProcess->responseTime == -1)
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime;

			// I/O 요청 조건: 아직 요청하지 않았고, 특정 시간 도달
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
				// CPU에서 정상 실행
				runningProcess->cpuRemainingTime--;
				runningProcess->turnaroundTime++;
				runningProcess->rrUsedTime++;

				printf("%d: PID %d is running (remaining: %d)\n",
					currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

				// 실행 종료 시 terminate
				if (runningProcess->cpuRemainingTime == 0) {
					insertTerminatedQueue(runningProcess);
					runningProcess = NULL;
				}
			}
		}


		// readyQueue 대기시간 증가
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}

		//  간트차트 기록은 시뮬레이션 루프의 마지막에서 수행
		if (runningProcess != NULL)
			ganttChart[currentTime] = runningProcess->pid;
		else if (ioRequestedAt[currentTime] != -1)
			ganttChart[currentTime] = -2;  // IO 요청 구간
		else
			ganttChart[currentTime] = -1;  // idle

		simulationEndTime = currentTime;
		
	}

}


// FCFS 알고리즘(평가항목 3번)
// ready 큐에 들어온 순서대로 실행
processPointer ALG_FCFS() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		return NULL; // simulate()가 종료 처리 담당
	}
	if (readySize == 0) return runningProcess;
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready 큐 맨 앞 프로세스 반환
	}
	return runningProcess; // 현재 실행중인 프로세스 그대로 반환
}


// Non-Preemptive SJF 알고리즘(평가항목 4번)
// 남은 cpu time 짧은 프로세스부터 실행
// 한번 프로세스가 시작되면 선점되지 않고 쭉 실행됨
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
		// 먼저 burstTime 기준으로 비교
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// 같으면 도착 시간 기준으로 tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// burst time이 같다면 도착 시간이 빠른 순
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// 선택한 프로세스를 임시 저장 후 큐에서 삭제
	processPointer selected = readyQueue[min_index];
	deleteReadyQueue(min_index);
	return selected;
}


// Preemptive SJF 알고리즘(평가항목 5번)
// 남은 cpu time 짧은 프로세스부터 실행
// 매 시간 cpu time을 체크해서 더 적은 실행시간을 가진 프로세스가 있으면 선점
processPointer ALG_P_SJF() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready 큐 비어있으면 NULL
	int min_index = 0;
	for (int i = 0; i < readySize; i++) {
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// 남은 cpu time이 같으면 도착 시간 기준으로
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
	// preemptive 체크
	if (runningProcess == NULL) { // 먼저 실행 중인 프로세스 없으면 그냥 ready 큐에서 가져오면 됨
		return deleteReadyQueue(min_index);
	}
	// 실행 중인 프로세스보다 새로 들어온 프로세스가 짧으면 선점
	if (runningProcess->cpuRemainingTime > readyQueue[min_index]->cpuRemainingTime) {
		insertReadyQueue(runningProcess);
		return deleteReadyQueue(min_index);
	}
	// 아니면 그냥 기존 프로세스 계속 실행
	return runningProcess;
}


// Non-Preemptive PRIORITY 알고리즘(평가항목 6번)
// 한번 프로세스가 시작하면 선점되지 않고 쭉 실행됨
// Priority가 높은 순서대로 실행
processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready 큐 비어있으면 NULL
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
	if (runningProcess == NULL) {
		int min_index = 0;
		for (int i = 0; i < readySize; i++) {
			//  priority 가장 높은 프로세스 찾기
			if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
				min_index = i;
			}
			// priority가 같으면 도착 시간 기준으로
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


// Preemptive PRIORITY 알고리즘(평가항목 6번)
// Priority가 높은 순서대로 실행
// 매 시간 priority를 체크해서 더 높은 priority를 가진 프로세스가 있으면 선점
processPointer ALG_P_PRIORITY() {
	if (readySize == 0) return (runningProcess != NULL) ? runningProcess : NULL; // ready 큐 비어있으면 NULL
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}
	int min_index = 0;
	for (int i = 0; i < readySize; i++) {
		//  priority 가장 높은 프로세스 찾기
		if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
			min_index = i;
		}
		// priority가 같으면 도착 시간 기준으로
		else if (readyQueue[i]->priority == readyQueue[min_index]->priority) {
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// preemptive 체크
	if (runningProcess == NULL) {
		return deleteReadyQueue(min_index); // 먼저 실행 중인 프로세스 없으면 그냥 ready 큐에서 가져오면 됨
	}
	// 실행 중인 프로세스보다 새로 들어온 프로세스가 priority가 높으면 선점
	if (runningProcess->priority > readyQueue[min_index]->priority) {
		insertReadyQueue(runningProcess);
		return deleteReadyQueue(min_index);
	}
	// 아니면 그냥 기존 프로세스 계속 실행
	return runningProcess;
}


// Round-Robbin 알고리즘(평가항목 7번)
// 각 프로세스가 time quantum만큼 실행되고 그 다음 프로세스로 넘어감
// 프로세스가 끝나지 않으면 ready 큐 맨 뒤로
processPointer ALG_RR() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		runningProcess = NULL;
	}

	if (readySize == 0) {
		return (runningProcess != NULL) ? runningProcess : NULL;
	}
	// 실행중인 프로세스가 없는 경우
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0);
		nextProcessor->rrUsedTime = 0; // 새로 실행할 프로세서 사용 시간 초기화
		return nextProcessor;
	}
	// 실행 중인 프로세스가 타임 퀀텀만큼 실행된 경우
	if (runningProcess->rrUsedTime >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // 다시 ready 큐로
		processPointer nextProcessor = deleteReadyQueue(0); // 다음 프로세스 실행
		nextProcessor->rrUsedTime = 0; // 새로 시작하므로 초기화
		return nextProcessor;
	}
	// 아직 타임퀀텀 끝나기 전이면
	return runningProcess;
}



// 평가(waiting time, turnaround time, response time) 평가항목 10번
void evaluate() {
	int totalWaitingTime = 0;
	int totalTurnaroundTime = 0;

	// terminated 큐에 있는 모든 프로세스 순회
	for (int i = 0; i < terminatedSize; i++) {
		totalWaitingTime += terminatedQueue[i]->waitingTime;
		totalTurnaroundTime += terminatedQueue[i]->turnaroundTime;
	}

	// 평균 계산(average waiting time, average turnaround time)
	float avgWaitingTime = totalWaitingTime / (float)terminatedSize;
	float avgTurnaroundTime = totalTurnaroundTime / (float)terminatedSize;

	// 결과 출력
	printf("\n-------------Evaluation Result-------------\n");
	printf("avgWaitingTime: %.2f\n", avgWaitingTime);
	printf("avgTurnaroundTime: %.2f\n", avgTurnaroundTime);
}


// 메인함수
int main() {
	srand(time(NULL));
	int numProcesses = 5; // 프로세스 개수 5개로 설정

	// 기준 프로세스 집합 생성(기준이 있어야 같은 프로세스들로 평가 가능)
	processPointer baseProcesses[MAX_PROCESS_NUM];
	int baseSize = 0;
	for (int i = 0; i < numProcesses; i++) {
		baseProcesses[baseSize++] = createProcess();
	}

	// 모든 알고리즘 실행 및 평가
	for (int alg = FCFS; alg <= RR; alg++) {
		printf("\n============================================\n");
		printf("%s\n", algorithmNames[alg]);

		// 초기화
		initQueues();

		// 기준 프로세스 복제 → jobQueue에 저장
		for (int i = 0; i < baseSize; i++) {
			processPointer original = baseProcesses[i];
			processPointer clone = (processPointer)malloc(sizeof(Process));

			// 모든 정보 복사
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

			insertJobQueue(clone); // job 큐에 저장
		}

		// job 큐 정렬
		sortJobQueue();

		// job큐 출력
		printJobQueue();

		// 시뮬레이션 실행
		simulate(alg);

		// 간트차트 출력
		printGanttChart(simulationEndTime);

		// 평가
		evaluate();

		// 디버깅용 terminated 큐 출력
		printTerminatedQueue();

		// terminated 큐 메모리 해제
		for (int i = 0; i < terminatedSize; i++) {
			free(terminatedQueue[i]);
		}
	}

	// base 프로세스 메모리 해제
	for (int i = 0; i < baseSize; i++) {
		free(baseProcesses[i]);
	}
}