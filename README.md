# 📈 TermStock : Terminal-based Real-time Crypto Chart Tracker

< 시스템 프로그래밍(ELEC462) 기말 프로젝트 >
무거운 HTS나 웹 브라우저 없이, 오직 OS 레벨 자원과 터미널 환경만으로 동작하는 초경량 실시간 암호화폐 차트 트래커입니다.

🚀 Project Overview

`TermStock`은 C언어 기반의 터미널 애플리케이션으로, 외부의 무거운 JSON 파싱 라이브러리나 GUI 프레임워크에 의존하지 않습니다. 대신 리눅스 시스템 콜(`fork`, `pipe`, `exec`)과 POSIX 스레드(`pthread`), 그리고 `ncursesw` 라이브러리를 결합하여 순수 운영체제(OS) 추상화 계층 위에서 동작하는 고성능 비동기 파이프라인을 구현했습니다.

🎯 Key Features (핵심 기능)

* Zero-Dependency IPC Network (비동기 IPC 파이프라인)
  * 무거운 HTTP 라이브러리를 링크하는 대신, 시스템 콜(`pipe`, `fork`, `dup2`, `execlp`)을 활용하여 리눅스 내장 쉘 유틸리티(`curl`, `awk`)와 통신하는 독자적인 IPC(프로세스 간 통신) 아키텍처를 구축했습니다.
* Flicker-Free High-Resolution Braille Engine (고해상도 점자 렌더링)
  * 부동소수점 연산을 배제한 브레젠험(Bresenham) 직선 알고리즘과 비트 마스킹 기법을 결합하여, 터미널 1칸을 2x4 픽셀로 활용하는 고해상도 그래픽 엔진을 구현했습니다.
  * `ncurses`의 `erase()`를 활용한 더블 버퍼링으로 화면 깜빡임을 완벽히 제거했습니다.
* Thread-Safe Concurrency & Persistence (스레드 안전성 및 영속성)
  * 백그라운드 수집 스레드와 메인 렌더링 스레드를 완벽히 분리하고, 세분화된 `Mutex` 락을 통해 Race Condition을 원천 차단했습니다.
  * 저수준 파일 I/O(`open`, `read`, `write`)를 통해 관심 종목 데이터의 영속성(Persistence)과 스레드 안전한 로깅을 보장합니다.
* Graceful Shutdown (안전한 시스템 종료)
  * `SIGINT` 시그널 핸들링과 `waitpid()`를 통한 좀비 프로세스 방지 로직으로, 종료 시 단 1바이트의 메모리 누수도 허용하지 않습니다.



🏗️ System Architecture

본 프로젝트는 철저한 역할 분리(Separation of Concerns)를 통해 3-Tier 아키텍처로 설계되었으며, `Makefile`을 통해 관리됩니다. 자세한 모듈별 동작 원리는 아래의 문서를 참고하세요.

* 🌐 [**Network Module** (`network.c`)](docs/network_module.md) : 비동기 IPC 파이프라인 및 멀티스레드 워커
* 🗄️ [**Data Module** (`data.c`)](docs/data_module.md) : 상태 관리, Mutex 동기화, 커스텀 로거 및 저수준 파일 I/O
* 🎨 [**UI Module** (`ui.c`)](docs/ui_module.md) : ncurses 기반 터미널 렌더링 엔진 및 반응형 레이아웃
* ⚙️ [**Chart Module** (`chart.c`)](docs/chart_module.md) : 2D 가상 프레임버퍼, 브레젠험 알고리즘, 점자 디코딩
* 🚀 [**Main Module** (`main.c`)](docs/main_module.md) : 컨트롤 타워, 논블로킹 이벤트 루프, 좀비 프로세스 방어
* 🛠️ [**Build System** (`Makefile`)](docs/makefile_module.md) : 자동화된 증분 빌드 및 의존성 주입



💻 Getting Started (설치 및 실행)

1. Prerequisites (의존성 패키지 설치)
본 프로젝트는 리눅스 환경(Ubuntu 등)에 최적화되어 있습니다. 빌드 및 실행을 위해 컴파일러와 필수 라이브러리를 설치합니다.
```bash
sudo apt update
sudo apt install build-essential libncursesw5-dev curl

```

2. Build & Run (빌드 및 실행)

`git clone`으로 저장소를 복제한 뒤, 제공된 `Makefile`을 통해 원클릭으로 빌드합니다.

bash
1. 저장소 복제
git clone [https://github.com/DoYoung-Ju/TermStock.git](https://github.com/DoYoung-Ju/TermStock.git)
cd TermStock

2. 컴파일 및 링킹 (자동 빌드)
make

3. 애플리케이션 실행
./termstock

---

⌨️ Shortcuts (단축키 및 조작법)

프로그램 실행 중 터미널에서 직관적으로 컨트롤할 수 있는 단축키입니다. (논블로킹 환경으로 동작합니다)

| Key | Description (설명) |
| --- | --- |
| `↑` / `↓` | 관심 종목(Watchlist) 커서 이동 |
| `1` ~ `4` | 타임프레임 실시간 동적 전환 (`1`: 1m, `2`: 15m, `3`: 1h, `4`: 1d) |
| `a` / `A` | 새로운 관심 종목 추가 (예: `BTCUSDT`) |
| `d` / `D` | 현재 선택된 관심 종목 삭제 |
| `q` / `Q` | 스레드 및 메모리 안전 해제 후 프로그램 종료 (Graceful Shutdown) |

---

* System Programming (ELEC462) Team Project
