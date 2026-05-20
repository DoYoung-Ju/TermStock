🗄️ Data Module (data.h, data.c) : State Management & Persistence

1. Module Overview (모듈 개요)
data 모듈은 TermStock 프로젝트의 중앙 데이터 저장소(Central Repository)이자 상태 관리(State Management)를 담당하는 코어 모듈입니다.

단일 진실 공급원(Single Source of Truth) 패턴을 적용하여, 프로젝트 전반에서 사용되는 관심 종목(Watchlist), 실시간 가격, 뉴스 헤드라인, 현재 타임프레임(current_interval) 등의 상태를 전역으로 관리합니다. 백그라운드 워커 스레드와 메인 UI 렌더링 스레드가 동일한 메모리 공간에 동시 접근할 때 발생하는 치명적인 충돌(Race Condition)을 방지하기 위해 완벽한 스레드 안전성(Thread-Safety)을 보장하도록 설계되었습니다.

2. Core Architecture & OS-Level Concepts (핵심 설계 및 시스템 프로그래밍 기법)
본 모듈은 일반적인 C언어 수준을 넘어, 운영체제(OS)의 시스템 콜과 메모리 동기화 기법을 적극적으로 활용했습니다.

Thread Synchronization (멀티스레드 동기화):
데이터 읽기/쓰기 충돌을 막기 위해 POSIX Threads의 상호 배제 잠금 장치인 pthread_mutex_t를 3개(data_mutex, news_mutex, log_mutex)로 세분화하여 도입했습니다. 병목 현상(Bottleneck)을 최소화하면서도 각 데이터의 임계 구역(Critical Section)을 안전하게 보호합니다.

Low-Level POSIX I/O (저수준 파일 입출력):
C 표준 라이브러리의 버퍼링된 I/O(fopen, fprintf) 대신, 운영체제의 파일 디스크립터(File Descriptor)를 직접 다루는 저수준 시스템 콜(open, read, write)을 사용하여 데이터 영속성과 로깅 로직을 구현했습니다.

Signal Safety (안전한 인터럽트 제어):
Ctrl+C와 같은 비동기 시그널을 안전하게 처리하기 위해 메인 루프 제어 변수에 volatile sig_atomic_t is_running을 적용했습니다. 컴파일러의 임의 최적화를 방지하고 인터럽트 발생 시 메모리 릭(Leak) 없는 안전한 종료(Graceful Shutdown)를 보장합니다.

3. Core Structures & Global States (핵심 구조체 및 상태 변수)
모든 전역 변수는 data.h에서 extern으로 선언되어 모듈 간 공유되며, 메모리 할당 및 초기화는 data.c에서 이루어집니다.

struct WatchlistItem & watchlist[MAX_SYMBOLS]

개별 종목의 상태를 담는 컨테이너 구조체 배열입니다. 심볼명(symbol), 실시간 현재가(price), 캔들 차트 렌더링을 위한 시가/종가 데이터(open_data, close_data)를 저장합니다.

char current_interval[10] (Timeframe State)

사용자가 현재 보고 있는 차트의 타임프레임(예: 1m, 15m, 1h, 1d)을 저장합니다. 메인 스레드에서 사용자의 키보드 입력에 따라 실시간으로 값이 변경되며, 네트워크 스레드는 이 값을 참조하여 동적으로 API 호출 URL을 조립합니다.

4. Lifecycle & Core Methods (주요 메서드 분석)
💾 Data Persistence (데이터 영속성)
load_watchlist()

프로그램 시작 시 로컬 디스크의 watchlist.txt 파일을 읽어 메모리에 적재합니다.

파일이 없거나 손상되었을 경우를 대비해 BTCUSDT를 기본값으로 자동 세팅하는 Fallback 로직이 적용되어 있습니다. strtok을 활용해 개행 문자를 파싱합니다.

save_watchlist()

유저가 종목을 추가(a)하거나 삭제(d)할 때 호출되며, 현재 배열 상태를 텍스트 파일로 직렬화(Serialization)하여 덮어씁니다. 데이터 유실을 실시간으로 방지합니다.

📝 Thread-Safe Logging (로깅 시스템)
write_log(const char* format, ...)

다중 스레드 환경에서 디버깅과 시스템 추적을 돕는 스레드 안전(Thread-safe) 커스텀 로거입니다.

C 언어의 가변 인자 매크로(va_list, va_start, va_end)와 vsnprintf를 결합하여 printf와 동일한 동적 포맷팅을 지원합니다.

log_mutex로 잠금을 걸어, 여러 스레드가 동시에 로그를 작성하더라도 파일 내에서 메시지가 섞이거나 깨지지 않도록 동기화했습니다.

5. Technical Highlights for Presentation (발표 핵심 포인트)
"데이터 모듈 설계 시 가장 까다로웠던 부분은 **'멀티스레딩 환경에서의 안전성 보장'**이었습니다. 네트워크 스레드가 배열에 새로운 차트 데이터를 쓰는 도중에 렌더링 스레드가 이를 읽으려 하면 Segmentation Fault가 발생합니다. 이를 해결하기 위해 운영체제 수업에서 배운 Mutex를 실제 코드에 적용하여 임계 구역을 보호했습니다. 더불어 가변 인자(va_list)를 활용한 커스텀 로거를 저수준 시스템 콜(write)로 구현하여, 어떤 상황에서도 절대 프로그램이 뻗지 않는 견고한 데이터 레이어를 완성했습니다."