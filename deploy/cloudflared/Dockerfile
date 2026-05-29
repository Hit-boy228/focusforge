FROM alpine:3.19

RUN apk add --no-cache curl ca-certificates

# Download cloudflared for the correct architecture
ARG TARGETARCH=amd64
RUN ARCH="${TARGETARCH}" && \
    curl -fsSL -o /usr/local/bin/cloudflared \
    "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-${ARCH}" && \
    chmod +x /usr/local/bin/cloudflared && \
    cloudflared --version

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
