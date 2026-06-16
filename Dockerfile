# ==============================================================================
# FocusForge — Multi-stage Dockerfile
# Stage 1 (builder): компиляция с userver
# Stage 2 (runtime): минимальный образ для продакшена
# ==============================================================================

# ── Builder ───────────────────────────────────────────────────────────────────
FROM --platform=linux/amd64 ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS builder

ARG BUILD_TYPE=Release
WORKDIR /build

# Копируем build-манифесты первыми (кешируем слой зависимостей)
COPY CMakeLists.txt .

# Копируем исходники
COPY src/ src/
COPY configs/ configs/

# Сборка БЕЗ Conan-toolchain: все зависимости (userver, fmt, openssl, ...)
# берутся из базового userver-образа. Conan-пресет сюда подключать НЕЛЬЗЯ —
# он линкует conan-openssl 3.2.0, который конфликтует с системным OpenSSL у
# libpq и ломает SCRAM-аутентификацию PostgreSQL ("could not generate nonce").
RUN cmake -B cmake-build \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TESTS=OFF \
    && cmake --build cmake-build --parallel "$(nproc)" --target focusforge

# ── Runtime ───────────────────────────────────────────────────────────────────
FROM --platform=linux/amd64 ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libssl3 \
    libcurl4 \
    ca-certificates \
    curl \
    libsnappy1v5 \
    libldap-2.5-0 \
    libgssapi-krb5-2 \
    libsasl2-2 \
    zlib1g \
    libicu70 \
    libhiredis0.14 \
    libc-ares2 \
    libev4 \
    libnghttp2-14 \
    libcrypto++8 \
    libjemalloc2 \
    libre2-9 \
    libfmt8 \
    libzstd1 \
    libyaml-cpp0.7 \
    libboost-iostreams1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-program-options1.74.0 \
    libboost-stacktrace1.74.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /build/cmake-build/focusforge /app/focusforge
COPY --from=builder /usr/lib/x86_64-linux-gnu/libmongocrypt.so.0* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libcctz.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libbson-1.0.so* /usr/lib/x86_64-linux-gnu/
COPY configs/ /app/configs/

RUN groupadd -r focusforge \
    && useradd -r -g focusforge focusforge \
    && chown -R focusforge:focusforge /app

USER focusforge

EXPOSE 8080

HEALTHCHECK --interval=15s --timeout=5s --start-period=30s --retries=3 \
    CMD curl -f http://localhost:8080/health/live || exit 1

ENTRYPOINT ["/app/focusforge"]
CMD ["-c", "/app/configs/static_config.yaml", "--config_vars",\
    "/app/configs/config_vars.yaml"]