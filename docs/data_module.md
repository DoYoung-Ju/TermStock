# 🗄️ Data Module (`data.h`, `data.c`) : State Management & Persistence

**Author**: 2024017497 주도영

## 1. Overview (개요)
`data` 모듈은 TermStock 프로젝트의 **상태 관리(State Management)**와 **데이터 영속성(Persistence)**을 책임지는 중앙 데이터 저장소입니다. 

애플리케이션 전반에서 사용되는 관심 종목(Watchlist) 리스트와 실시간 가격/차트 데이터를 구조체 배열로 관리합니다. 특히, 백그라운드 스레드(데이터 갱신)와 메인 UI 스레드(화면 렌더링 및 유저 입력)가 동일한 메모리 공간에 동시 접근할 때 발생하는 치명적인 충돌을 막기 위해 **스레드 안전성(Thread-Safety)**을 최우선으로 설계했습니다.

## 2. Core Concepts (핵심 설계 철학)

* **Single Source of Truth (단일 진실 공급원):**
  프로그램 내의 모든 모듈(`main`, `ui`, `network`)은 데이터를 직접 소유하지 않고, 오직 전역으로 관리되는 `watchlist` 배열과 상태 변수들을 통해서만 데이터를 읽고 씁니다. 이를 통해 데이터의 불일치(Inconsistency)를 방지합니다.
* **Thread Synchronization (스레드 동기화):**
  POSIX Threads(`pthread`)의 상호 배제 잠금 장치인 `pthread_mutex_t`를 도입했습니다. 네트워크 모듈이 데이터를 덮어쓰는 도중에는 UI 모듈이 화면을 그리지 못하도록 Lock을 걸어 **경쟁 상태(Race Condition)**와 **세그멘테이션 폴트(Segmentation Fault)**를 원천 차단합니다.
* **Data Persistence (데이터 영속성):**
  사용자가 추가/삭제한 종목 리스트를 메모리에만 휘발성으로 두지 않고, 로컬 디스크의 텍스트 파일(`watchlist.txt`)로 직렬화(Serialization)하여 저장합니다. 이를 통해 프로그램 재시작 시에도 이전 상태를 완벽하게 복구합니다.

## 3. Core Structures & Variables (핵심 구조체 및 변수)

### `struct WatchlistItem`
* **역할:** 개별 종목의 모든 상태를 담고 있는 데이터 컨테이너(Data Container)입니다.
* **멤버 변수:**
  * `char symbol[20]`: 코인/주식 티커 심볼 (예: "BTCUSDT")
  * `char price[20]`: 실시간 현재가 문자열
  * `float open_data[CHART_POINTS]`: 캔들 차트 렌더링을 위한 시가(Open) 배열
  * `float close_data[CHART_POINTS]`: 캔들 차트 렌더링을 위한 종가(Close) 배열

### `pthread_mutex_t data_mutex` / `news_mutex`
* **역할:** 가격 데이터와 뉴스 데이터에 대한 동시 접근을 통제하는 자물쇠(Lock) 객체입니다.

## 4. Function Details (주요 메서드 분석)

### `void init_data()`
* **역할:** 프로그램 시작 시 뮤텍스(Mutex)를 초기화하고 필요한 메모리 공간을 준비합니다. C++의 전역 객체 생성자(Constructor) 역할을 수행합니다.

### `void load_watchlist()`
* **역할:** 로컬 파일(`watchlist.txt`)을 읽어들여 메모리(`watchlist` 배열)에 로드합니다.
* **특징:** 파일이 존재하지 않거나 손상되었을 경우를 대비한 예외 처리(Error Handling) 로직이 포함되어 있으며, 파일 I/O 실패 시 프로그램이 종료되지 않고 빈 리스트로 안전하게 시작되도록 설계했습니다.

### `void save_watchlist()`
* **역할:** 메모리에 있는 현재 종목 리스트를 로컬 파일에 덮어씁니다.
* **동작 시점:** 사용자가 메인 화면에서 키보드 이벤트(`a` 추가, `d` 삭제)를 발생시켜 상태가 변경될 때마다 즉시 호출되어 데이터 유실을 방지합니다.

## 5. Technical Highlights for Presentation (발표 핵심 포인트)
> "데이터 모듈 설계 시 가장 까다로웠던 부분은 **'멀티스레딩 환경에서의 안전성 보장'**이었습니다. 네트워크 스레드가 배열에 새로운 차트 데이터를 쓰고 있는데, 동시에 렌더링 스레드가 그 배열을 읽어 차트를 그리려고 하면 메모리가 꼬이면서 프로그램이 강제 종료됩니다. 이를 해결하기 위해 운영체제 수업에서 배운 `Mutex` 개념을 실제 코드에 적용하여 임계 구역(Critical Section)을 보호했습니다. 결과적으로 아무리 빠른 속도로 키보드를 연타하거나 네트워크가 폭주해도 **절대 죽지 않는 견고한 데이터 레이어**를 완성했습니다."