FROM alpine:latest as base

RUN apk add --no-cache curl jq python3 py3-flask && \
    ARCH=$(uname -m) ; \
    if [ "$ARCH" = "aarch64" ]; then \
        CRANE_URL="https://github.com/google/go-containerregistry/releases/latest/download/go-containerregistry_Linux_arm64.tar.gz" ;\
    else \
        CRANE_URL="https://github.com/google/go-containerregistry/releases/latest/download/go-containerregistry_Linux_x86_64.tar.gz" ;\
    fi && \
    curl -sL $CRANE_URL | tar -zx -C /usr/local/bin crane

COPY ./config_server.py /
ENTRYPOINT ["python","/config_server.py"]
