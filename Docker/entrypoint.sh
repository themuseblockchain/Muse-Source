#!/bin/sh
cd ${MUSE_WORKDIR}


if [ -f "${MUSE_DATADIR}/config.ini" ]
  then
    echo
  else
    cp /config.ini ${MUSE_DATADIR}
fi


if [ "${NODE_TYPE}" = "test" ]; then
	if [ -f "${MUSE_DATADIR}/genesis-test.json" ]
	  then
	    echo "Starting muse daemon in TEST"
	  	exec mused -s ${TEST_SEED} \
		--rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${MUSE_DATADIR}/genesis-test.json \
		-d ${MUSE_DATADIR}/
	  else
	  	echo "Starting muse daemon in TEST.  Replaying blockchain"
	    cp /genesis-test.json ${MUSE_DATADIR}
	    exec mused -s ${TEST_SEED} \
		--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${MUSE_DATADIR}/genesis-test.json \
		-d ${MUSE_DATADIR}/
	fi

fi

if [ "${NODE_TYPE}" = "prod" ]; then
	if [ -f "${MUSE_DATADIR}/genesis.json" ]
	  then
	    echo "Starting muse daemon in PROD"
	    exec mused -s ${PROD_SEED} \
		--rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${MUSE_DATADIR}/genesis.json \
		-d ${MUSE_DATADIR}/
	  else
	  	echo "Starting muse daemon in PROD.  Replaying blockchain"
	    cp /genesis.json ${MUSE_DATADIR}
		exec mused -s ${PROD_SEED} \
		--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${MUSE_DATADIR}/genesis.json \
		-d ${MUSE_DATADIR}/
	fi

fi



