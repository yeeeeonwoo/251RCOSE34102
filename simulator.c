#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TIME_UNIT 1000 // 간트차트 최대길이
#define MAX_PROCESS_NUM 30 // 최대 프로세스 수
#define TIME_QUANTUM 3 // RR에서 사용할 타임퀀텀

#define TRUE 1
#define FALSE 0

// 스케줄링 알고리즘 상수매핑(simulate()에서 switch-case문에 사용됨)
#define FCFS 0
#define NP_SJF 1
#define P_SJF 2
#define NP_PRIORITY 3
#define P_PRIORITY 4
#define RR 5
#define IASJF 6


// 스케줄링 알고리즘 이름 배열(출력용)
const char* algorithmNames[] = {
	"FCFS",
	"Non-Preemptive SJF",
	"Preemptive SJF",
	"Non-Preemptive Priority",
	"Preemptive Priority",
	"Round Robin",
	"IO-Aware SJF",
};


// CPU 구조체 정의
typedef struct Process {
	// 프로세스 기본정보
	int pid;
	int arrivalTime;
	int priority; // 낮을 수록 높은 우선순위라 가정

	// cpu burst 관련 정보
	int cpuBurstTime;
	int cpuRemainingTime;
	int rrUsedTime; // RR 알고리즘에서 프로세스가 현재 사용한 시간(타임퀀텀 끝났는지 확인용)

	// io burst 관련 정보
	int ioRequestTime; // IO 요청 시점(cpu가 몇초 사용된 이후인지)
	int ioBurstTime;
	int ioRemainingTime;
	int isInIO; // 지금 IO 대기중인지 아닌지
	int hasRequestedIO; // 전에 IO 요청한적 있는지(IO는 한번만 랜덤으로 발생한다 가정)

	// evaluation 정보
	int waitingTime; // 총 대기시간(프로세스가 ready 큐에 있는 시간 합)
	int turnaroundTime; // 종료시간 - 도착시간

}Process;

// 구조체 포인터 정의
typedef struct Process* processPointer;

// 함수 원형 선언
processPointer ALG_FCFS();
processPointer ALG_NP_SJF();
processPointer ALG_P_SJF();
processPointer ALG_NP_PRIORITY();
processPointer ALG_P_PRIORITY();
processPointer ALG_RR();
processPointer ALG_IASJF();


// 큐 및 전역변수 정의

// job 큐(아직 도착하지 않은 프로세스들)
processPointer jobQueue[MAX_PROCESS_NUM];
int jobSize = 0; // 현재 job큐에 있는 프로세스 개수

// ready 큐(cpu 사용 대기중인 프로세스들)
processPointer readyQueue[MAX_PROCESS_NUM];
int readySize = 0; // 현재 ready큐에 있는 프로세스 개수

// waiting 큐(io 발생해서 대기중인 프로세스들)
processPointer waitingQueue[MAX_PROCESS_NUM];
int waitingSize = 0;

// terminated 큐(실행 완료된 프로세스들)
processPointer terminatedQueue[MAX_PROCESS_NUM];
int terminatedSize = 0; // 현재 terminated큐에 있는 프로세스 개수

// 현재 실행 중인 프로세스
processPointer runningProcess = NULL;

int currentTime = 0; // 현재 시간
int cpuIdleTime = 0; // cpu idle 시간 합
int simulationEndTime = 0; // 마지막 실행 시점
int usedPID[MAX_PROCESS_NUM] = { 0 }; // pid 랜덤생성할 때 생성된 pid들은 저장(중복방지)
int ganttChart[MAX_TIME_UNIT]; // 간트 차트를 위한 기록용
int ioRequestedAt[MAX_TIME_UNIT]; // 해당 시간에 어떤 프로세스가 IO request했는지 기록

// 큐와 전역변수를 초기화하는 함수
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


