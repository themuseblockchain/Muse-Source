FROM educatedwarrior/invictus_image:1.59
MAINTAINER educatedwarrior 

# Configuration variables
#test or prod for NODE_TYPE
ENV NODE_TYPE prod
ENV LANG en_US.UTF-8
ENV REPOLINK https://github.com/educatedwarrior/Muse-Source.git
ENV REPOBRANCH master
ENV WORKDIR /opt/muse/bin
ENV DATADIR /opt/muse/bin/witness_node_data_dir
ENV USEGENSISJSON false 
ENV TEST_SEED 192.34.60.157:29092
ENV PROD_SEED 138.197.68.175:33333

LABEL org.freenas.interactive="false"       \
      org.freenas.version="1.59.1.0002"      \
      org.freenas.upgradeable="true"        \
      org.freenas.expose-ports-at-host="true"   \
      org.freenas.autostart="true"      \
      org.freenas.web-ui-protocol="http"    \
      org.freenas.web-ui-port=8090     \
      org.freenas.web-ui-path=""     \
      org.freenas.port-mappings="8090:8090/tcp"         \
      org.freenas.volumes="[                    \
          {                         \
              \"name\": \"${DATADIR}\",              \
              \"descr\": \"Data directory\"           \
          }                         \
      ]"                            \
      org.freenas.settings="[                   \
          {                         \
              \"env\": \"NODE_TYPE\",                  \
              \"descr\": \"test or prod.  Default value test\",       \
              \"optional\": false                \
          },                            \
          {                         \
              \"env\": \"TEST_SEED\",                \
              \"descr\": \"Default value 192.34.60.157:29092\",          \
              \"optional\": true                \
          },                            \
          {                         \
              \"env\": \"PROD_SEED\",                \
              \"descr\": \"Default value 138.197.68.175:29092\",         \
              \"optional\": true                \
          }                            \
      ]"

#Build blockchain source
RUN \
	cd /tmp && git clone "$REPOLINK" && \
	cd Muse-Source && \
	git submodule update --init --recursive && \
	cmake -j 8 -DBOOST_ROOT="$BOOST_ROOT" -DBUILD_MUSE_TEST=OFF -DCMAKE_BUILD_TYPE=Debug . && \
	make mused cli_wallet

# Make binary builds available for general-system wide use 
RUN \
	cp /tmp/Muse-Source/programs/mused/mused /usr/bin/mused && \
	cp /tmp/Muse-Source/programs/cli_wallet/cli_wallet /usr/bin/cli_wallet

RUN mkdir -p "$DATADIR"
RUN touch genesis-test.json
RUN touch genesis.json
COPY /Docker/config.ini genesis-test.json /
COPY /Docker/entrypoint.sh /sbin
RUN cd "$WORKDIR" && chmod +x /sbin/entrypoint.sh
VOLUME "$DATADIR"
EXPOSE 8090
CMD ["/sbin/entrypoint.sh"]
