# ==============================================================================
# FocusForge — Dev Container Dockerfile
# Базируется на userver development image с полным toolchain
# ==============================================================================

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

ARG DEBIAN_FRONTEND=noninteractive

# ── Системные зависимости разработки ─────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    gnupg \
    # Build tools
    ninja-build \
    ccache \
    gdb \
    valgrind \
    # Clang toolchain (для format/tidy)
    clang \
    clang-format \
    clang-tidy \
    lldb \
    # Clang toolchain
    clang \
    clangd \
    clang-format \
    clang-tidy \
    lldb \
    # DB CLI tools
    postgresql-client \
    # Redis CLI
    redis-tools \
    # Python (для тестов)
    python3 \
    python3-pip \
    python3-venv \
    # Утилиты
    jq \
    tree \
    htop \
    wget \
    make \
    git \
    && curl -fsSL https://www.mongodb.org/static/pgp/server-7.0.asc \
    | gpg --dearmor -o /usr/share/keyrings/mongodb-server-7.0.gpg \
    && echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-7.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" \
    > /etc/apt/sources.list.d/mongodb-org-7.0.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends mongodb-mongosh \
    && rm -rf /var/lib/apt/lists/*

# ── Conan (менеджер пакетов C++) ──────────────────────────────────────────────
RUN pip3 install conan==2.0.17 --quiet

# ── Python test dependencies ──────────────────────────────────────────────────
RUN pip3 install httpx pytest pytest-asyncio --quiet

# ── Настройка пользователя (vscode user для Dev Containers) ──────────────────
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid $USER_GID $USERNAME 2>/dev/null || true \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME 2>/dev/null || true \
    && apt-get update \
    && apt-get install -y --no-install-recommends sudo \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" >> /etc/sudoers \
    && rm -rf /var/lib/apt/lists/*

# ── Настройка ccache ──────────────────────────────────────────────────────────
ENV PATH="/usr/lib/ccache:$PATH"
ENV CCACHE_DIR=/home/$USERNAME/.ccache

RUN mkdir -p /home/$USERNAME/.ccache \
    && chown $USER_UID:$USER_GID /home/$USERNAME/.ccache

# ── Рабочий каталог ───────────────────────────────────────────────────────────
WORKDIR /workspace

# Переключаемся на пользователя разработчика
USER $USERNAME

# ── Инициализация Conan для пользователя ──────────────────────────────────────
RUN conan profile detect --force 2>/dev/null || true

# Запускаем shell
CMD ["/bin/bash"]

RUN ln -s /usr/bin/clangd /usr/bin/clangd-17 || true


