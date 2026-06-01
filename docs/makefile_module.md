🛠️ Build System (Makefile) : Automated Compilation & Dependency Management

1. Module Overview (모듈 개요)
본 문서에서는 TermStock 프로젝트의 자동화된 빌드 파이프라인을 정의하는 Makefile의 구조와 역할을 설명합니다.

단일 파일 형태의 토이 프로젝트를 넘어, 역할에 따라 5개의 서브 모듈(main, ui, data, network, chart)로 철저히 분리된 대규모 C 프로그램을 효율적으로 컴파일하고 링크(Link)하기 위해 작성되었습니다. 복잡한 gcc 명령어와 의존성 트리를 매번 수동으로 관리할 필요 없이, make 명령어 하나로 전체 시스템 프로그래밍 파이프라인을 빌드할 수 있습니다.

2. Core Build Concepts (핵심 빌드 및 컴파일 철학)
본 프로젝트의 빌드 시스템은 리눅스 환경의 C/C++ 컴파일러 표준 동작 방식을 100% 활용하도록 설계되었습니다.

Separation of Compilation and Linking (컴파일과 링킹의 분리):
모든 .c 소스 파일을 한 번에 묶어서 실행 파일로 번역하지 않습니다. 각각의 소스 코드를 독립적인 기계어 조각인 .o (Object File)로 먼저 개별 번역(Compile)한 뒤, 마지막에 링커(Linker)가 이 조각들과 OS 라이브러리를 하나로 이어 붙여(Link) 최종 바이너리(./termstock)를 생성합니다.

Incremental Build (증분 빌드 최적화):
코드 한 줄을 수정했을 때 전체 프로젝트를 처음부터 다시 빌드하는 것은 엄청난 시간 낭비입니다. Makefile의 의존성 추적 및 내부 매크로($@, $<, $^)를 활용하여, 수정된 소스 파일만 타겟으로 삼아 새롭게 컴파일하도록 구성했습니다.

Preprocessor Macros & Dependency Injection (전처리기 매크로 및 의존성 주입):
시스템 프로그래밍 레벨의 멀티스레딩 제어와 터미널 UI 렌더링을 위해, 컴파일 단계에서는 POSIX 확장 매크로를 주입하고 링킹 단계에서는 필수적인 리눅스 시스템 라이브러리(pthread, ncursesw)를 안전하게 주입합니다.

3. Key Variables & Flags (핵심 변수 및 컴파일 옵션)
⚙️ Compiler & Preprocessor Flags (컴파일러 및 전처리기 플래그)
CC = gcc : GNU C Compiler를 기본 빌드 도구로 사용합니다.

CFLAGS = -Wall -O2 -D_XOPEN_SOURCE_EXTENDED=1

-Wall: 모든 경고(Warning) 메시지를 활성화하여 메모리 누수나 포인터 타입 불일치를 엄격하게 잡아냅니다.

-O2: 프로덕션 레벨의 코드 최적화 옵션을 켜서 차트 렌더링 엔진의 실행 속도를 극대화합니다.

-D_XOPEN_SOURCE_EXTENDED=1: (핵심) X/Open POSIX 확장 표준을 활성화하는 전처리기 매크로입니다. 점자 특수문자(⡷)와 캔들 블록(█) 등 UTF-8 기반의 와이드 캐릭터(Wide-character)를 ncurses가 정상적으로 인식하고 렌더링하기 위해 컴파일 타임에 반드시 주입되어야 합니다.

🔗 Linker Flags (링커 플래그)
본 프로젝트가 정상적으로 동작하기 위한 필수 OS 라이브러리입니다.

-lncursesw : Wide-character(UTF-8)를 지원하는 ncurses 라이브러리. 텍스트 환경인 터미널을 그래픽 캔버스로 활용하기 위해 링크합니다.

-lpthread : POSIX Thread 라이브러리. 네트워크 비동기 파이프라인과 UI 렌더링을 완전히 분리하고, Mutex 기반의 동시성 제어를 수행하기 위해 운영체제의 스레드 라이브러리를 주입합니다.

4. Target Rules & Automation (타겟 규칙 및 자동화)
OBJS = main.o ui.o network.o data.o chart.o : 프로젝트를 구성하는 오브젝트 파일들의 리스트입니다.

%.o: %.c : 와일드카드를 사용하여 개별 .c 파일을 동명의 .o 파일로 변환하는 범용 컴파일 규칙입니다.

all : 기본 빌드 타겟으로, 최종 바이너리인 termstock을 생성합니다.

clean : 빌드 디렉토리 내에 쌓인 모든 찌꺼기 .o 파일들과 기존 실행 파일을 일괄 삭제하여, 의존성이 꼬였을 때 완벽한 Clean Build 환경을 보장합니다.
