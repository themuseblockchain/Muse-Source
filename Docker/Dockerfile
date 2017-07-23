FROM phusion/baseimage:0.9.19
MAINTAINER educatedwarrior 

# Configuration variables
#test or prod for NODE_TYPE
ENV NODE_TYPE=test
ENV LANG=en_US.UTF-8
ENV MUSE_WORKDIR /opt/muse/bin
ENV MUSE_DATADIR /opt/muse/bin/witness_node_data_dir
ENV TEST_SEED = 192.34.60.157:29092
ENV PROD_SEED = 138.197.68.175:29092

LABEL org.freenas.interactive="false"       \
      org.freenas.version="1.0.0.0000"      \
      org.freenas.upgradeable="true"        \
      org.freenas.expose-ports-at-host="true"   \
      org.freenas.autostart="true"      \
      org.freenas.web-ui-protocol="http"    \
      org.freenas.web-ui-port=8090     \
      org.freenas.web-ui-path=""     \
      org.freenas.port-mappings="8090:80900/tcp"         \
      org.freenas.volumes="[                    \
          {                         \
              \"name\": \"${MUSE_DATADIR}\",              \
              \"descr\": \"Data directory\"           \
          }                         \
      ]"                            \
      org.freenas.settings="[                   \
          {                         \
              \"env\": \"NODE_TYPE\",                  \
              \"descr\": \"test or prod\",       \
              \"optional\": false                \
          },                            \
          {                         \
              \"env\": \"TEST_SEED\",                \
              \"descr\": \"<testseedIPAddress>:29092\",          \
              \"optional\": true                \
          },                            \
          {                         \
              \"env\": \"PROD_SEED\",                \
              \"descr\": \"<prodseedIPAddress>:29092\",         \
              \"optional\": true                \
          }                            \
      ]"

RUN \
    apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        build-essential \
        cmake \
        doxygen \
        libboost-all-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        ncurses-dev \
        gcc-4.9 g++-4.9 \
        libbz2-dev libdb++-dev libdb-dev \
        python3 \
        python3-dev \
        python3-jinja2 \
        python3-pip \
        wget \
        screen \
        git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* 


#Install required version of boost
RUN \
	cd /tmp && wget https://cytranet.dl.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.bz2 && \
	tar xvf boost_1_59_0.tar.bz2 && \
	cd boost_1_59_0 && \
	./bootstrap.sh --prefix=/usr/local --with-libraries=all --libdir=/usr/local/lib --includedir=/usr/local/include && \
	./b2 install && \
	rm -rf /tmp/boost_1_59_0*


#Manually add source code until docker code ready
#ADD . /usr/local/src/muse
RUN \
	cd /tmp && git clone https://github.com/themuseblockchain/Muse-Source.git && \
	cd Muse-Source && \
	git submodule update --init --recursive && \
	cmake -DBOOST_ROOT="$BOOST_ROOT" -DBUILD_MUSE_TEST=ON -DCMAKE_BUILD_TYPE=Debug . && \
	make mused cli_wallet

# Make binary builds available for general-system wide use 
RUN \
	cp ./programs/mused/mused /usr/bin/mused && \
	cp ./programs/cli_wallet/cli_wallet /usr/bin/cli_wallet && \

RUN mkdir -p ${MUSE_DATADIR}
COPY config.ini genesis-test.json genesis.json /
COPY entrypoint.sh /sbin
RUN cd ${MUSE_WORKDIR} && chmod +x /sbin/entrypoint.sh
VOLUME ${MUSE_DATADIR}
EXPOSE 8090
ENTRYPOINT ["/sbin/entrypoint.sh"]
#Clean up
#RUN \
#    apt-get clean && \
#    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* 

