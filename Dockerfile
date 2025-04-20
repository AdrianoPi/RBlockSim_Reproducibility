FROM ubuntu:24.04

RUN apt-get update

RUN apt-get update && apt-get install -y \
make cmake build-essential \
python3 python3-pip \
git \
ssh openmpi-bin openmpi-common libopenmpi-dev

RUN export CC=gcc

RUN mkdir "/results_attacks/"
RUN mkdir "/results_performance/"
RUN mkdir "/results_figure_8/"
RUN mkdir "/plots/"

COPY ./RBlockSim/scripts/requirements.txt /requirements.txt

RUN pip3 install -r /requirements.txt --break-system-packages

COPY ./ /RBlockSim_repro/

WORKDIR /RBlockSim_repro/ROOT-Sim_core
RUN mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=RELEASE && make -j4 && make install

WORKDIR /RBlockSim_repro/ROOT-Sim_rng
RUN mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=RELEASE && make -j4 && make install

WORKDIR /RBlockSim_repro/

# ENTRYPOINT
ENTRYPOINT ["python3", "entrypoint.py"]
