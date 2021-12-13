FROM ubuntu:21.10

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -y \
    && apt-get install -y build-essential  manpages-dev gdb
