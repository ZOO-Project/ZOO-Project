FROM alpine:3.11
ADD . /websocketd
RUN    apk update && apk add --no-cache unzip curl python3 \
    && if [ ! -e /usr/bin/python ]; then ln -sf python3 /usr/bin/python ; fi \
    \
    && echo "**** install pip ****" \
    && python3 -m ensurepip \
    && rm -r /usr/lib/python*/ensurepip \
    && pip3 install --no-cache --upgrade pip setuptools wheel redis \
    && if [ ! -e /usr/bin/pip ]; then ln -s pip3 /usr/bin/pip ; fi \
    && wget -P /tmp/ https://github.com/joewalnes/websocketd/releases/download/v0.3.1/websocketd-0.3.1-linux_amd64.zip \
    && unzip -o -d /websocketd/ /tmp/websocketd-0.3.1-linux_amd64.zip
WORKDIR /websocketd
ENTRYPOINT [ "/websocketd/websocketd" ]
CMD [ "--port=8888", "/websocketd/shell.sh" ]