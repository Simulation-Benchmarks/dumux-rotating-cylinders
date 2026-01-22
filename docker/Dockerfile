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

# create a dumux user
# make the set permission helper script executable
# add user to video group which enables graphics if desired
RUN useradd -m --home-dir /dumux dumux \
    && chmod +x /etc/my_init.d/setpermissions.sh \
    && usermod -a -G video dumux

# switch to the dumux user and set the working directory
USER dumux
WORKDIR /dumux

# create a shared volume communicating with the host
RUN mkdir /dumux/shared
VOLUME /dumux/shared

# set git user in case installation requires to apply patches
RUN git config --global user.name "rotatingCylinders"
RUN git config --global user.email "sarbani.roy@simtech.uni-stuttgart.de"

# Install the dumux module and its dependencies
# This expects the install script to do everything from clone to configure
COPY install_rotating-cylinders.sh /dumux/install_rotating-cylinders.sh
RUN bash /dumux/install_rotating-cylinders.sh \
    && rm -f /dumux/install_rotating-cylinders.sh

RUN cd /dumux/rotating-cylinders/build-cmake/test/freeflow/navierstokes/rotatingcylinders
        
# Build rotating cylinders executable
RUN cd /dumux/rotating-cylinders/build-cmake/test/freeflow/navierstokes/rotatingcylinders \
    && make test_ff_navierstokes_rotatingcylinders

# unset git user
RUN git config --global --unset user.name
RUN git config --global --unset user.email

# switch back to root
WORKDIR /dumux
USER root

# set entry point like advised https://github.com/phusion/baseimage-docker
# this sets the permissions right, see above
ENTRYPOINT ["/sbin/my_init","--","/sbin/setuser","dumux","/bin/bash","-l","-c"]

# Starts executing the simulation
CMD ["/dumux/run_simulation.sh"]