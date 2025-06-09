FROM debian:sid

# Install system dependencies. debian sid needed for boost 1.83
# boost 1.83 is needed for test case serialization compatibility
# the project binaries will likely work with debian:12 defaults.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    git \
    gdb \
    wget \
    curl \
    ccrypt \
    pkg-config \
    python3 \
    python3-pip \
    libboost1.83-all-dev \
    libssl-dev \
    libzstd-dev \
    libfmt-dev \
    doxygen \
    graphviz \
    locales \
    locales-all \
    clang \
    clangd \
    vim \
    && rm -rf /var/lib/apt/lists/*

# Set locale (for Boost, etc.)
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Optionally set up a non-root user for VS Code devcontainers best practice
RUN useradd -ms /bin/bash devuser
USER devuser
WORKDIR /workspace
RUN git clone https://github.com/algoratio/bentoclient.git bentoclient/src
RUN cd bentoclient \
    && mkdir build \
    && mkdir install
RUN cd bentoclient/build \
    && cp ../src/docker/run_me_to_build_in_container.sh \
    && chmod +x run_me_to_build_in_container.sh
WORKDIR /workspace/bentoclient/build
CMD [ "bash" ]
                                                              46,1          96%

