# SPDX-FileCopyrightText: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
# SPDX-License-Identifier: GPL-3.0-or-later

# rotatingCylinders docker container
# see https://github.com/phusion/baseimage-docker for information on the base image
# It is Ubuntu LTS customized for better Docker compatibility
FROM phusion/baseimage:jammy-1.0.4

# run Ubuntu update as advised on https://github.com/phusion/baseimage-docker
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade -y -o Dpkg::Options::="--force-confold" \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# install the basic dependencies
RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
    ca-certificates \
    vim \
    python3-dev \
    python3-pip \
    git \
    pkg-config \
    cmake \
    build-essential \
    gfortran \
    mpi-default-bin \
    mpi-default-dev \
    libsuitesparse-dev \
    libsuperlu-dev \
    libeigen3-dev \
    doxygen \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# add the permission helper script to the my_init service
COPY setpermissions.sh /etc/my_init.d/setpermissions.sh
RUN chmod +x /etc/my_init.d/setpermissions.sh

# create a dumux user
# add user to video group which enables graphics if desired
RUN useradd -m --home-dir /dumux dumux \
    && usermod -a -G video dumux

# Switch to dumux user
USER dumux
WORKDIR /dumux

# create a shared volume communicating with the host
RUN mkdir /dumux/shared
VOLUME /dumux/shared

# Container-only writable folder
RUN mkdir -p /dumux/work
RUN chown -R dumux:dumux /dumux/work

# ---------------------------------------------
# Configure git for installation
# ---------------------------------------------
RUN git config --global user.name "rotatingCylinders" \
    && git config --global user.email "sarbani.roy@simtech.uni-stuttgart.de"

# Install modules using installation script
COPY install_rotating-cylinders.sh /dumux/install_rotating-cylinders.sh
RUN bash /dumux/install_rotating-cylinders.sh \
    && rm -f /dumux/install_rotating-cylinders.sh

# Unset git user
RUN git config --global --unset user.name \
    && git config --global --unset user.email

# ---------------------------------------------
# Switch back to root
# ---------------------------------------------
WORKDIR /dumux
USER root

# ---------------------------------------------
# Entry point
# ---------------------------------------------
ENTRYPOINT ["/sbin/my_init","--","/sbin/setuser","dumux","/bin/bash","-l","-c"]
CMD ["/bin/bash"]
