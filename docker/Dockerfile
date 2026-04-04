FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY server/server.c ./
COPY server/server.h ./
COPY server/http_server.c ./
COPY server/http_server.h ./

RUN gcc -O2 -Wall -Wextra -o server server.c http_server.c -lpthread

EXPOSE 5000 8081

CMD ["./server", "5000", "8081", "server.log"]