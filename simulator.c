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

// CPU 구조체 정의
typedef struct Process* processPointer;
typedef struct Process {
	int pid;
	int arrivalTime;
	int cpuBurstTime;
	int cpuRemainingTime;
	int ioBurstTime;
	int ioRemainingTime;
	int ioRequestTime; // 일단은 io 한번만 발생하게 설정(평가항목 2번 감점요인/ 추후에 수정하기)
	int priority; // 낮을 수록 높은 우선순위라 가정
	int waitingTime;
	int turnaroundTime;
	int responseTime; 
	int rrUsedTime;
}Process;

// 함수 원형 
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();

// 함수 배열
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin"
};


// 전역변수 및 큐 정의
// job 큐
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // 현재 job큐에 있는 프로세스 개수

// ready 큐
processPointer readyQueue[MAX_PROCESS_NUM]; 
int readySize = 0; // 현재 ready큐에 있는 프로세스 개수

// waiting 큐
processPointer waitingQueue[MAX_PROCESS_NUM]; 
int waitingSize = 0; // 현재 waiting큐에 있는 프로세스 개수

// terminated 큐
processPointer terminatedQueue[MAX_PROCESS_NUM]; 
int terminatedSize = 0; // 현재 terminated큐에 있는 프로세스 개수

// 현재 실행 중인 프로세스(running)
processPointer runningProcess = NULL; 

int currentTime = 0; // 현재 시간
int cpuIdleTime = 0; // cpu 유휴 시간
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid 랜덤생성할 때, 중복방지를 위해 생성된 pid들은 저장

int ganttChart[MAX_TIME_UNIT]; // 간트 차트 display용


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
		ganttChart[i] = -1;
	}
}


// pid를 랜덤으로 생성하는 함수
int generatePID() {
	while (1) {
		int pid = rand() % 1000 + 1; // 1~100범위
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
	newProcess->cpuBurstTime = rand() % 20 + 1; // 범위 1~20으로 설정
	newProcess->ioBurstTime = rand() % 8 + 3; // 적당히 범위 3~10으로 설정 
	newProcess->priority = rand() % 5 + 1; // 범위 1~5로 설정
	newProcess->ioRequestTime = rand() % (newProcess->cpuBurstTime - 1) + 1; // 범위 1~cpu burst time - 1로 설정

	// 상태 초기화
	newProcess->cpuRemainingTime = newProcess->cpuBurstTime;
	newProcess->ioRemainingTime = newProcess->ioBurstTime;
	newProcess->waitingTime = 0;
	newProcess->turnaroundTime = 0;
	newProcess->responseTime = -1; // 아직 cpu 할당받지 않았으니까 -1로 초기화
	newProcess->rrUsedTime = 0;

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

	for (int i = 0; i < jobSize - 1; i++) { // queue 한칸씩 앞으로 당기기
		jobQueue[i] = jobQueue[i + 1];
	}
	jobQueue[jobSize - 1] = NULL; // 큐 마지막은 NULL로
	jobSize--;

	return temp; // 빼둔 프로세스 반환
}

// job 큐 상태 출력
void printJobQueue() {
	if (jobSize == 0) printf("Job Queue is empty\n");
	for (int i = 0; i < jobSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n", 
			jobQueue[i]->pid, jobQueue[i]->arrivalTime, jobQueue[i]->cpuBurstTime, jobQueue[i]->ioBurstTime, jobQueue[i]->priority, jobQueue[i]->ioRequestTime);
	}
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

	for (int i = 0; i < readySize - 1; i++) { // queue 한칸씩 앞으로 당기기
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
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n", 
			readyQueue[i]->pid, readyQueue[i]->arrivalTime, readyQueue[i]->cpuBurstTime, readyQueue[i]->ioBurstTime, readyQueue[i]->priority, readyQueue[i]->ioRequestTime);
	}
}



