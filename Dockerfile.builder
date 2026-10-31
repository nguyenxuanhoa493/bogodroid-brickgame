# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2025-2026 jenny92-tech <jennyliu90223@gmail.com>
FROM debian:11

ENV DEBIAN_FRONTEND=noninteractive

# Enable arm64 multiarch — cross-compile builds link against :arm64 dev libs.
# Debian 11 (bullseye) ships glibc 2.31, so artifacts produced here run on
# any device with glibc >= 2.31 (Ubuntu 20.04+, Debian 11+, SteamOS, Raspberry
# Pi OS Bookworm, RHEL 9, and virtually all maintained distros).
# Debian 11 reached EOL (2026-08-31): deb.debian.org drops the bullseye suites.
# Pin the archive mirrors and skip Valid-Until (archive Release files are expired).
RUN printf '%s\n' \
      'deb http://archive.debian.org/debian bullseye main' \
      'deb http://archive.debian.org/debian bullseye-updates main' \
      'deb http://archive.debian.org/debian-security bullseye-security main' \
      > /etc/apt/sources.list \
 && rm -f /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources \
 && dpkg --add-architecture arm64 \
 && apt-get -o Acquire::Check-Valid-Until=false -o Acquire::Retries=3 update -qq \
 && apt-get install -y -qq --no-install-recommends \
      cmake ninja-build pkg-config g++ \
      ca-certificates \
      crossbuild-essential-arm64 \
      libsdl2-dev libgles2-mesa-dev libegl1-mesa-dev \
      libcurl4-openssl-dev libsecret-1-dev \
      libbsd-dev libmd-dev \
      libsdl2-dev:arm64 libgles2-mesa-dev:arm64 libegl1-mesa-dev:arm64 \
      libcurl4-openssl-dev:arm64 libsecret-1-dev:arm64 \
      libbsd-dev:arm64 libmd-dev:arm64 \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /work
