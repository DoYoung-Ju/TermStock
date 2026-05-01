# 🌐 Network Module (`network.h`, `network.c`) : Async Data Pipeline

**Author**: 2024017497 주도영

## 1. Overview (개요)
`network` 모듈은 TermStock 프로젝트의 **데이터 공급 파이프라인(Data Pipeline)**을 담당합니다. 바이낸스(Binance) API의 실시간 시세 데이터와 글로벌 암호화폐 뉴스(RSS 피드)를 외부 서버로부터 수집하여 내부 데이터베이스(`data.c`)에 안전하게 전달합니다.

가장 큰 기술적 특징은 네트워크 지연(Network Latency)으로 인해 터미널 UI가 멈추는 현상을 방지하기 위해 **100% 비동기 멀티스레딩(Asynchronous Multi-threading)**으로 설계되었다는 점입니다.

## 2. Core Concepts (핵심 설계 철학)

* **UNIX Philosophy (리눅스 파이프라인 활용):**
  C언어 환경에서 무거운 HTTP/JSON 파싱 라이브러리(예: `libcurl`, `cJSON`)를 정적으로 링크하면 바이너리가 무거워지고 의존성이 복잡해집니다. 이를 피하기 위해 OS 내장 명령어인 `curl`, `grep`, `awk`, `tail` 등을 `popen()` 또는 프로세스 파이프라인(`|`)으로 조합하여 가볍고 빠른 파싱 환경을 구축했습니다.
* **Thread-Safe Data Handling (스레드 안전성):**
  백그라운드 네트워크 스레드가 데이터를 갱신하는 순간 메인 UI 스레드가 데이터를 읽으면 Race Condition(경쟁 상태)이 발생해 프로그램이 튕길 수 있습니다. 이를 방지하기 위해 `pthread_mutex_t`를 통한 정교한 락(Lock) 메커니즘을 적용했습니다.

## 3. Function Details (주요 메서드 분석)

### `void* fetch_worker(void* arg)`
* **역할:** 주가 데이터를 주기적으로 긁어오는 백그라운드 워커(Worker) 스레드입니다.
* **동작 원리:** 무한 루프(`while(is_running)`)를 돌며 설정된 간격(예: 5초)마다 선택된 종목의 바이낸스 API(`interval=1h`)를 호출합니다.

### `void parse_klines(const char* json_data, float* open_data, float* close_data)`
* **역할:** 바이낸스가 응답한 OHLC(시가/고가/저가/종가) 배열 데이터에서 시가와 종가만 정밀하게 추출합니다.
* **특징:** C언어의 문자열 토큰화 로직(`strtok` 또는 인덱스 스캔)을 최적화하여 메모리 오버헤드 없이 `float` 배열로 스케일링 데이터를 넘겨줍니다.

### `void* news_worker(void* arg)`
* **역할:** 암호화폐 관련 최신 뉴스 헤드라인을 가져오는 독립 스레드입니다.
* **특징:** API 호출 횟수를 낭비하지 않기 위해, 타임 타이머 기반이 아닌 '사용자가 종목을 변경했을 때' 등 특정 이벤트가 발생할 때만 비동기적으로 동작하도록 상태 플래그를 제어합니다.

### `void fetch_news()`
* **역할:** RSS 피드(Cointelegraph 등)를 JSON으로 변환해 주는 API를 호출하고, 핵심 헤드라인 3줄을 파싱합니다.
* **버그 픽스 및 최적화 로직:**
  * `tail -n +2` 명령어를 쉘 스크립트 파이프에 삽입하여, API가 반환하는 첫 번째 무의미한 메타데이터(채널명)를 필터링(Skip)하고, `head -n 3`로 정확히 3개의 알짜 기사만 렌더링되도록 튜닝했습니다.

## 4. Technical Highlights for Presentation (발표 핵심 포인트)
> "이 모듈을 개발할 때 가장 집중한 것은 **'메인 UI 스레드의 절대적인 방어'**입니다. 만약 와이파이가 끊기거나 서버 응답이 10초 이상 지연되더라도, 유저는 여전히 터미널에서 부드럽게 방향키를 움직이고 종목을 삭제할 수 있습니다. 네트워크 작업과 화면 렌더링(UI) 작업을 POSIX Thread(`pthread`)로 완벽하게 격리시켰기 때문입니다. 또한, 리눅스 시스템 프로그래밍의 강력함을 살려 파이프라인 명령어를 C언어 내부로 끌어와 파서(Parser)를 직접 구현한 것이 제 개발 역량을 가장 잘 보여주는 부분이라고 생각합니다."