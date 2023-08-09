FROM fedora:37

RUN dnf update -y && \
    dnf clean all && \
    dnf install -y python3 python3-pip git \
        avr-gcc avr-gcc-c++ avr-libc avr-binutils

ENV PIO_VERSION="6.1.9"
ENV PIO_ATMELAVR_VERSION="4.2.0"
ENV PIO_FRAMEWORK_ARDUINO_AVR="5.1.0"
ENV PIO_FRAMEWORK_ARDUINO_AVR_MINICORE="2.2.2"

ENV PLATFORMIO_CORE_DIR="/home/user/.platformio"

ARG USER_ID=984
ARG GROUP_ID=983

RUN groupadd -r user -g $GROUP_ID && \
    useradd -g $GROUP_ID -u $USER_ID user
USER user
WORKDIR /home/user

RUN python3 -m venv .venv && \
    . .venv/bin/activate && \
    pip install --upgrade pip && \
    pip install platformio==${PIO_VERSION}

ENV PATH="/home/user/.venv/bin:${PATH}"

RUN . .venv/bin/activate && \
    pio pkg install -g -p "platformio/atmelavr@${PIO_ATMELAVR_VERSION}" &&\
    pio pkg install -g -t "platformio/framework-arduino-avr@${PIO_FRAMEWORK_ARDUINO_AVR}" &&\
    pio pkg install -g -t "platformio/framework-arduino-avr-minicore@${PIO_FRAMEWORK_ARDUINO_AVR_MINICORE}" &&\
    pio pkg install -g -t "platformio/tool-scons@4.40502.0"

ENTRYPOINT ["/home/user/.venv/bin/pio"]