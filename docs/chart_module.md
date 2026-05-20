⚙️ Chart Module (chart.h, chart.c) : 2D Virtual Braille Graphics Engine

1. Module Overview
chart 모듈은 외부 그래픽 라이브러리에 의존하지 않고, 텍스트 기반의 터미널 환경을 고해상도 2D 캔버스로 변환하는 TermStock 프로젝트의 핵심 렌더링 엔진입니다.

메모리 상에 가상 프레임버퍼(Virtual Framebuffer)를 직접 구현하고, 브레젠험 직선 알고리즘과 비트 마스킹 기법을 결합하여 컴퓨터 그래픽스의 로우레벨(Low-level) 파이프라인을 C 언어만으로 밑바닥부터 설계했습니다.

2. Core Architecture & Algorithms
본 모듈은 제한된 터미널 환경에서 극한의 렌더링 속도와 메모리 효율성을 달성하기 위해 세 가지 핵심 최적화 기법을 적용했습니다.

Virtual Bitmap Canvas (1차원 배열 메모리 최적화)
터미널의 1칸(Block)을 가로 2px, 세로 4px의 서브 픽셀로 분할하여 해상도를 확장합니다. 2차원 좌표계 (x, y)를 1차원 동적 배열 인덱스 (y * width + x)로 매핑하여 캐시 히트율(Cache Hit Rate)과 메모리 접근 속도를 극대화했습니다.

Bresenham's Line Algorithm (정수 기반 렌더링)
부동소수점(Floating-point) 연산을 완전히 배제하고, 오직 정수의 사칙연산과 비트 시프트만으로 두 점 사이의 최단 경로를 계산합니다. 이를 통해 CPU 오버헤드를 최소화하고 실시간 렌더링 성능을 확보했습니다.

Bitwise Braille Encoding (비트 연산 점자 디코딩)
ISO/TR 11941 표준을 역산하여, 2x4 픽셀 블록 내 활성화된 점의 위치를 비트 논리합(|)으로 누적합니다. 압축된 1바이트 데이터를 UTF-8 점자 유니코드 시작점(U+2800)과 결합해 화면에 출력합니다.

3. Lifecycle & Core Methods
C++의 객체 지향 프로그래밍(OOP) 철학을 차용하여, 상태 데이터(Canvas)와 조작 함수를 묶고 안전한 메모리 생명주기(Lifecycle)를 갖도록 설계했습니다.

📦 객체 생성 및 소멸 (Memory Management)
create_canvas(term_w, term_h): calloc을 사용해 필요한 픽셀 수만큼 캔버스를 동적 할당하고 초기화하는 생성자(Constructor) 역할을 수행합니다.

free_canvas(Canvas* canvas): 렌더링 사이클이 끝난 후 메모리 누수(Leak)를 방지하기 위해 힙(Heap) 영역을 강제로 반환하는 소멸자(Destructor) 역할을 수행합니다.

🖌️ 렌더링 파이프라인 (Rendering Pipeline)
set_pixel(Canvas* canvas, int x, int y): 가상 캔버스의 특정 좌표에 점을 기록합니다. 좌표가 화면을 벗어나는지 검사하는 Out-of-Bounds 방어 로직을 포함하여 Segmentation Fault를 원천 차단합니다.

draw_line(Canvas* canvas, int x0, int y0, int x1, int y1): X축과 Y축의 누적 오차값(err)을 계산하는 브레젠험 알고리즘을 수행하여 픽셀 간의 선을 긋습니다.

draw_canvas_to_screen(Canvas* canvas, int start_y, int start_x): 가상 메모리에 완성된 비트맵을 2x4 윈도우 단위로 스캔하여 점자 문자로 디코딩한 뒤, 터미널 화면으로 일괄 전송(Flush)합니다.

4. Technical Highlights
"이 모듈의 진가는 **'극한의 최적화'**에 있습니다. 단순히 API 데이터를 텍스트로 나열하는 것을 넘어, 무거운 소수점 연산을 철저히 배제하고 CPU 오버헤드 없이 10x40 크기의 터미널 공간을 40x160 (총 6,400 서브 픽셀)의 고해상도 도화지로 변환해 냈습니다. 이는 제약된 터미널 환경에서도 성능 타협 없이 HTS 수준의 시각화를 이끌어낸 독자적인 렌더링 엔진 설계의 결과물입니다."