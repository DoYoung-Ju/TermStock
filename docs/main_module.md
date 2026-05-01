# 🚀 Main Module (`main.c`) : Control Tower & Event Loop

**Author**: 2024017497 주도영

## 1. Overview (개요)
`main` 모듈은 TermStock 프로젝트의 **진입점(Entry Point)**이자, 전체 시스템의 흐름을 관장하는 **컨트롤 타워(Control Tower)**입니다. 

초기화(Initialization), 스레드 생성(Thread Creation), 유저 입력 처리(Input Handling), 화면 렌더링(Rendering)의 전체 생명주기(Lifecycle)를 관리하며, 백그라운드 스레드들과 메인 UI 스레드가 충돌 없이 조화롭게 동작하도록 오케스트레이션(Orchestration)합니다.

## 2. Core Concepts (핵심 설계 철학)

* **Event-Driven Architecture (이벤트 기반 아키텍처):**
  `ncurses`의 `nodelay(stdscr, TRUE)` 옵션을 사용하여 입력 대기(Blocking) 상태에 빠지지 않는 논블로킹(Non-blocking) I/O 환경을 구축했습니다. 덕분에 키보드 입력이 없는 순간에도 차트와 뉴스는 실시간으로 부드럽게 갱신됩니다.
* **Flicker-Free Rendering (더블 버퍼링 로직):**
  화면을 갱신할 때마다 `clear()`를 호출하면 화면 전체가 지워졌다가 다시 그려지면서 심각한 깜빡임(Flickering)이 발생합니다. 이를 해결하기 위해 터미널 사이즈(`getmaxyx`) 변경 시에만 `clear()`를 호출하고, 평상시에는 가상 메모리만 지우는 `erase()` 함수를 호출하는 **더블 버퍼링(Double Buffering)** 기법을 구현했습니다.
* **Graceful Shutdown (안전한 시스템 종료):**
  사용자가 `q`를 누르거나 강제 종료 인터럽트(`Ctrl+C`)를 발생시켰을 때, 프로그램이 즉시 튕기지 않고 `is_running` 플래그를 통해 모든 루프를 안전하게 탈출합니다. 이후 `pthread_join`으로 자식 스레드들이 작업을 마칠 때까지 기다려주고 메모리를 해제하는 우아한 종료(Graceful Shutdown)를 보장합니다.

## 3. Function Details (주요 로직 분석)

### `void handle_sigint(int sig)`
* **역할:** 운영체제로부터 `SIGINT (Ctrl+C)` 신호를 받았을 때 호출되는 콜백 함수입니다.
* **특징:** 전역 플래그인 `is_running`을 `0`으로 변경하여, 메인 루프와 워커 스레드들이 하던 작업을 안전하게 마무리하고 스스로 종료되도록 유도합니다.

### `int main()`
프로그램의 메인 라이프사이클을 담당하며 아래의 순서로 동작합니다.

1. **System Initialization (초기화):**
   * 로컬 디스크에서 `watchlist` 데이터를 로드합니다 (`load_watchlist`).
   * `ncurses` 라이브러리를 초기화하고, 특수문자 출력(`setlocale`)과 색상 쌍(`init_pair`)을 설정합니다.
   * `data_mutex`와 `news_mutex`를 초기화하고, `pthread_create`를 통해 네트워크 스레드들을 백그라운드에 띄웁니다.

2. **Main Event Loop (메인 렌더링 루프):**
   * `while(is_running)` 루프 안에서 1초에 수십 번씩 터미널 화면을 다시 그립니다.
   * `ui_module`과 `chart_module`의 함수를 호출하여 화면의 레이아웃과 점자/캔들 차트를 렌더링합니다.
   * `getch()`로 유저의 키보드 입력을 감지합니다.
     * `[UP] / [DOWN]`: `selected_idx`를 변경하여 관심 종목 탐색.
     * `[a]`: 터미널 입력 모드(`echo`)로 전환하여 새로운 종목 심볼을 입력받아 배열에 추가.
     * `[d]`: 현재 선택된 종목 삭제. **(삭제 후 인덱스 초과 예방을 위한 자동 커서 보정 로직 포함)**
     * `[q]`: 루프 탈출 및 프로그램 종료.

3. **Cleanup (메모리 해제 및 종료):**
   * 루프를 빠져나오면 `pthread_join`을 호출해 백그라운드 스레드들을 안전하게 회수합니다.
   * `endwin()`을 호출하여 터미널을 원래의 쉘(Shell) 상태로 깔끔하게 복구합니다.

## 4. Technical Highlights for Presentation (발표 핵심 포인트)
> "제가 터미널 프로그램을 만들면서 가장 중요하게 생각한 것은 **'사용자 경험(UX)'**입니다. `main` 함수에 작성된 메인 루프를 보시면, `getch()`가 입력을 기다리며 프로그램을 멈추게 하는(Blocking) 기본 동작을 억제하고 논블로킹(Non-blocking)으로 전환했습니다. 또한 `erase()`를 통한 더블 버퍼링으로 깜빡임을 잡았습니다. 결과적으로 C 언어와 터미널 환경이라는 투박한 제약 속에서도, 60fps로 동작하는 웹 애플리케이션 못지않은 극강의 부드러움과 반응성을 이끌어 냈습니다."