// pid를 랜덤으로 생성하는 함수
int generatePID() {
	while (1) {
		int pid = rand() % 10 + 1; // 1~10 범위
		int duplicate = 0; // 중복체크(0이면 중복아님, 1이면 중복)
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
	newProcess->rrUsedTime = 0;

	newProcess->ioRequestTime = rand() % (newProcess->cpuBurstTime - 1) + 1; // IO 요청 시점은 1 이상, cpuBurstTime-1 이하 (cpu 작업 사이애 발생)
	newProcess->ioBurstTime = rand() % 5 + 2; // 범위 2~6으로 설정
	newProcess->ioRemainingTime = 0;
	newProcess->isInIO = FALSE;
	newProcess->hasRequestedIO = FALSE;

	return newProcess; // newProcess를 반환해서 job 큐에 넣음 
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


// job 큐에서 프로세스 빼기(현재 시각 기준 도착한 프로세스(index)) -> 빠진 프로세스는 ready 큐로 이동
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

// job 큐에 있는 프로세스들 출력(디버깅용)
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

// job 큐 arrival time 기준으로 정렬하는 함수(도착 시간 순서대로 ready 큐에 옮겨야 하니까)
void sortJobQueue() { // insertion sort 방식
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

// ready 큐에 프로세스 삽입(from job큐 or waiting 큐)
void insertReadyQueue(processPointer newProcess) {
	if (readySize < MAX_PROCESS_NUM) {
		readyQueue[readySize] = newProcess; //queue에 하나씩 프로세스 넣기
		readySize++;
	}
	else {
		printf("Ready Queue is already full\n");
	}
}


// ready 큐에서 프로세스 빼기(to runningProcess)
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


// ready 큐 상태 출력(디버깅용)
void printReadyQueue() {
	printf("\n--- Ready Queue ---\n");
	if (readySize == 0) printf("Ready Queue is empty\n");
	for (int i = 0; i < readySize; i++) {
		printf("PID: %d Arrival time: %d CPU burst time: %d Priority: %d\n",
			readyQueue[i]->pid, readyQueue[i]->arrivalTime,
			readyQueue[i]->cpuBurstTime, readyQueue[i]->priority); 
	}
}



// waiting 큐

// ready 큐에 프로세스 삽입(runningProcess에서 io request 했을 때)
void insertWaitingQueue(processPointer newProcess) {
	if (waitingSize < MAX_PROCESS_NUM) {
		waitingQueue[waitingSize] = newProcess;
		waitingSize++;
	}
}

// ready 큐에서 프로세스 빼기(to runningprocess)
processPointer deleteWaitingQueue(int index) {
	if (index >= waitingSize) return NULL;

	processPointer temp = waitingQueue[index];

	for (int i = index; i < waitingSize - 1; i++) {
		waitingQueue[i] = waitingQueue[i + 1];
	}
	waitingQueue[waitingSize - 1] = NULL; // 큐 마지막은 NULL로
	waitingSize--;

	return temp;
}

// waiting 큐 상태 출력(디버깅용)
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
		terminatedQueue[terminatedSize] = newProcess; 
		terminatedSize++;
	}
	else {
		printf("Terminated Queue is full!\n");
	}
}


//  terminated 큐 상태 출력(디버깅용)
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


// 간트 차트 출력함수
void printGanttChart(int endTime) {
	int previous = ganttChart[0]; // 직전 시간의 상태 저장(-1: idle, -2: io request, pid: 해당 프로세스가 cpu 사용중)
	int start = 0;

	printf("=============== Gantt Chart ===============\n");
	printf("| %2d ", start);

	for (int i = 1; i < endTime; i++) {
		if (ganttChart[i] != previous) { // 상태가 바뀌었을 때만 새롭게 출력
			// 구간 출력
			if (previous == -1) { // idle이라면
				printf("|--idle--|");
			}
			else if (previous == -2) {
				if (ioRequestedAt[start] != -1) // IO request 발생했다면
					printf("|--P%d(IO)--|", ioRequestedAt[start]);
				else
					printf("|--IO--|");  // 에러 방지
			}
			else { // 기존 프로세스와 다른 프로세스가 실행되었다면
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
			printf("|--IO--|");  // 에러 방지
	}
	else {
		printf("|--P%d--|", previous);
	}
	printf(" %2d |\n", endTime);
}


void simulate(int algorithm) {
	for (currentTime = 0; currentTime < MAX_TIME_UNIT; currentTime++) {

		// I/O 완료된 프로세스 있는지 확인(waiting 큐 -> ready 큐)
		for (int i = 0; i < waitingSize; i++) {
			waitingQueue[i]->ioRemainingTime--;

			if (waitingQueue[i]->ioRemainingTime <= 0) { // IO 남은 시간이 없다면(IO 완료라면)
				waitingQueue[i]->isInIO = FALSE;

				printf("%d: PID %d I/O complete (back to ready queue)\n",
					currentTime, waitingQueue[i]->pid);

				insertReadyQueue(deleteWaitingQueue(i)); // waiting 큐 -> ready 큐
				i--; //deleteWaitingQueue에서 한칸씩 밀어버린거 보정
			}
		}

		// 종료된 running 프로세스 처리(running -> terminated 큐)
		if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // CPU 남은 시간이 없다면(CPU 완료라면)
			insertTerminatedQueue(runningProcess);
			runningProcess = NULL;
		}

		// 매 시간마다 각 프로세스의 도착시간 확인하기(job 큐 -> ready 큐)
		for (int i = 0; i < jobSize; i++) {
			if (jobQueue[i]->arrivalTime == currentTime) {
				insertReadyQueue(deleteJobQueue(i)); // job 큐 -> ready 큐
				i--; //deleteJobQueue에서 한칸씩 밀어버린거 보정
			} 
		}

		// 전체 시뮬레이션 종료 조건 확인
		if (jobSize == 0 && readySize == 0 && waitingSize == 0 && runningProcess == NULL) { // 모든 큐가 비고 실행 중인 프로세스도 없으면
			simulationEndTime = currentTime; // 종료시간 기록
			break;
		}


		// 스케줄링 알고리즘 호출
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

		// 프로세스 실행
		if (selected == NULL) { // 아무것도 선택되지 않음 -> idle
			runningProcess = NULL;
			cpuIdleTime++;
			printf("%d: CPU is idle\n", currentTime);
		}
		else { // 선택되었다면 실행
			runningProcess = selected;

			// I/O 요청 조건: 이전에 IO 요청이 없었고 ioRequestTime 도달했을 때(runningProcess -> waiting큐)
			if (!runningProcess->hasRequestedIO &&
				runningProcess->cpuBurstTime - runningProcess->cpuRemainingTime == runningProcess->ioRequestTime) {

				ioRequestedAt[currentTime] = runningProcess->pid; // IO 요청 시점 기록
				runningProcess->isInIO = TRUE; // IO 처리중 
				runningProcess->hasRequestedIO = TRUE; // IO 요청 여부 기록
				runningProcess->ioRemainingTime = runningProcess->ioBurstTime; // ioRemainingTime 초기화

				printf("%d: PID %d is requesting I/O for %d units\n",
					currentTime, runningProcess->pid, runningProcess->ioBurstTime);

				insertWaitingQueue(runningProcess); 
				ganttChart[currentTime] = -2; // io request일때는 간트차트 상태 -2로 저장
				runningProcess = NULL; // io request 동안에는 다른 프로세스 처리불가
				cpuIdleTime++; // cpu가 idle
				continue;
			}

			// IO 요청이 아니라면(그냥 cpu에서 프로세스 실행)
			else {
				// CPU에서 정상 실행
				runningProcess->cpuRemainingTime--;
				runningProcess->turnaroundTime++;
				runningProcess->rrUsedTime++;

				printf("%d: PID %d is running (remaining: %d)\n",
					currentTime, runningProcess->pid, runningProcess->cpuRemainingTime);

				// cpu burst 남은 시간 없으면 (running -> terminated 큐)
				if (runningProcess->cpuRemainingTime == 0) {
					insertTerminatedQueue(runningProcess); // running -> terminated 큐
					runningProcess = NULL;
				}
			}
		}


		// ready 큐에 있는 프로세스들 대기/전체 시간 증가
		for (int i = 0; i < readySize; i++) {
			readyQueue[i]->waitingTime++;
			readyQueue[i]->turnaroundTime++;
		}

		// 간트차트 기록
		if (runningProcess != NULL) // 실행중인 프로세스가 있으면 pid
			ganttChart[currentTime] = runningProcess->pid;
		else if (ioRequestedAt[currentTime] != -1)
			ganttChart[currentTime] = -2;  // IO request라면 -2
		else
			ganttChart[currentTime] = -1;  // idle이면 -1

		simulationEndTime = currentTime; // 조기종료가 안되고 MAX_TIME_UNIT 까지 갈수도 있으니까 에러 방지
	}

}


// FCFS 알고리즘(평가항목 3번)
// ready 큐에 들어온 순서대로 실행
// non-preemptive 방식

processPointer ALG_FCFS() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) {
		return NULL; // 이미 실행 종료이므로 simulate()에서 idle로 처리
	}
	if (readySize == 0) {
		return runningProcess; // ready 큐가 비어있으면 그냥 실행중이던 프로세스 반환해서 simulate에서 실행
	}
	if (runningProcess == NULL) { // 실행 중인 프로세스가 없으면
		return deleteReadyQueue(0); // ready 큐 맨 앞 프로세스 반환해서 simulate에서 실행
	}
	return runningProcess; // 현재 실행중인 프로세스 그대로 반환
}


