FROM alpine:3.7 AS build

ARG ENCRYPTION=on
ARG REALDRIVERS=on
ARG PIGPIO_COMMIT=1737d477479f6009a4e1f3227480bfdb1a3653d0
ARG PIGPIO_SHA256_CHECKSUM=218db5bda5d58a4ecd9dc47027f82d51203de61d49b2eeaae29b0007f5ef31fd

COPY . /usr/src/PumpControl

RUN apk --update --no-cache --no-progress add \
        boost-dev \
        ca-certificates \
        curl \
        g++ \
        gcc \
        libc-dev \
        libc-dev \
        libressl-dev \
        make \
        unzip &&\
    `#----- build PiGPIO -----#` &&\
    mkdir -p /usr/src &&\
    curl -fSL "https://github.com/joan2937/pigpio/archive/${PIGPIO_COMMIT}.zip" -o /usr/src/pigpio.zip &&\
    echo "${PIGPIO_SHA256_CHECKSUM}  /usr/src/pigpio.zip" | sha256sum -c &&\
    unzip -d /usr/src /usr/src/pigpio.zip &&\
    cd /usr/src/pigpio-${PIGPIO_COMMIT} &&\
    `# syntax for ldconfig is different for Alpine than specified in the Makefile` &&\
    (make install ; ldconfig /) &&\
    `#----- build PumpControl -----#` &&\
    cd /usr/src/PumpControl &&\
    make ENCRYPTION=$ENCRYPTION REALDRIVERS=$REALDRIVERS &&\
    make install

#-------------------------------------------------------------------------------

FROM alpine:3.7

ARG CREATED
ARG REVISION

LABEL org.opencontainers.image.title="PumpControl" \
    org.opencontainers.image.version="0.1.0-1" \
    org.opencontainers.image.vendor="IUNO" \
    org.opencontainers.image.description="Decrypt and run recipe programs on the cocktail mixer" \
    org.opencontainers.image.licenses="GPL-3.0" \
    org.opencontainers.image.authors="Andre Lehmann <aisberg@posteo.de>" \
    org.opencontainers.image.source="https://github.com/IUNO-TDM/PumpControl.git" \
    org.opencontainers.image.revision="$REVISION" \
    org.opencontainers.image.created="$CREATED"

COPY --from=build /usr/bin/pumpcontrol /usr/bin/pumpcontrol
COPY --from=build /usr/local /usr
COPY pumpcontrol.settings.conf /etc/pumpcontrol/pumpcontrol.conf

RUN addgroup -S -g 99 pump_control &&\
	adduser -D -S -h /etc/pump-control -s /sbin/nologin -u 99 -G pump_control pump_control &&\
    apk --update --no-cache --no-progress add \
        bash \
        boost \
        boost-program_options \
        boost-random \
        boost-regex \
        boost-system \
        boost-thread \
        libressl \
        tini &&\
    mkdir -p /etc/pump-control &&\
    chown pump_control:pump_control /etc/pump-control

EXPOSE 9002/tcp

ENTRYPOINT ["/sbin/tini", "--", "/usr/bin/pumpcontrol", "--config", "/etc/pumpcontrol/pumpcontrol.conf"]
