🚀 Main Module (main.c) : Control Tower & Event Loop

1. Module Overview (모듈 개요)
main 모듈은 TermStock 프로젝트의 진입점(Entry Point)이자, 전체 시스템의 생명주기(Lifecycle)를 관장하는 컨트롤 타워(Control Tower)입니다.

초기화(Initialization)부터 백그라운드 워커 스레드(network)와 렌더링 엔진(ui, chart) 간의 오케스트레이션(Orchestration)을 담당합니다. 특히 입력 대기에 빠지지 않는 논블로킹 이벤트 루프(Non-blocking Event Loop)를 구축하여, 멀티스레딩 환경에서도 충돌 없이 실시간으로 데이터가 갱신되고 화면이 반응하도록 설계되었습니다.

2. Core System Programming Concepts (핵심 시스템 프로그래밍 기법)
본 모듈은 터미널 환경의 제약을 극복하고, 안정적인 프로세스 구동 및 종료를 위해 운영체제(OS) 수준의 자원 관리 기법을 적극적으로 적용했습니다.

Flicker-Free Rendering & Responsive UI (더블 버퍼링 및 반응형 렌더링):
매 프레임 clear()를 호출하면 발생하는 심각한 깜빡임(Flickering)을 잡기 위해, getmaxyx()로 터미널 창의 리사이즈 여부를 실시간으로 감지합니다. 창 크기가 변했을 때만 clear()를, 평상시에는 가상 메모리만 지우는 erase()를 호출하는 영리한 더블 버퍼링(Double Buffering) 구조를 구현했습니다. 동시에 터미널 창을 조절해도 차트가 빈 공간을 100% 활용하도록 수학적 비율을 동적 할당(Dynamic Scaling)합니다.

Signal Handling & Graceful Shutdown (안전한 시스템 종료):
signal(SIGINT, handle_sigint)를 등록하여 사용자가 강제 종료(Ctrl+C)를 시도하더라도 프로그램이 즉시 튕기지(Kill) 않도록 방어합니다. 전역 플래그(is_running = 0)를 통해 메인 루프와 자식 스레드들이 진행 중이던 작업을 안전하게 마무리하고 스스로 메모리를 반환하도록 유도합니다.

Zombie Process Reaping (좀비 프로세스 원천 차단):
네트워크 모듈에서 비동기 파싱을 위해 무수히 생성했던 자식 프로세스(curl 등)들이 루프 종료 후 남겨질 수 있습니다. 메인 루프 탈출 후 while(waitpid(-1, NULL, WNOHANG) > 0); 구문을 삽입하여, 종료를 기다리는 찌꺼기 자식 프로세스들을 수거해 OS의 프로세스 테이블을 깔끔하게 비웁니다.

3. Lifecycle & Main Event Loop (주요 생명주기 및 루프 분석)
프로그램의 메인 라이프사이클은 다음 순서로 동작합니다.

⚙️ Step 1. System Initialization (초기화)
로컬 디스크에서 watchlist 데이터를 로드합니다 (load_watchlist).

ncurses 라이브러리 초기화 및 특수문자/색상 렌더링 환경(setlocale, init_pair)을 세팅합니다.

pthread_create를 통해 fetch_worker와 news_worker 스레드를 백그라운드에 띄웁니다.

🔄 Step 2. Non-blocking Event Loop (메인 렌더링 루프)
while(is_running) 루프 안에서 초당 수십 번씩 렌더링과 이벤트 처리를 반복합니다.

getch()에 timeout(100)을 설정하여 입력을 기다리며 프로그램이 멈추는(Blocking) 현상을 억제합니다.

Event Routing (이벤트 라우팅):

[1~4]: 타임프레임 동적 전환. 유저가 숫자키를 누르는 즉시 상태 변수(current_interval)를 변경하고, 차트를 초기화하여 1분봉부터 1일봉까지 즉각적으로 넘나드는 트레이딩 분석 툴의 역할을 수행합니다.

[UP] / [DOWN]: selected_idx를 변경하여 관심 종목을 탐색합니다.

[a] / [d]: 터미널 입력 모드로 전환하여 종목을 추가(Add)하거나 삭제(Delete)하며, 삭제 후 인덱스 초과 예방을 위한 자동 커서 보정 로직이 동작합니다.

🧹 Step 3. Cleanup (자원 회수 및 종료)
루프를 빠져나오면 시스템 자원을 완벽하게 회수합니다.

pthread_join으로 백그라운드 스레드들의 종료를 대기하고, pthread_mutex_destroy로 자물쇠 객체를 OS에 반환합니다.

endwin()을 호출하여 터미널을 원래의 쉘(Shell) 상태로 복구합니다.
