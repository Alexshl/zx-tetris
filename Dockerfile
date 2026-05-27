# z88dk toolchain for building ZX Spectrum Tetris.
# Builds z88dk once at image build time; subsequent project builds reuse the layer.

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        bison \
        flex \
        libgmp-dev \
        libboost-dev \
        libxml2-dev \
        libpng-dev \
        zlib1g-dev \
        m4 \
        git \
        pkg-config \
        ca-certificates \
        dos2unix \
    && rm -rf /var/lib/apt/lists/*

ENV Z88DK=/opt/z88dk
ENV PATH=$Z88DK/bin:$PATH
ENV ZCCCFG=$Z88DK/lib/config

RUN git clone --recursive https://github.com/z88dk/z88dk.git $Z88DK \
    && cd $Z88DK \
    && ./build.sh \
    && find $Z88DK -name '*.o' -delete

WORKDIR /app

CMD ["make"]
