FROM debian:stretch

RUN dpkg --add-architecture i386 && \
  apt-get update && apt-get -y --no-install-recommends install \
    wget \
    ca-certificates \
    build-essential \
    libusb-dev \
    libusb-1.0-0-dev \
    libftdi1 \
    git \
    automake \
    autoconf \
    libtool \
    pkg-config \
    usbutils \
    libc6:i386

RUN mkdir -p /usr/local/bin/

# Install gcc-arm 5.4
RUN wget -q -O - https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2 | \
  tar -jx -C /usr/local/bin/

# Install OpenOCD from git
WORKDIR /tmp
RUN git clone https://github.com/ntfreak/openocd openocd && \
  cd openocd && \
  git checkout 06123153f38280608b1e92dcb766b31ade7e4668 # 2018-06-06

RUN cd openocd && \
  ./bootstrap && \
  ./configure --enable-buspirate && \
  make && \
  make install && \
  rm -rf /tmp/*
# OpenOCD GDB
EXPOSE 3333/tcp
# OpenOCD telnet
EXPOSE 4444/tcp

RUN mkdir /src
WORKDIR /src

CMD ["make"]
