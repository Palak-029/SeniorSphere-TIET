FROM debian:trixie-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake make libsodium-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY CMakeLists.txt ./
COPY src ./src
COPY third_party ./third_party
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2

FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends libsodium23 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/build/seniorsphere /app/seniorsphere
ENV PORT=8080 DATA_DIR=/data SEED_DEMO=0
EXPOSE 8080
CMD ["/app/seniorsphere"]
