# 💡 컴파일러 설정
CC = gcc

# 💡 컴파일 옵션 (-Wall: 경고 다 띄움, -O2: 최적화, -D_: ncursesw 유니코드 설정)
CFLAGS = -Wall -O2 -D_XOPEN_SOURCE_EXTENDED=1

# 💡 링커 옵션 (우리가 쓰던 라이브러리들)
LDFLAGS = -lncursesw -lpthread

# 💡 최종 실행 파일 이름
TARGET = termstock

# 💡 컴파일할 C 파일들을 목적 파일(.o)로 변환할 리스트
OBJS = main.o ui.o network.o data.o chart.o

# 기본 목표 (make를 쳤을 때 실행되는 곳)
all: $(TARGET)

# 최종 실행 파일을 만드는 규칙
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 각각의 .c 파일을 .o 파일로 컴파일하는 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# 💡 찌꺼기 파일 청소용 (make clean)
clean:
	rm -f $(OBJS) $(TARGET)