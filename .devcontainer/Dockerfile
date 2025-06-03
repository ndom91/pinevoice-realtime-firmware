FROM ubuntu:22.04

ARG USERNAME=vscode
ARG UID=1000
ARG GID=1000

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    sudo build-essential git clang curl ca-certificates tio

RUN mkdir -p /opt/toolchain \
  && curl -fsSL http://files.pine64.org/tools/bouffalo/bl606p_toolchain.tar.gz \
       | tar xz -C /opt/toolchain

ENV PATH=/opt/toolchain/bin/:$PATH
#ENV PATH=/opt/toolchain/riscv64-unknown-elf/bin/:$PATH

RUN apt-get install -y --no-install-recommends \
    bash python3 python3-pip python-is-python3 make tar git xxd bear \
  && pip install yoctools==2.0.40 -U && pip install intelhex prettytable DecryptLogin selenium disutils scons \
  && groupadd --gid ${GID} ${USERNAME} \
  && useradd --uid ${UID} --gid ${GID} -m ${USERNAME} \
  && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
  && chmod 0440 /etc/sudoers.d/${USERNAME}


USER ${USERNAME}

WORKDIR /workspace

ENV SHELL /bin/bash