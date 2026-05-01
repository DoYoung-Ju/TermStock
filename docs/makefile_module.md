# 🛠️ Build System (`Makefile`) : Automated Compilation & Dependency Management

**Author**: 2024017497 주도영

## 1. Overview (개요)
본 문서에서는 TermStock 프로젝트의 자동화된 빌드 파이프라인을 정의하는 `Makefile`의 구조와 역할을 설명합니다. 

단순히 하나의 파일로 이루어진 프로젝트를 넘어, 5개 이상의 모듈(`main`, `ui`, `data`, `network`, `chart`)로 분리된 대규모 C 프로그램을 효율적으로 컴파일하고 링크(Link)하기 위해 작성되었습니다. 이를 통해 개발자는 복잡한 `gcc` 명령어를 매번 입력할 필요 없이 `make` 명령어 하나로 전체 시스템을 빌드할 수 있습니다.

## 2. Core Concepts (핵심 컴파일 철학)

* **Separation of Compilation and Linking (컴파일과 링킹의 분리):**
  모든 `.c` 소스 파일을 한 번에 묶어서 실행 파일로 만들지 않고, 각각을 독립적인 기계어 조각인 `.o` (Object File)로 먼저 번역(Compile)합니다. 이후 링커(Linker)가 이 조각들과 외부 라이브러리를 하나로 이어 붙여(Link) 최종 실행 파일(`./termstock`)을 생성합니다.
* **Incremental Build (증분 빌드 최적화):**
  코드 한 줄을 수정했을 때 전체 프로젝트를 처음부터 다시 빌드하는 것은 엄청난 시간 낭비입니다. `Makefile`의 의존성(Dependency) 추적 기능을 활용하여, **수정된 소스 파일만 타겟으로 삼아 새롭게 컴파일**하도록 구성했습니다.
* **Dependency Injection (외부 라이브러리 의존성 주입):**
  C언어 표준 라이브러리만으로는 구현할 수 없는 멀티스레딩과 터미널 UI 렌더링을 위해, 링킹 단계에서 필수적인 리눅스 시스템 라이브러리들을 안전하게 주입합니다.

## 3. Key Variables & Flags (핵심 변수 및 컴파일 옵션)

### 컴파일러 및 플래그 세팅
* `CC = gcc` : GNU C Compiler를 기본 빌드 도구로 사용합니다.
* `CFLAGS = -Wall -O2` : 
  * `-Wall`: 모든 경고(Warning) 메시지를 켜서 사소한 메모리 실수나 타입 불일치를 컴파일 단계에서 엄격하게 잡아냅니다.
  * `-O2`: 프로덕션 레벨의 코드 최적화 옵션을 켜서 차트 렌더링 엔진의 실행 속도를 극대화합니다. (디버깅 시에는 `-g` 사용)

### 링커 플래그 (LDFLAGS)
본 프로젝트가 정상적으로 동작하기 위한 3가지 필수 뼈대입니다.
* `-lncursesw` : Wide-character(UTF-8)를 지원하는 ncurses 라이브러리. 점자 특수문자(`⡷`)와 캔들 블록(`█`)을 깨짐 없이 터미널에 렌더링하기 위해 필수적입니다.
* `-lpthread` : POSIX Thread 라이브러리. 백그라운드 데이터 수집(네트워크)과 화면 렌더링(UI)을 멀티스레드로 분리하고, Mutex 동기화를 처리하기 위해 링크합니다.
* `-lm` : Math 라이브러리. 차트 스케일링을 위한 `round()`, `abs()` 등 수학적 계산을 처리합니다.

### 타겟(Target) 목록
* `OBJS = main.o ui.o network.o data.o chart.o` : 프로젝트를 구성하는 오브젝트 파일들의 리스트입니다.
* `all` : 기본 빌드 타겟으로, 최종 실행 파일인 `termstock`을 생성합니다.
* `clean` : `make clean` 명령어 입력 시 실행되며, 디렉토리 내에 쌓인 모든 찌꺼기 `.o` 파일들과 기존 실행 파일을 일괄 삭제하여 **Clean Build** 환경을 보장합니다.

## 4. Technical Highlights for Presentation (발표 핵심 포인트)
> "제가 이 프로젝트를 모듈화하면서 가장 신경 쓴 부분은 의존성 관리입니다. 만약 `chart.h`의 구조체를 변경했는데 `Makefile`이 제대로 구성되어 있지 않다면, 변경 사항이 전체 코드에 반영되지 않아 치명적인 버그를 유발합니다. 저는 컴파일 단계(`.c -> .o`)와 링킹 단계(`.o -> binary`)를 완벽하게 분리하여, 리눅스 환경에서의 C/C++ 빌드 파이프라인이 어떻게 동작하는지 100% 통제할 수 있는 환경을 구축했습니다."