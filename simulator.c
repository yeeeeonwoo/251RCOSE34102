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

int currentTime = 0; // 총 시간
int cpuIdleTime = 0; // cpu 유휴 시간
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid 랜덤생성할 때, 중복방지를 위해 생성된 pid들은 저장

// 큐를 초기화하는 함수
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
void createProcess() {
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

