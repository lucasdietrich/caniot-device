# FROM sglahn/platformio-core:latest

# docker build -t devops/fedora-pio-toolchain -f scripts/Dockerfile .

FROM fedora:37

RUN dnf update -y && \
    dnf clean all && \
    dnf install -y python3 python3-pip git nano

RUN python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"

RUN /root/.platformio/penv/bin/pio pkg install -p platformio/atmelavr --global

ENV PATH="/root/.platformio/penv/bin:${PATH}"

ENTRYPOINT ["tail", "-f", "/dev/null"]