# Ubuntu 16.04 release
From ubuntu:16.04

USER root

RUN apt-get update &&\
		apt-get install -y gcc python3 python3-pip bash sudo git gnuplot\
		bison flex libreadline6 linux-tools-generic zlib1g-dev vim\
		linux-tools-`uname -r` build-essential libreadline-dev

ARG USER=vdriver
ENV USER ${USER}
ENV USER_HOME /home/${USER}
RUN useradd -ms /bin/bash ${USER} \
		&& echo "${USER}	ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /home/vdriver/vdriver

COPY --chown=${USER}:${USER} . .

RUN pip3 install -r requirements.txt


ENTRYPOINT ["su", "vdriver"]

