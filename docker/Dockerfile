FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY server/server.c .
COPY server/server.h .

RUN gcc -o server server.c -lpthread

EXPOSE 8080

CMD ["./server", "8080", "logs.txt"]