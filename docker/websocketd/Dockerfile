FROM alpine:3
ADD . /websocketd
RUN    apk update && apk add --no-cache unzip curl python3 py3-pip py3-setuptools py3-wheel py3-redis\
    && apk add --upgrade libcrypto3 libssl3 \
    && cp /websocketd/zoo-project/zoo-services/utils/open-api/server/subscriber.py /shell.py\
    && if [ ! -e /usr/bin/python ]; then ln -sf python3 /usr/bin/python ; fi \
    && if [ ! -e /usr/bin/pip ]; then ln -s pip3 /usr/bin/pip ; fi \
    && wget -P /tmp/ https://github.com/joewalnes/websocketd/releases/download/v0.3.1/websocketd-0.3.1-linux_amd64.zip \
    && unzip -o -d /websocketd/ /tmp/websocketd-0.3.1-linux_amd64.zip
WORKDIR /websocketd
ENTRYPOINT [ "/websocketd/websocketd" ]
CMD [ "--port=8888", "/shell.py" ]