// Non-Preemptive SJF 알고리즘(평가항목 4번)
// 남은 cpu time 짧은 프로세스부터 실행
// 한번 프로세스가 시작되면 선점되지 않고 쭉 실행됨

processPointer ALG_NP_SJF() {
	if (runningProcess != NULL) { // 현재 실행중인 프로세스가 있는데
		if (runningProcess->cpuRemainingTime > 0) // 아직 cpu 시간 남았으면 그대로 실행중인 프로세스 반환해서 simulate에서 실행
			return runningProcess;
		else // cpu 시간 안남았으면
			return NULL; // 이미 실행 종료이므로 simulate()에서 idle로 처리
	}

	if (readySize == 0) { // 실행중인 프로세스도 없고 ready큐가 비었으면
		return NULL; // idle 처리
	}

	int min_index = 0; // cpuRemainingTime이 가장 적은 프로세스의 index 저장

	for (int i = 1; i < readySize; i++) {
		// 먼저 cpuRemainingTime 기준으로 비교
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i; 
		}
		// 같으면 도착 시간 기준으로 tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// cpuRemainingTime이 같다면 도착 시간이 빠른 프로세스가 먼저
			if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
				min_index = i;
			}
		}
	}
	// 선택한 프로세스를 ready 큐에서 삭제하고 반환(simulate()에서 ready큐 -> runningProcess)
	processPointer selected = readyQueue[min_index];
	deleteReadyQueue(min_index);
	return selected;
}


