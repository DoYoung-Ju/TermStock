# 📊 Chart Module (`chart.h`, `chart.c`) : 2D Virtual Braille Graphics Engine

**Author**: 2024017497 주도영

## 1. Overview (개요)
`chart` 모듈은 TermStock 프로젝트의 심장부로, 일반적인 텍스트 환경인 터미널을 **고해상도 2D 그래픽 캔버스**처럼 사용할 수 있게 만들어주는 독자적인 렌더링 엔진입니다. 

외부 그래픽 라이브러리(OpenGL 등)에 의존하지 않고, 메모리상에 가상 프레임버퍼(Framebuffer)를 생성하여 픽셀 단위의 제어, 선 긋기, 비트 마스킹(Bit-masking), 유니코드 인코딩까지 컴퓨터 그래픽스의 로우레벨(Low-level) 파이프라인을 C 언어만으로 밑바닥부터 구현했습니다.

## 2. Core Concepts (핵심 기술 및 알고리즘)

* **Virtual Bitmap Canvas (가상 비트맵 캔버스):**
  터미널의 칸 수(`term_w`, `term_h`)를 점자 문자의 특성(가로 2px, 세로 4px)에 맞춰 해상도를 뻥튀기한 1차원 동적 배열(`uint8_t* grid`)로 메모리에 할당합니다. 2D 좌표 `(x, y)`를 1D 인덱스 `(y * width + x)`로 변환하여 캐시 히트율(Cache Hit Rate)과 메모리 접근 속도를 극대화했습니다.
* **Bresenham's Line Algorithm (브레젠험 직선 알고리즘):**
  점과 점 사이를 이을 때 소수점(Floating-point) 연산을 배제하고 오직 **정수(Integer)의 덧셈과 뺄셈, 비트 시프트만으로** 가장 완벽한 픽셀 직선을 찾아내는 컴퓨터 그래픽스의 표준 알고리즘을 직접 구현했습니다.
* **Bitwise Braille Encoding (비트 연산 기반 점자 인코딩):**
  ISO/TR 11941 표준을 역산하여, 2x4(총 8픽셀) 크기의 블록을 스캔한 뒤 켜져 있는 픽셀 위치에 따라 비트 논리합(Bitwise OR, `|`) 연산을 수행합니다. 압축된 1바이트 숫자를 UTF-8 점자 유니코드 시작점(`U+2800`)과 결합해 터미널에 출력합니다.

## 3. Function Details (주요 메서드 분석)

### `Canvas* create_canvas(int term_w, int term_h)` / `void free_canvas(Canvas* canvas)`
* **역할:** C++의 생성자(Constructor)와 소멸자(Destructor) 역할을 수행합니다.
* **특징:** 필요한 픽셀 수만큼 `calloc`을 통해 메모리를 안전하게 할당 및 초기화하며, 사용 후 메모리 누수(Leak)가 발생하지 않도록 `free`를 강제하는 구조를 가집니다.

### `void set_pixel(Canvas* canvas, int x, int y)`
* **역할:** 가상 도화지의 특정 좌표에 점(1)을 찍습니다.
* **특징:** 입력된 좌표가 캔버스 범위를 벗어나는지 검사하는 `Out of Bounds` 방어 로직이 포함되어 있어 렌더링 중 Segmentation Fault가 발생하지 않습니다.

### `void draw_line(Canvas* canvas, int x0, int y0, int x1, int y1)`
* **역할:** 브레젠험 알고리즘을 사용하여 두 좌표 사이에 선을 긋습니다.
* **동작 원리:** X축과 Y축의 절대 거리(`dx`, `dy`)와 누적 오차값(`err`)을 계산하여, 각 스텝마다 어느 방향으로 픽셀을 찍고 나아갈지 수학적으로 결정합니다.

### `void draw_canvas_to_screen(Canvas* canvas, int start_y, int start_x)`
* **역할:** 완성된 비트맵 메모리를 터미널 화면으로 전송(Flush)합니다.
* **동작 원리:** 가로 2픽셀, 세로 4픽셀 단위의 돋보기(Window)로 캔버스를 순회하며 8개의 픽셀 상태를 비트 마스킹으로 압축한 뒤, `get_braille_char()`를 호출해 3바이트 문자열로 디코딩합니다.

## 4. Technical Highlights for Presentation (발표 핵심 포인트)
> "이 모듈의 진가는 **'극한의 최적화'**에 있습니다. 터미널 환경에서 그래픽을 부드럽게 표현하기 위해, 무거운 소수점 연산을 철저히 배제하고 브레젠험 알고리즘과 비트 마스킹을 도입했습니다. 덕분에 CPU 오버헤드 없이 `10x40` 크기의 터미널 공간을 `40x160` (총 6,400 서브 픽셀)의 고해상도 도화지로 변환해 낼 수 있었으며, 이는 단순한 API 데이터 뷰어를 넘어 저만의 독자적인 **'터미널 렌더링 엔진'**을 구축했다는 데 큰 의의가 있습니다."