// waiting 큐
// waiting 큐에 프로세스 삽입
void insertWaitingQueue(processPointer newProcess) {
	if (waitingSize < MAX_PROCESS_NUM) {
		waitingQueue[waitingSize] = newProcess; //queue에 하나씩 프로세스 넣기
		waitingSize++;
	}
	else {
		printf("Waiting Queue is already full\n");
	}
}

// waiting 큐에서 프로세스 빼기
processPointer deleteWaitingQueue(int index) {
	if (waitingSize < 0 || index >= waitingSize) return NULL;
	processPointer temp = waitingQueue[index]; // index의 프로세스 빼기
	for (int i = 0; i < waitingSize - 1; i++) { // queue 한칸씩 앞으로 당기기
		waitingQueue[i] = waitingQueue[i + 1];
	}
	waitingQueue[waitingSize - 1] = NULL; // 큐 마지막은 NULL로
	waitingSize--;

	return temp; // 빼둔 프로세스 반환
}

// waiting 큐 상태 출력
void printWaitingQueue() {
	if (waitingSize == 0) printf("Waiting Queue is empty\n");
	for (int i = 0; i < waitingSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n",
			waitingQueue[i]->pid, waitingQueue[i]->arrivalTime, waitingQueue[i]->cpuBurstTime, waitingQueue[i]->ioBurstTime, waitingQueue[i]->priority, waitingQueue[i]->ioRequestTime);
	}
}



// terminated큐
// terminated 큐에 프로세스 삽입
void insertTerminatedQueue(processPointer newProcess) {
	if (terminatedSize < MAX_PROCESS_NUM) {
		terminatedQueue[terminatedSize] = newProcess; //queue에 하나씩 프로세스 넣기
		terminatedSize++;
	}
	else {
		printf("Terminated Queue is already full\n");
	}
}

// terminated 큐 상태 출력
void printTerminatedQueue() {
	if (terminatedSize == 0) printf("Terminated Queue is empty\n");
	for (int i = 0; i < terminatedSize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d IO burst time: %d priority: %d IO request time: %d\n",
			terminatedQueue[i]->pid, terminatedQueue[i]->arrivalTime, terminatedQueue[i]->cpuBurstTime, terminatedQueue[i]->ioBurstTime, terminatedQueue[i]->priority, terminatedQueue[i]->ioRequestTime);
	}
}




// 프로세스 여러 개 생성하는 함수
void createProcesses(int n) {
	for (int i = 0; i < n; i++) {
		processPointer newProcess = createProcess(); //createProcess()함수 여러 번 호출해서!
		insertJobQueue(newProcess);
	}
	sortJobQueue();
}

// 간트 차트 출력 함수(평가 항목 9번)
// pid가 바뀔 때마다 출력해서 프로세스 시작과 끝만 표시저
void printGanttChart() {
	int previous = ganttChart[0]; // 이전 프로세스의 pid를 저장
	int start = 0; // 프로세스의 시작점 저장

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < MAX_TIME_UNIT; i++) {
		if (ganttChart[i] != previous) { // 프로세스가 다른 프로세스로 바뀌면 출력
			if (previous == -1) { 
				printf("|--idle--|"); // pid가 0이면 프로세스 없고 cpu idle 상태
			}
			else {
				printf("|--P%d--|", previous); // pid가 0이 아니면 pid 출력
			}
			printf(" %2d ", i);

			// 새로운 프로세스(구간) 시작
			start = i; 
			previous = ganttChart[i];
		}
	}
	// 마지막 구간 출력
	if (previous == -1)
		printf("|--idle--|");
	else
		printf("|--P%d--|", previous);
	printf(" %2d |\n", MAX_TIME_UNIT);
	}
}


