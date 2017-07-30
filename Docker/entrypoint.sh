#!/bin/sh
cd ${WORKDIR}


if [ -f "${DATADIR}/config.ini" ]
  then
    echo
  else
    cp /config.ini ${DATADIR}
fi


if [ "${NODE_TYPE}" = "test" ]; then
	if [ -f "${DATADIR}/genesis-test.json" ]
	  then
	    echo "Starting muse daemon in TEST"
	  	exec mused -s ${TEST_SEED} \
		--rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${DATADIR}/genesis-test.json \
		-d ${DATADIR}/
	  else
	  	echo "Starting muse daemon in TEST.  Replaying blockchain"
	    cp /genesis-test.json ${DATADIR}
	    exec mused -s ${TEST_SEED} \
		--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${DATADIR}/genesis-test.json \
		-d ${DATADIR}/
	fi

fi

if [ "${NODE_TYPE}" = "prod" ]; then
	if [ -f "${DATADIR}/genesis.json" ]
	  then
	    echo "Starting daemon in PROD"
	    exec mused -s ${PROD_SEED} \
		--rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${DATADIR}/genesis.json \
		-d ${DATADIR}/
	  else
	  	echo "Starting daemon in PROD.  Replaying blockchain"
	    cp /genesis.json ${DATADIR}
		exec mused -s ${PROD_SEED} \
		--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
		--genesis-json ${DATADIR}/genesis.json \
		-d ${DATADIR}/
	fi

fi



