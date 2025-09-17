# Tên file nguồn
SRC = mp.c

# Tên file thực thi
TARGET = counter

# Trình biên dịch
CC = gcc

# Tùy chọn biên dịch
CFLAGS = -Wall -O2 -pthread

# Mục mặc định
all: $(TARGET)

# Build chương trình
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Chạy chương trình
run: $(TARGET)
	./$(TARGET)

# Xóa file thực thi
clean:
	rm -f $(TARGET)