// 시뮬레이션 함수
void simulate(int algorithm) {
	for (currentTime = 0; currentTime < MAX_TIME_UNIT; currentTime++) {
		// job 큐 -> ready 큐
		for (int i = 0; i < jobSize; i++) {
			if (jobQueue[i]->arrivalTime == currentTime) { // 현재시간 = 프로세스 도착시간, job 큐 -> ready 큐
				processPointer newProcess = deleteJobQueue(i);
				insertReadyQueue(newProcess);
				i--; // job 큐에서 하나 뺐으니까
			}
			else if (jobQueue[i]->arrivalTime > currentTime) break; // 아직 프로세스 도착 전
		}

		// waiting 큐 -> ready 큐
		for (int i = 0; i < waitingSize; i++) {
			waitingQueue[i]->ioRemainingTime--;
			waitingQueue[i]->turnaroundTime++;

			if (waitingQueue[i]->ioRemainingTime == 0) {
				processPointer newProcess = deleteWaitingQueue(i); // waiting 큐에서 빼서
				insertReadyQueue(newProcess); // ready 큐에 넣기
				i--;// ready 큐에서 하나 뺐으니까
			}
		}

		// 실행할 프로세스 선택
		processPointer selected = NULL;

		// 알고리즘 선택
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

		// 실행할 프로세스 없으면 cpu idle
		if (selected == NULL) {
			ganttChart[currentTime] = -1;
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		
		// 실행할 프로세스 있으면
		else {
			runningProcess = selected;
			ganttChart[currentTime] = runningProcess->pid;

			// response time 저장
			if (runningProcess->responseTime == -1) {
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime; // response time은 현재시간 - 도착시간
			}

			// RR일 때, time quantum 사용 체크
			runningProcess->rrTimeUsed++;

			// 실행
			runningProcess->cpuRemainingTime--;
			runningProcess->turnaroundTime++;
			printf("%d: PID %d is running (remaining: %d)\n",
				currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

			// IO request 발생(평가 항목 2번)
			if (runningProcess->cpuRemainingTime > 0 &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {
				// RR일 때
				runningProcess->rrTimeUsed = 0;
				// 아직 실행이 안끝났고 io 요청 시간에 도달했으면
				insertWaitingQueue(runningProcess); // waiting 큐로 보내야함
				runningProcess = NULL; // cpu는 idle로 변함
				continue;  
			}

			// cpu burst 완료
			if (runningProcess->cpuRemainingTime == 0) { // cpu 남은 시간 없으면
				// RR일 때
				runningProcess->rrTimeUsed = 0;
				insertTerminatedQueue(runningProcess); // terminated 큐로 이동
				runningProcess = NULL;
			}
		}
		// 대기 중인 다른 프로세스들 waitingTime 증가
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}
	}
}

// FCFS 알고리즘(평가항목 3번)
// ready 큐에 들어온 순서대로 실행
processPointer ALG_FCFS() {
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready 큐 맨 앞 프로세스 반환
	}
	return runningProcess; // 현재 실행중인 프로세스 그대로 반환
}

// Non-Preemptive SJF 알고리즘(평가항목 4번)
// 남은 cpu time 짧은 프로세스부터 실행
// 한번 프로세스가 시작되면 선점되지 않고 쭉 실행됨
processPointer ALG_NP_SJF() {
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL
	if (runningProcess == NULL) {
		int min_index = 0;
		for (int i = 0; i < readySize; i++) {
			// 남은 cpu time 가장 짧은 프로세스 찾기
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
		return deleteReadyQueue(min_index);
	}
	else {
		return runningProcess;
	}
}
// Preemptive SJF 알고리즘(평가항목 5번)
// 남은 cpu time 짧은 프로세스부터 실행
// 매 시간 cpu time을 체크해서 더 적은 실행시간을 가진 프로세스가 있으면 선점
processPointer ALG_P_SJF() {
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL
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
}

// Non-Preemptive PRIORITY 알고리즘(평가항목 6번)
// 한번 프로세스가 시작하면 선점되지 않고 쭉 실행됨
// Priority가 높은 순서대로 실행
processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL{
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
	else {
		return runningProcess;
	}
}

// Non-Preemptive PRIORITY 알고리즘(평가항목 6번)
// Priority가 높은 순서대로 실행
// 매 시간 priority를 체크해서 더 높은 priority를 가진 프로세스가 있으면 선점
processPointer ALG_P_PRIORITY() {
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL
	
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
	if (readySize == 0) return NULL; // ready 큐 비어있으면 NULL

	// 실행중인 프로세스가 없는 경우
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0);
		nextProcessor->rrTimeUsed = 0; // 새로 실행할 프로세서 사용 시간 초기화
		return nextProcessor;
	}
	// 실행 중인 프로세스가 타임 퀀텀만큼 실행된 경우
	if (runningProcess->rrTimeUsed >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // 다시 ready 큐로
		processPointer nextProcessor = deleteReadyQueue(0); // 다음 프로세스 실행
		nextProcessor->rrTimeUsed = 0; // 새로 시작하므로 초기화
		return nextProcessor;
	}
	// 아직 타임퀀텀 끝나기 전이면
	return runningProcess;
}


