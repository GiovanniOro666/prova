CC = gcc
CFLAGS = -O3 -fopenmp -Wall -Wextra
LDFLAGS = -lm -fopenmp

SRCS = main.c filters.c signal_processing.c trigger.c drift_analysis.c io.c
OBJS = $(SRCS:.c=.o)
TARGET = dosews

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean