#ifndef CONFIG_H
#define CONFIG_H

static const uint64 COIN = 100000000;
static const uint64 CENT = 1000000;

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 4000000;                      // 4000KB block hard limit
/** Obsolete: maximum size for mined blocks */
static const unsigned int MAX_BLOCK_SIZE_GEN = MAX_BLOCK_SIZE/4;         // 1000KB  block soft limit
/** Default for -blockmaxsize, maximum size for mined blocks **/
static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 1000000;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 1000000; //17000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** The maximum number of orphan transactions kept in memory */
static const unsigned int MAX_ORPHAN_TRANSACTIONS = MAX_BLOCK_SIZE/100;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB
/** Fake height value used in CCoins to signify they are only in the memory pool (since 0.8) */
static const unsigned int MEMPOOL_HEIGHT = 0x7FFFFFFF;
/** Dust Soft Limit, allowed with additional fee per output */
static const int64 DUST_SOFT_LIMIT = 100000; // 0.001 COOL
/** Dust Hard Limit, ignored as wallet inputs (mininput default) */
static const int64 DUST_HARD_LIMIT = 1000;   // 0.00001 COOL mininput
/** No amount larger than this (in satoshi) is valid */
static const int64 MAX_MONEY = 30000000000 * COIN;
inline bool MoneyRange(int64 nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 10;
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC
/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
#ifdef USE_UPNP
static const int fHaveUPnP = true;
#else
static const int fHaveUPnP = false;
#endif

/** coin value reward in mining */
static const int CONF_REWARD_COIN_VALUE = 10000000;
static const int CONF_NUMBER_BLOCK_HAS_REWARD = 3000;

/** Crytocurrency addresses start with character */
#define CONF_PUBKEY_ADDRESS 28 // 38-G; 28-C;
#define CONF_SCRIPT_ADDRESS 5
#define CONF_PUBKEY_ADDRESS_TEST 111
#define CONF_SCRIPT_ADDRESS_TEST 196

// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
static const char *strMainNetDNSSeed[][2] = {
    {"Server", "10.20.0.101"},
    {"Node1", "10.20.0.201"},
    {"Node2", "10.20.0.202"},
    {NULL, NULL}
};

static const char *strTestNetDNSSeed[][2] = {
    {"Server", "64.62.153.21"}
};

//Genesis block
#define  CONF_GENESIS_BLOCK "0xa24c6ffe1f9e1d3a70f3c16861c0111558cbe2c8ff7348787acfdcc08c113c38"
#define  CONF_GENESIS_BLOCK_TESTNET "0xe583a384c0517ab115379ca535b7af203436232b56c38573ba9e8840d17dffc1"
#define  CONF_BLOCK_HASH_MERKLE_ROOT "0x3b40bc8273afa3646b561e5ea318687902183c62bd7fc897f07e80a29d15b907"
#define  CONF_PSZTIMESTAMP "CoolCash 15/01/2015 The new future has begun"
#define  CONF_BLOCK_NVERSION 1
#define  CONF_BLOCK_NTIME    1421280000; //15/01/2015
#define  CONF_BLOCK_NBITS    0x1e0ffff0;
#define  CONF_BLOCK_NNONCE   2085383345;
#define  CONF_BLOCK_NTIME_TESTNET    1421270000;
#define  CONF_BLOCK_NNONCE_TESTNET   2084851830;

//Speed generate block
static const int64 nTargetTimespan = 0.25 * 24 * 60 * 60; // Coolcash: 0.25 days
static const int64 nTargetSpacing = 30; // Coolcash: 30s
static const int64 nInterval = nTargetTimespan / nTargetSpacing;

//Port for network p2p
#define  CONF_TESTNET_PORT 19109
#define  CONF_PORT 9109
#define  CONF_TESTNET_PORT_STRING "19109"
#define  CONF_PORT_STRING "9109"

#define  CONF_DEFAULT_DATA_DIR_WINDOW "Coolcash"
#define  CONF_DEFAULT_DATA_DIR_MAC_OSX "Coolcash"
#define  CONF_DEFAULT_DATA_DIR_UNIX ".coolcash"
#define  CONF_DEFAULT_CONFIG_FILE "coolcash.conf"
#define  CONF_DEFAULT_PIG_FILE "coolcash.pid"

#endif // CONFIG_H
