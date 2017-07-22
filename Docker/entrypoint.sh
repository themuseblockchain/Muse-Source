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
	    echo
	  else
	    cp /genesis-test.json ${MUSE_DATADIR}
	fi
	exec mused -s ${TEST_SEED} \
	--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
	--genesis-json ${MUSE_DATADIR}/genesis-test.json \
	-d ${MUSE_DATADIR}/
fi

if [ "${NODE_TYPE}" = "prod" ]; then
	if [ -f "${MUSE_WORKDIR}/genesis.json" ]
	  then
	    echo
	  else
	    cp /genesis.json ${MUSE_DATADIR}
	fi
	exec mused -s ${PROD_SEED} \
	--replay-blockchain --rpc-endpoint=0.0.0.0:8090 \
	--genesis-json ${MUSE_DATADIR}/genesis.json \
	-d ${MUSE_DATADIR}/
fi



