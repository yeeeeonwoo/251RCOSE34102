#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS_NUM 30
#define TIME_QUANTUM 3

#define TRUE 1
#define FALSE 0


// CPU 스케줄링 알고리즘 상수
#define FCFS 0
#define SJF 1
#define PRIORITY 2
#define RR 3


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
	int priority;
	int waitingTime;
	int turnaroundTime;
	int responseTime; 
}Process;


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
				selected = FCFS();
				break;
		}

		// 실행할 프로세스 없으면 cpu idle
		if (selected == NULL) {
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		
		// 실행할 프로세스 있으면
		else {
			runningProcess = selected;

			// response time 저장
			if (runningProcess->responseTime == -1) {
				runningProcess->responseTime = currentTime - runningProcess->arrivalTime; // response time은 현재시간 - 도착시간
			}

			// 실행
			runningProcess->cpuRemainingTime--;
			runningProcess->turnaroundTime++;
			printf("%d: PID %d is running (remaining: %d)\n",
				currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

			// IO request 발생
			if (runningProcess->cpuRemainingTime > 0 &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {
				// 아직 실행이 안끝났고 io 요청 시간에 도달했으면
				insertWaitingQueue(runningProcess); // waiting 큐로 보내야함
				runningProcess = NULL; // cpu는 idle로 변함
				continue;  
			}

			// cpu burst 완료
			if (runningProcess->cpuRemainingTime == 0) { // cpu 남은 시간 없으면
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
// cpu burst 끝나면 terminated 큐로
// 실행하다가 io request 있으면 waiting 큐로

processPointer FCFS() {
	if (readySize == 0) return NULL;
	if (runningProcess == NULL) {
		return deleteReadyQueue(0); // ready 큐 맨 앞 프로세스 반환
	}
	return runningProcess; // 현재 실행중인 프로세스 그대로 반환
}