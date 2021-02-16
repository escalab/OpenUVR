FROM nvidia/cuda:11.2.1-devel-ubuntu20.04 AS devel-base

ENV		NVIDIA_DRIVER_CAPABILITIES compute,utility,video
WORKDIR		/tmp/workdir

RUN	apt-get -yqq update && \
	apt-get install -yq --no-install-recommends build-essential sudo ca-certificates ssh expat libgomp1 && \
	apt-get autoremove -y && \
	apt-get clean -y

FROM nvidia/cuda:11.2.1-runtime-ubuntu20.04 AS runtime-base

ENV		NVIDIA_DRIVER_CAPABILITIES compute,utility,video
WORKDIR		/tmp/workdir

RUN	apt-get -yqq update && \
	apt-get install -yq --no-install-recommends build-essential sudo ca-certificates ssh expat libgomp1 libxcb-shape0-dev && \
	apt-get autoremove -y && \
	apt-get clean -y

FROM devel-base as build

ENV	DEBIAN_FRONTEND="noninteractive" \
	TZ="America/Los_Angeles"

RUN	buildDeps="autoconf \
		   automake \
		   curl \
		   ffmpeg \
		   g++ \
		   gcc \
		   git \
		   libssl-dev \
		   make \
		   openssl \
		   pkg-config \
		   yasm" && \
	apt-get -yqq update && \
	apt-get install -yq --no-install-recommends ${buildDeps}

COPY	. /OpenUVR
WORKDIR /OpenUVR/sending

RUN	mkdir /root/.ssh
COPY	id_rsa /root/.ssh/
RUN	chmod 600 /root/.ssh/id_rsa && \
	touch /root/.ssh/known_hosts && \
	ssh-keyscan github.com >> /root/.ssh/known_hosts && \
	git submodule init && git submodule update && \
	git apply /OpenUVR/sending/bgr0_ffmpeg.patch

## nv-codec
RUN \
	DIR=/tmp/nv-codec-headers && \
	mkdir -p ${DIR} && \
	cd ${DIR} && \
	git clone https://git.videolan.org/git/ffmpeg/nv-codec-headers.git ${DIR} && \
	make && \
	make install && \
	rm -rf ${DIR}

RUN	codecs="libass-dev \
		libfdk-aac-dev \
		libgles2-mesa-dev \
		libmp3lame-dev \
		libpulse-dev \
		libvorbis-dev \
		libx264-dev \
		libx265-dev \
		libtheora-dev" && \
	apt-get install -yq --no-install-recommends ${codecs}

RUN	make ffmpeg

RUN	git config --global core.compression 0 && \
	git clone --depth 1 git@github.com:EpicGames/UnrealTournament.git /UnrealTournament
WORKDIR	/UnrealTournament
RUN	git fetch --unshallow && \
	git pull --all && \
	apt-get install -yq --no-install-recommends mono-dmcs mono-mcs && \
	./Setup.sh && \
	./GenerateProjectFiles.sh && \
	make

CMD	["./UE4Editor"]
	
