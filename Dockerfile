FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY backend backend
RUN g++ -std=c++17 -O2 -pthread -o backend/reverser backend/main.cpp

FROM debian:bookworm-slim
WORKDIR /app/backend
COPY --from=build /app/backend/reverser .
COPY frontend ../frontend
EXPOSE 8080
CMD ["./reverser"]
