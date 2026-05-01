# 🎨 UI Module (`ui.h`, `ui.c`) : Terminal Rendering Engine

**Author**: 2024017497 주도영

## 1. Overview (개요)
`ui` 모듈은 TermStock 프로젝트의 **View 계층**을 담당하는 핵심 렌더링 엔진입니다. 백그라운드 스레드에서 수집된 원시 데이터(Raw Data)를 터미널 화면의 픽셀(Row/Col) 좌표계로 매핑하고, `ncursesw` 라이브러리를 통해 시각화합니다. 

본 모듈은 C언어로 작성되었으나, 내부 로직은 **C++의 클래스(Class) 메서드 동작 방식**을 차용하여 상태(Data)와 표현(Render)을 완벽하게 분리하는 구조로 설계되었습니다.

## 2. Core Concepts (핵심 렌더링 개념)

* **Mathematical Scaling (수학적 스케일링):**
  동적으로 변하는 가격 데이터(예: $60,000 ~ $70,000)를 제한된 터미널 높이(예: 15줄)에 맞게 정규화(Normalization)하는 수학적 매핑 알고리즘이 적용되어 있습니다.
* **Color Pairing (조건부 색상 매핑):**
  시가(Open)와 종가(Close) 데이터를 비교 알고리즘으로 분석하여, 상승장(양봉)일 경우 초록색(`COLOR_PAIR(1)`), 하락장(음봉)일 경우 빨간색(`COLOR_PAIR(2)`)을 실시간으로 덧입힙니다.

## 3. Function Details (주요 메서드 분석)

### `void draw_layout(int max_y, int max_x)`
* **역할:** 화면의 뼈대가 되는 베이스 레이아웃과 구분선(Divider)을 그립니다.
* **특징:** 단순 ASCII 문자가 아닌 터미널 기본 `ACS (Alternate Character Set)` 선 문자를 활용하여 깨짐 없는 UI 박스를 렌더링합니다.

### `void draw_ascii_chart(int y, int x, float* open_data, float* close_data)`
* **역할:** 1D 스파크라인(Sparkline) 형태의 경량화된 차트를 출력합니다.
* **동작 원리:** 데이터를 0.0 ~ 1.0 사이로 정규화한 뒤, 8단계의 블록 배열(`chart_bars`) 인덱스에 매핑하여 단일 라인으로 추세를 표현합니다. (현재는 고급 차트로 대체되어 하위 호환성을 위해 보존)

### `void draw_braille_chart(int start_y, int start_x, int width, int height, float* prices, int num_points)`
* **역할:** 2x4 서브 픽셀 해상도를 지원하는 점자(Braille) 기반 라인 차트를 그립니다.
* **기술적 특징:**
  * **RAII 패턴 적용:** 함수 진입 시 `Canvas` 객체를 동적 할당(`create_canvas`)하고, 렌더링이 끝나면 즉시 메모리를 해제(`free_canvas`)하여 C++의 스마트 포인터나 RAII(Resource Acquisition Is Initialization) 패턴처럼 **메모리 누수(Memory Leak)를 원천 차단**했습니다.
  * 브레젠험(Bresenham) 알고리즘 모듈과 연동하여 부드러운 곡선 차트를 구현합니다.

### `void draw_candlestick_chart(...)`
* **역할:** 다중 라인(Multi-line)을 활용한 정통 OHLC 캔들 차트를 렌더링합니다.
* **기술적 특징:**
  * 배열(개념적으로 `std::vector<float>`와 동일하게 취급)로 전달된 시가/종가 데이터를 순회하며, 터미널의 Y 좌표를 역산합니다. (터미널은 상단이 `y=0`이므로 `1.0f - normalized` 공식을 적용)
  * 계산된 `top_y`부터 `bottom_y`까지 유니코드 블록(`█`)을 반복 출력하여 속이 꽉 찬 형태의 캔들 기둥을 완성합니다.

## 4. Technical Highlights for Presentation (발표 핵심 포인트)
> "데이터를 단순히 텍스트로 찍어내는 것을 넘어, **터미널을 하나의 2D 그래픽 캔버스로 취급**했습니다. 특히 `draw_candlestick_chart` 함수에서는 가격의 상하한선 범위(`range`)를 구하고 이를 화면 높이 비율로 환산하는 **선형 보간 로직**을 직접 구현하여, 어떤 종목을 선택하든 차트가 UI를 이탈하지 않고 완벽한 비율로 자동 렌더링되도록 자동화(Auto-Scaling)했습니다."