// Preemptive SJF 알고리즘(평가항목 5번)
// 남은 cpu time 짧은 프로세스부터 실행
// 매 시간 cpu time을 체크해서 더 적은 실행시간을 가진 프로세스가 있으면 선점

processPointer ALG_P_SJF() {
	if (readySize == 0) // ready 큐 비어있으면
		return (runningProcess != NULL) ? runningProcess : NULL; // 현재 실행중인 프로세스 반환

	int min_index = 0;  // cpuRemainingTime이 가장 적은 프로세스의 index 저장

	for (int i = 0; i < readySize; i++) {
		// 먼저 cpuRemainingTime 기준으로 비교
		if (readyQueue[i]->cpuRemainingTime < readyQueue[min_index]->cpuRemainingTime) {
			min_index = i;
		}
		// 같으면 도착 시간 기준으로 tie-break
		else if (readyQueue[i]->cpuRemainingTime == readyQueue[min_index]->cpuRemainingTime) {
			// cpuRemainingTime이 같다면 도착 시간이 빠른 프로세스가 먼저
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
		insertReadyQueue(runningProcess); // 현재 실행 중인 프로세스를 ready 큐에 보내고 (runningProcess -> ready 큐)
		return deleteReadyQueue(min_index); // 선점한 프로세스를 ready 큐에서 삭제 (simulate()에서 ready큐 -> runningProcess)
	}
	// 아니면 그냥 기존 프로세스 계속 실행
	return runningProcess;
}


// Non-Preemptive PRIORITY 알고리즘(평가항목 6번)
// 한번 프로세스가 시작하면 선점되지 않고 쭉 실행됨
// Priority가 높은 순서대로 실행

processPointer ALG_NP_PRIORITY() {
	if (readySize == 0) // ready 큐 비어있으면
		return (runningProcess != NULL) ? runningProcess : NULL;  // 현재 실행중인 프로세스 반환
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // 실행 중인 프로세스가 종료된 경우
		runningProcess = NULL;
	}
	if (runningProcess == NULL) { // 실행 중인 프로세스가 없다면
		int min_index = 0; // priority가 가장 높은 프로세스 찾기(priority 값이 작아야 priority가 높음)
		for (int i = 0; i < readySize; i++) {
			//  priority 가장 높은 프로세스 찾기
			if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
				min_index = i;
			}
			// 같으면 도착 시간 기준으로 tie-break
			else if (readyQueue[i]->priority == readyQueue[min_index]->priority) {
				if (readyQueue[i]->arrivalTime < readyQueue[min_index]->arrivalTime) {
					min_index = i;
				}
			}
		}
		return deleteReadyQueue(min_index); // ready 큐에서 삭제 (simulate()에서 ready큐 -> runningProcess)
	}
	// 실행중인 프로세스가 있으면 그대로 실행
	return runningProcess;
}