// 같은 프로세스로 평가해야 하니까 복제 함수 필요(job 큐에서)
void cloneProcessesfromJQ() {
	readySize = 0;
	for (int i = 0; i < jobSize; i++) {
		processPointer original = jobQueue[i];
		processPointer cloned = (processPointer)malloc(sizeof(Process));

		// 모든 필드 복사
		cloned->pid = original->pid;
		cloned->arrivalTime = original->arrivalTime;
		cloned->cpuBurstTime = original->cpuBurstTime;
		cloned->cpuRemainingTime = original->cpuBurstTime; // 항상 새로 시작
		cloned->ioBurstTime = original->ioBurstTime;
		cloned->ioRemainingTime = original->ioBurstTime;
		cloned->ioRequestTime = original->ioRequestTime;
		cloned->priority = original->priority;

		// 초기화
		cloned->waitingTime = 0;
		cloned->turnaroundTime = 0;
		cloned->responseTime = -1;
		cloned->rrTimeUsed = 0;

		readyQueue[readySize++] = cloned; // ready 큐 다시 채우기
	}
}

// 평가(waiting time, turnaround time, response time) 평가항목 10번
void evaluate() {
	int totalWaitingTime = 0;
	int totalTurnaroundTime = 0;
	int totalResponseTime = 0;

	// terminated 큐에 있는 모든 프로세스 순회
	for (int i = 0; i < terminatedSize; i++) {
		totalWaitingTime += terminatedQueue[i]->waitingTime;
		totalTurnaroundTime += terminatedQueue[i]->turnaroundTime;
		totalResponseTime += terminatedQueue[i]->responseTime;
	}
	
	// 평균 계산(average waiting time, average turnaround time, average response time)
	float avgWaitingTime = totalWaitingTime / (float)terminatedSize;
	float avgTurnaroundTime = totalTurnaroundTime / (float)terminatedSize;
	float avgResponseTime = totalResponseTime / (float)terminatedSize;

	// 결과 출력
	printf("\n-------------Evaluation Result-------------\n");
	printf("avgWaitingTime: %.2f\n", avgWaitingTime);
	printf("avgTurnaroundTime: %.2f\n", avgTurnaroundTime);
	printf("avgResponseTime: %.2f\n", avgResponseTime);
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
			clone->ioBurstTime = original->ioBurstTime;
			clone->ioRemainingTime = original->ioBurstTime;
			clone->ioRequestTime = original->ioRequestTime;
			clone->priority = original->priority;

			clone->waitingTime = 0;
			clone->turnaroundTime = 0;
			clone->responseTime = -1;
			clone->rrTimeUsed = 0;

			insertJobQueue(clone); // job 큐에 저장
		}

		// 시뮬레이션 실행
		simulate(alg);

		// 간트차트 출력
		printGanttChart();

		// 평가
		evaluate();

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

