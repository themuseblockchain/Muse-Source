#!/usr/bin/node

require("console");

//var MUSE_100_PCT = 100000;
var MUSE_100_PCT = 10000;

//var TOTAL_INFLATION = 1;
var TOTAL_INFLATION = .0475;

var MAX_MULT = Math.pow(2,64) / MUSE_100_PCT;

var BLOCK_TIME_SECS = 3;
var DAYS_PER_YEAR = 365;
var HOURS_PER_YEAR = 24 * DAYS_PER_YEAR;
var BLOCKS_PER_YEAR = HOURS_PER_YEAR * 3600 / BLOCK_TIME_SECS;
var ROUNDS_PER_YEAR = BLOCKS_PER_YEAR / 21;

var WANTED = {
    BLOCK: BLOCKS_PER_YEAR,
    ROUND: ROUNDS_PER_YEAR,
    HOUR:  HOURS_PER_YEAR,
    DAY:   DAYS_PER_YEAR
};

function calc_constants( steps ) {
    var multiplier = Math.expm1(Math.log1p(TOTAL_INFLATION) / steps) / MUSE_100_PCT;
    var shift = 0;
    while (multiplier < MAX_MULT) {
	shift++;
	multiplier *= 2;
    }
    shift--;
    multiplier /= 2;
    return [Math.round(multiplier + .5), shift];
}

Object.keys(WANTED).forEach(function (key) {
    var result = calc_constants( WANTED[key] );
    //console.log("// " + key + ": " + WANTED[key] + " - " + result[0] + " >> " + result[1]);
    console.log("#define MUSE_APR_PERCENT_MULTIPLY_PER_" + key + " (0x" + result[0].toString(16) + "ULL)");
    console.log("#define MUSE_APR_PERCENT_SHIFT_PER_" + key + " " + result[1]);
});