// Preemptive PRIORITY 알고리즘(평가항목 6번)
// Priority가 높은 순서대로 실행
// 매 시간 priority를 체크해서 더 높은 priority를 가진 프로세스가 있으면 선점

processPointer ALG_P_PRIORITY() {
	if (readySize == 0) // ready 큐 비어있으면
		return (runningProcess != NULL) ? runningProcess : NULL;  // 현재 실행중인 프로세스 반환
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // 실행 중인 프로세스가 종료된 경우
		runningProcess = NULL;
	}
	int min_index = 0; // priority가 가장 높은 프로세스 찾기(priority 값이 작아야 priority가 높음)
	for (int i = 0; i < readySize; i++) {
		//  priority 가장 높은 프로세스 찾기
		if (readyQueue[i]->priority < readyQueue[min_index]->priority) {
			min_index = i;
		}
		// 같으면 도착 시간 기준으로 tie-break
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
		insertReadyQueue(runningProcess); // 현재 실행 중인 프로세스를 ready 큐에 보내고 (runningProcess -> ready 큐)
		return deleteReadyQueue(min_index); // 선점한 프로세스를 ready 큐에서 삭제 (simulate()에서 ready큐 -> runningProcess)
	}
	// 아니면 그냥 기존 프로세스 계속 실행
	return runningProcess;
}


// Round-Robbin 알고리즘(평가항목 7번)
// 각 프로세스가 time quantum만큼 실행되고 그 다음 프로세스로 넘어감
// 프로세스가 끝나지 않으면 ready 큐 맨 뒤로

processPointer ALG_RR() {
	if (runningProcess != NULL && runningProcess->cpuRemainingTime <= 0) { // 실행 중인 프로세스가 끝났으면
		runningProcess = NULL; // NULL 반환
	}

	if (readySize == 0) { // ready 큐 비어있으면
		return (runningProcess != NULL) ? runningProcess : NULL; // 현재 실행중인 프로세스 반환
	}
	// 실행중인 프로세스가 없는 경우
	if (runningProcess == NULL) {
		processPointer nextProcessor = deleteReadyQueue(0); // ready 큐 맨앞에서 가져오기
		nextProcessor->rrUsedTime = 0; // 새로 실행할 프로세스 사용 시간 초기화
		return nextProcessor;
	}
	// 실행 중인 프로세스가 타임 퀀텀만큼 실행된 경우
	if (runningProcess->rrUsedTime >= TIME_QUANTUM) {
		insertReadyQueue(runningProcess); // 다시 ready 큐로(running -> ready 큐)
		processPointer nextProcessor = deleteReadyQueue(0); // 다음 프로세스(ready 큐 맨앞) 실행
		nextProcessor->rrUsedTime = 0; // 새로 시작하므로 사용시간 초기화
		return nextProcessor;
	}
	// 아직 타임퀀텀 끝나기 전이면
	return runningProcess;
}

