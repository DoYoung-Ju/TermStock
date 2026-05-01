# 📈 TermStock : 터미널 기반 실시간 암호화폐 차트 트래커

**Developer**

## 📖 프로젝트 소개 (Project Overview)
**TermStock**은 무거운 GUI 프레임워크나 웹 브라우저 없이, 오직 **리눅스 터미널(Terminal)** 창 하나만으로 바이낸스(Binance) API 기반의 실시간 시세와 글로벌 경제 뉴스를 모니터링할 수 있는 경량화된 차트 트래커입니다. 

C 언어의 한계를 넘어 가상 프레임버퍼(Framebuffer) 개념과 브레젠험 알고리즘을 도입하여, 터미널 환경에서도 HTS 수준의 **부드러운 캔들 차트(Candlestick Chart)**와 **깜빡임 없는(Flicker-Free) UI**를 구현했습니다.

## ✨ 주요 기능 (Key Features)
* **실시간 캔들 차트 렌더링**: 유니코드 블록(`█`)과 색상 쌍을 활용한 상승/하락장 시각화
* **Flicker-Free UI**: `ncurses` 가상 메모리 지우기(`erase()`)를 활용한 더블 버퍼링 기법 적용
* **비동기 멀티스레딩 파이프라인**: UI 멈춤 현상 없이 백그라운드에서 실시간 시세 및 RSS 뉴스 피드 수집
* **안전한 데이터 영속성**: 사용자의 관심 종목(Watchlist) 추가/삭제 상태를 로컬 스토리지에 동기화

## 📚 상세 모듈 문서 및 코드 리뷰 (Documentation)
본 프로젝트는 단일 파일 구조에서 벗어나, 역할과 책임에 따라 모듈화(Modularity)되어 있습니다. 각 모듈의 수학적 알고리즘과 메모리/스레드 제어 방식은 아래 상세 문서에서 확인할 수 있습니다.

* 🎨 **[UI & Rendering Engine (`ui_module.md`)](./docs/ui_module.md)** : 터미널 픽셀 스케일링 및 캔들 차트 수학적 렌더링
* 📊 **[Chart Graphics Engine (`chart_module.md`)](./docs/chart_module.md)** : 2D 가상 도화지 할당 및 점자(Braille) 비트 마스킹
* 🌐 **[Network Pipeline (`network_module.md`)](./docs/network_module.md)** : POSIX 멀티스레딩 및 리눅스 파이프(`|`) 기반 비동기 데이터 수집
* 🗄️ **[Data Management (`data_module.md`)](./docs/data_module.md)** : Mutex 스레드 동기화 및 파일 I/O
* 🚀 **[Main Control Tower (`main_module.md`)](./docs/main_module.md)** : 논블로킹 이벤트 루프 및 Graceful Shutdown
* 🛠️ **[Build System (`makefile_module.md`)](./docs/makefile_module.md)** : 컴파일/링킹 분리 및 의존성 주입 구조

## 💻 설치 및 실행 방법 (Installation & Usage)
POSIX 호환 운영체제(Linux, macOS)에서 아래 명령어를 통해 즉시 빌드 및 실행이 가능합니다.
```bash
$ git clone [https://github.com/DoYoung-Ju/TermStock.git](https://github.com/DoYoung-Ju/TermStock.git)
$ cd TermStock
$ make clean
$ make
$ ./termstock
