🌐 Network Module (network.h, network.c) : Async IPC & Multi-threading Pipeline

1. Module Overview (모듈 개요)
network 모듈은 TermStock 프로젝트의 데이터 공급 파이프라인(Data Pipeline)입니다. 바이낸스(Binance) API의 실시간 시세 데이터와 글로벌 암호화폐 뉴스(RSS 피드)를 외부 서버로부터 수집하여 전역 상태 저장소(data.c)에 전달합니다.

가장 큰 기술적 성과는 네트워크 지연(Network Latency)으로 인해 터미널 UI가 멈추는(Freezing) 현상을 완벽하게 방지하기 위해 비동기 멀티스레딩(Asynchronous Multi-threading)을 도입하고, 무거운 파싱 라이브러리 대신 OS 레벨의 프로세스 간 통신(IPC)을 설계했다는 점입니다.

2. Core System Programming Concepts (핵심 시스템 프로그래밍 기법)
본 모듈은 C언어의 한계를 극복하기 위해 리눅스/UNIX의 핵심 철학과 시스템 콜(System Call)을 적극적으로 활용했습니다.

Inter-Process Communication (IPC 및 파이프라인):
무거운 HTTP/JSON 통신 라이브러리(libcurl, cJSON 등)를 정적으로 링크하는 대신, 시스템 콜을 직접 호출했습니다. pipe()로 통신 통로를 열고, fork()로 자식 프로세스를 생성한 뒤, dup2()를 통해 표준 출력(STDOUT)을 파이프로 리다이렉션하여 부모 프로세스가 데이터를 read()로 읽어오는 완벽한 IPC 파이프라인을 구축했습니다.

Thread-Safe Asynchrony (스레드 안전성 및 비동기 제어):
주가 갱신과 뉴스 갱신을 메인 UI와 완전히 분리된 POSIX Threads(pthread) 백그라운드 워커로 동작시킵니다. 두 스레드가 전역 배열(watchlist)을 갱신할 때 메인 스레드와 충돌(Race Condition)하지 않도록 pthread_mutex_t를 통한 정교한 락(Lock) 메커니즘을 적용했습니다.

Process Lifecycle Management (좀비 프로세스 방지):
execlp()를 호출하여 자식 프로세스를 쉘 명령어로 덮어씌우고, 부모 프로세스는 waitpid()를 호출하여 자식 프로세스가 안전하게 종료될 때까지 대기합니다. 메모리 누수와 좀비 프로세스 생성을 원천 차단했습니다.

3. Function Details (주요 메서드 및 시스템 콜 분석)
📡 Data Fetching (fetch_price, fetch_news)
핵심 시스템 콜 Flow: pipe() ➔ fork() ➔ close() ➔ dup2() ➔ execlp() ➔ read() ➔ waitpid()

동적 URL 포맷팅 (fetch_price): 고정된 API가 아닌, data 모듈의 current_interval 변수를 sprintf로 주입받아 URL을 생성합니다. 유저가 분봉/일봉(1m, 15m, 1h, 1d)을 변경하는 즉시 동적으로 API를 재호출합니다.

쉘 파이프라인 파싱 (fetch_news): RSS 피드를 가져온 뒤, curl | grep | tail -n +2 | head -n 3 | awk로 이어지는 리눅스 쉘 파이프라인 명령어를 C언어 내부로 끌어와, 불필요한 메타데이터를 쳐내고 순수 기사 제목 3줄만 초고속으로 추출해 냅니다.

🧵 Async Worker Threads (fetch_worker, news_worker)
fetch_worker: 무한 루프 내에서 sleep(5)를 통해 5초 주기로 모든 관심 종목의 시세 데이터를 폴링(Polling)합니다.

news_worker: API 호출 낭비를 막기 위해 타임 타이머가 아닌 '사용자가 종목 커서(selected_idx)를 변경했을 때'만 조건부로 뉴스를 비동기 갱신하는 최적화된 이벤트 리스너(Event Listener) 형태로 동작합니다.

✂️ Memory-Efficient Parsing (parse_klines)
바이낸스 API가 반환하는 대용량 JSON 데이터(다차원 배열)를 무거운 파서 없이 C 표준 라이브러리인 strchr, atof의 포인터 연산만으로 스캔하여, 캔들 렌더링에 필요한 시가(Open)와 종가(Close)만 float 배열로 초고속 추출합니다.

4. Technical Highlights for Presentation (발표 핵심 포인트)
"이 모듈을 개발할 때 가장 집중한 두 가지는 **'메인 UI 스레드의 절대적인 방어'**와 **'OS 자원의 극대화'**입니다. 만약 와이파이가 끊기거나 서버 응답이 10초 이상 지연되더라도, 유저는 여전히 터미널에서 부드럽게 방향키를 움직이고 차트 타임프레임을 전환할 수 있습니다.
또한, C언어로 JSON 통신을 구현하기 위해 무거운 라이브러리를 쓰는 대신, 운영체제 수업에서 배운 fork, pipe, dup2 시스템 콜을 응용하여 자식 프로세스에게 curl 명령을 위임하는 독자적인 IPC 아키텍처를 설계했습니다. 이는 제 시스템 프로그래밍 역량과 트러블슈팅 능력을 가장 명확하게 보여주는 핵심 파이프라인입니다."