🎨 UI Module (ui.h, ui.c) : Terminal Rendering Engine

1. Module Overview (모듈 개요)
ui 모듈은 TermStock 프로젝트의 프론트엔드(Front-end) 렌더링 엔진이자 View 계층을 담당합니다.
백그라운드 네트워크 스레드가 수집한 원시 데이터(Raw Data)를 터미널 화면의 픽셀(Row/Col) 좌표계로 매핑하고, ncursesw 라이브러리를 통해 시각화합니다. C언어로 작성되었으나 내부 로직은 C++의 클래스(Class) 메서드 동작 방식을 차용하여, 상태(Data)와 표현(Render)을 완벽하게 분리하는 MVC 패턴의 철학을 구현했습니다.

2. Core Rendering Concepts (핵심 렌더링 기법)
Mathematical Auto-Scaling (수학적 정규화 및 선형 보간):
동적으로 요동치는 가격 데이터(예: $60,000 ~ $70,000)를 제한된 터미널 높이(예: 15줄)에 꽉 차게 맞추기 위해 정규화(Normalization) 알고리즘을 적용했습니다. 특히 터미널은 Y축이 아래로 갈수록 증가(y=0이 최상단)하므로, (1.0f - normalized) * height 공식을 적용해 차트가 뒤집히지 않도록 수학적으로 역산했습니다.

RAII-style Memory Management (안전한 메모리 생명주기):
매 프레임마다 차트를 새로 그릴 때 발생하는 동적 할당 오버헤드와 메모리 누수(Leak)를 원천 차단하기 위해, C++의 스마트 포인터나 RAII(Resource Acquisition Is Initialization) 패턴을 C언어로 모방했습니다. 함수 진입 시 도화지를 할당(create_canvas)하고 렌더링 직후 즉시 해제(free_canvas)하여 메모리 안전성을 극대화했습니다.

Semantic Color Mapping (시맨틱 컬러 마스킹):
시가(Open)와 종가(Close)를 비교하는 알고리즘을 통해 상승장(양봉)일 경우 초록색(COLOR_PAIR(1)), 하락장(음봉)일 경우 빨간색(COLOR_PAIR(2))을 동적으로 덧입힙니다.

3. Function Details (주요 메서드 분석)
📐 draw_layout(int max_y, int max_x)
역할: 화면의 뼈대가 되는 베이스 레이아웃과 구분선(Divider)을 그립니다.

특징: 단순 ASCII 문자가 아닌 터미널 기본 ACS (Alternate Character Set) 선 문자를 활용하여 깨짐 없는 UI 박스를 렌더링합니다. 터미널 창의 크기(max_x, max_y) 변화에 맞춰 1:2 비율로 패널을 나누는 반응형(Responsive) 설계가 적용되었습니다.

🕯️ draw_candlestick_chart(...)
역할: 다중 라인(Multi-line)을 활용한 정통 OHLC 캔들 차트를 렌더링합니다.

동작 원리: 전달된 시가/종가 배열을 순회하며 터미널 Y 좌표를 역산한 뒤, 계산된 top_y부터 bottom_y까지 유니코드 블록(█)을 반복 출력하여 속이 꽉 찬 형태의 캔들 기둥을 렌더링합니다.

⠷ draw_braille_chart(...)
역할: chart 모듈과 연동하여 2x4 서브 픽셀 해상도를 지원하는 점자(Braille) 기반 라인 차트를 그립니다.

동작 원리: 메모리에 가상 캔버스를 올리고 브레젠험(Bresenham) 알고리즘으로 부드러운 곡선을 그린 뒤, 이를 3바이트 UTF-8 점자로 디코딩하여 화면에 뿌려주는 고해상도 그래픽 파이프라인의 최종 목적지입니다.

📊 draw_ascii_chart(int y, int x, float* open_data, float* close_data)
역할: 1D 스파크라인(Sparkline) 형태의 경량화된 차트를 출력합니다.

동작 원리: 데이터를 8단계의 블록 배열(chart_bars) 인덱스에 매핑하여 단일 라인으로 추세를 표현합니다. (하위 호환성을 위해 보존)

4. Technical Highlights for Presentation (발표 핵심 포인트)
"데이터를 단순히 텍스트로 찍어내는 것을 넘어, 터미널 자체를 하나의 2D 그래픽 캔버스로 취급했습니다. 특히 draw_candlestick_chart 함수에서는 가격의 상하한선 범위(range)를 구하고 이를 화면 높이 비율로 환산하는 선형 보간 로직을 직접 구현했습니다. 그 결과, 어떠한 종목을 선택하거나 터미널 창의 크기를 마음대로 조절하더라도, 차트가 UI를 이탈하지 않고 완벽한 비율로 반응하며 자동 렌더링(Auto-Scaling)되는 견고한 프론트엔드를 구축해 냈습니다."