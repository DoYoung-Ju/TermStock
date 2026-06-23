# 📈 TermStock

> **Terminal-based Real-time Crypto Chart Tracker**
> 
> 시스템 프로그래밍(ELEC462) 기말 팀 프로젝트

<img width="1297" height="763" alt="image" src="https://github.com/user-attachments/assets/14680f00-4731-4c2c-9b6b-0d2a537ef72f" />
<img width="1299" height="763" alt="image" src="https://github.com/user-attachments/assets/44ba9d0a-92e0-4ea5-96df-4cb8bbdf500d" />



무거운 HTS나 웹 브라우저 없이, **오직 OS 레벨 자원과 터미널 환경만으로** 동작하는 초경량 실시간 암호화폐 차트 트래커입니다.

---

## 📋 Table of Contents

- [About The Project](#-about-the-project)
- [Key Features](#-key-features)
- [System Architecture](#-system-architecture)
- [Getting Started](#-getting-started)
  - [Prerequisites](#prerequisites)
  - [Build & Run](#build--run)
- [Usage & Shortcuts](#-usage--shortcuts)
- [Team Members](#-team-members)

---

## 🚀 About The Project

`TermStock`은 **C언어** 기반의 터미널 애플리케이션으로, 외부의 무거운 JSON 파싱 라이브러리나 GUI 프레임워크에 의존하지 않습니다.

대신 리눅스 시스템 콜(`fork`, `pipe`, `exec`)과 POSIX 스레드(`pthread`), 그리고 `ncursesw` 라이브러리를 결합하여 **순수 OS 추상화 계층 위에서 동작하는 고성능 비동기 파이프라인**을 구현했습니다.

> 📸 **Demo Screenshot**
<img width="1296" height="759" alt="image" src="https://github.com/user-attachments/assets/2a5a143e-4e9e-4944-b5ff-a67f60382276" />

---

## 🎯 Key Features

| 기능 | 설명 |
| --- | --- |
| **Zero-Dependency IPC Network** | `pipe`, `fork`, `dup2`, `execlp` 시스템 콜로 `curl`·`awk`와 통신하는 독자적 IPC 아키텍처 |
| **Flicker-Free Braille Engine** | 브레젠험(Bresenham) 알고리즘 + 비트 마스킹으로 터미널 1칸을 2×4 픽셀로 활용하는 고해상도 그래픽 엔진 |
| **Thread-Safe Concurrency** | 백그라운드 수집 스레드 ↔ 메인 렌더링 스레드 완전 분리, 세분화된 Mutex 락으로 Race Condition 원천 차단 |
| **Low-Level File I/O** | `open`/`read`/`write` 기반 관심 종목 영속성(Persistence) 및 스레드 안전 로깅 |
| **Graceful Shutdown** | `SIGINT` 핸들링 + `waitpid()` 좀비 프로세스 방지, 종료 시 메모리 누수 0 보장 |

---

## 🏗️ System Architecture

철저한 역할 분리(Separation of Concerns)를 통한 3-Tier 아키텍처로 설계되었습니다.

```
┌─────────────────────────────────────────────┐
│              main.c (Control Tower)         │
│    논블로킹 이벤트 루프 / 시그널 핸들링      │
└────────────┬────────────────┬───────────────┘
             │                │
    ┌────────▼──────┐  ┌──────▼────────┐
    │  network.c    │  │    ui.c       │
    │  비동기 IPC   │  │ ncurses 렌더링 │
    │  멀티스레드   │  │ 반응형 레이아웃│
    └────────┬──────┘  └──────┬────────┘
             │                │
    ┌────────▼──────┐  ┌──────▼──────────┐
    │   data.c      │  │   chart.c       │
    │ 상태 관리     │  │ 브레젠험 알고리즘│
    │ Mutex 동기화  │  │ 점자 렌더링 엔진 │
    └───────────────┘  └─────────────────┘
```

각 모듈의 상세 문서:

- 🌐 [**Network Module** (`network.c`)](docs/network_module.md) — 비동기 IPC 파이프라인 및 멀티스레드 워커
- 🗄️ [**Data Module** (`data.c`)](docs/data_module.md) — 상태 관리, Mutex 동기화, 커스텀 로거 및 저수준 파일 I/O
- 🎨 [**UI Module** (`ui.c`)](docs/ui_module.md) — ncurses 기반 터미널 렌더링 엔진 및 반응형 레이아웃
- ⚙️ [**Chart Module** (`chart.c`)](docs/chart_module.md) — 2D 가상 프레임버퍼, 브레젠험 알고리즘, 점자 디코딩
- 🚀 [**Main Module** (`main.c`)](docs/main_module.md) — 컨트롤 타워, 논블로킹 이벤트 루프, 좀비 프로세스 방어
- 🛠️ [**Build System** (`Makefile`)](docs/makefile_module.md) — 자동화된 증분 빌드 및 의존성 주입

---

## 💻 Getting Started

### Prerequisites

본 프로젝트는 Linux 환경(Ubuntu 권장)에 최적화되어 있습니다. 다음 패키지를 먼저 설치해주세요.

```bash
sudo apt update
sudo apt install build-essential libncursesw5-dev curl
```

| 패키지 | 용도 |
| --- | --- |
| `build-essential` | GCC 컴파일러 및 make |
| `libncursesw5-dev` | 터미널 UI 라이브러리 (유니코드 점자 지원) |
| `curl` | Binance API 데이터 수집 |

### Build & Run

```bash
# 1. 저장소 복제
git clone https://github.com/DoYoung-Ju/TermStock.git
cd TermStock

# 2. 빌드 (Makefile 자동 빌드)
make

# 3. 실행
./termstock

# 4. 빌드 결과물 정리 (선택사항)
make clean
```

---

## ⌨️ Usage & Shortcuts

프로그램 실행 중 아래 단축키로 조작할 수 있습니다. (논블로킹 환경으로 동작합니다)

| Key | Description |
| --- | --- |
| `↑` / `↓` | 관심 종목(Watchlist) 커서 이동 |
| `1` | 타임프레임: 1분봉 (1m) |
| `2` | 타임프레임: 15분봉 (15m) |
| `3` | 타임프레임: 1시간봉 (1h) |
| `4` | 타임프레임: 1일봉 (1d) |
| `a` / `A` | 새 관심 종목 추가 (예: `BTCUSDT`) |
| `d` / `D` | 현재 선택된 관심 종목 삭제 |
| `q` / `Q` | 안전 종료 (Graceful Shutdown) |

---

## 👥 Team Members

| 이름 | 학번 |
| --- | --- |
| 주도영 | 2024017497 |
| 전진영 | 2024017438 |
| 김민성 | 2024017248 |

---

<p align="center">
  <sub>System Programming (ELEC462) Team Project · 2025</sub>
</p>