// IO-Aware SJF 알고리즘(추가함수, 평가항목 11번)
// non-preemptive 방식
// SJF와 달리 IO 요청 전 프로세스는 IO burst time까지 더해서 실행시간 예측
// 즉, cpu burst time (+ io burst time)이 짧은 프로세스 우선 실행

processPointer ALG_IASJF() {
	// 실행 중인 프로세스가 있으면 반환
	if (runningProcess != NULL && runningProcess->cpuRemainingTime > 0)
		return runningProcess;

	if (readySize == 0) // ready 큐가 비었으면 NULL
		return NULL;

	int bestIndex = 0; // 가장 짧은 effective time(cpu burst time + io burst time)을 가진 프로세스의 index
	int bestEffective = readyQueue[0]->cpuRemainingTime +
		(readyQueue[0]->hasRequestedIO ? 0 : readyQueue[0]->ioBurstTime); // IO 요청 전 프로세스는 IO burst time까지 더해서

	for (int i = 1; i < readySize; i++) {
		int effective = readyQueue[i]->cpuRemainingTime +
			(readyQueue[i]->hasRequestedIO ? 0 : readyQueue[i]->ioBurstTime);

		if (effective < bestEffective) { // effective time이 짧은 프로세스 찾기
			bestEffective = effective;
			bestIndex = i;
		}
		else if (effective == bestEffective) { // 도착 시간이 빠른 순으로 tie-break
			if (readyQueue[i]->arrivalTime < readyQueue[bestIndex]->arrivalTime) {
				bestIndex = i;
			}
		}
	}

	return deleteReadyQueue(bestIndex); // effective time이 짧은 프로세스를 ready 큐에서 삭제 (simulate()에서 ready큐 -> runningProcess)
}

// 평가(waiting time, turnaround time) 평가항목 10번
void evaluate() {
	int totalWaitingTime = 0; // ready 큐에 있었던 시간의 총합
	int totalTurnaroundTime = 0; // 프로세스가 도착해서 종료할때까지 걸린 총 시간

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
	srand(time(NULL)); // 실행할 때마다 다른 rand값 생성하게
	int numProcesses = 5; // 알고리즘 비교를 위해 프로세스 개수 5개로 제한(변경 가능 -> but, priority 범위도 같이 변경해야 함)

	// 기준 프로세스 집합 생성(기준이 있어야 모든 알고리즘을 같은 프로세스 집합으로 평가 가능)
	processPointer baseProcesses[MAX_PROCESS_NUM];
	int baseSize = 0;
	for (int i = 0; i < numProcesses; i++) {
		baseProcesses[baseSize++] = createProcess();
	}

	// 모든 알고리즘 시뮬레이션 및 평가
	for (int alg = FCFS; alg <= IASJF; alg++) {
		printf("\n==================================================================================\n");
		printf("%s\n", algorithmNames[alg]); // 알고리즘 이름 출력(아까 정의해둔 const char* algorithmNames[]에서 참조)

		// 큐 및 전역변수 초기화
		initQueues();

		// 기준 프로세스 복제 -> jobQueue에 삽입
		for (int i = 0; i < baseSize; i++) {
			processPointer original = baseProcesses[i];
			processPointer clone = (processPointer)malloc(sizeof(Process));

			// original(기준프로세스)의 모든 정보 clone에 복사
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

			insertJobQueue(clone); // job 큐에 삽입
		}

		// job 큐 정렬(arrival time 기준으로)
		sortJobQueue();

		// job큐 출력(디버깅용)
		printJobQueue();

		// 시뮬레이션 실행(알고리즘 번호에 따라 호출)
		simulate(alg);

		// 간트차트 출력
		printGanttChart(simulationEndTime);

		// 평가(평균 waiting/turnaround time)
		evaluate();

		// 디버깅용 terminated 큐 출력
		//printTerminatedQueue();

		// terminated 큐 메모리 해제
		for (int i = 0; i < terminatedSize; i++) {
			free(terminatedQueue[i]);
		}
	}

	// 기준 프로세스 메모리 해제
	for (int i = 0; i < baseSize; i++) {
		free(baseProcesses[i]);
	}
}