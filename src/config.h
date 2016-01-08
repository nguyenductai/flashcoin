#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

//amount.h
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

/** No amount larger than this (in satoshi) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Bitcoin
 * currently happens to be less than 21,000,000 BTC for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static const CAmount MAX_MONEY = 30000000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }


//consensus.h
/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 4000000;                      // 4000KB block hard limit
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 10;

//policy.h
/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 1000000; // 750000;
static const unsigned int DEFAULT_BLOCK_MIN_SIZE = 0;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 1000000; //17000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
/** Maximum number of signature check operations in an IsStandard() P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_STANDARD_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;
/** Default for -maxmempool, maximum megabytes of mempool memory usage */
static const unsigned int DEFAULT_MAX_MEMPOOL_SIZE = 300;

//main.h
/** Default for accepting alerts from the P2P network. */
static const bool DEFAULT_ALERTS = true;
/** Default for DEFAULT_WHITELISTALWAYSRELAY. */
static const bool DEFAULT_WHITELISTALWAYSRELAY = true;
/** Default for -minrelaytxfee, minimum relay fee for transactions */
static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000;
/** Default for -maxorphantx, maximum number of orphan transactions kept in memory */
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100;
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_LIMIT = 25;
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT = 101;
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_LIMIT = 25;
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT = 101;
/** Default for -mempoolexpiry, expiration time for mempool transactions in hours */
static const unsigned int DEFAULT_MEMPOOL_EXPIRY = 72;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB

/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC

/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached its tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Time to wait (in seconds) between writing blocks/block index to disk. */
static const unsigned int DATABASE_WRITE_INTERVAL = 60 * 60;
/** Time to wait (in seconds) between flushing chainstate to disk. */
static const unsigned int DATABASE_FLUSH_INTERVAL = 24 * 60 * 60;
/** Maximum length of reject messages. */
static const unsigned int MAX_REJECT_MESSAGE_LENGTH = 111;
static const unsigned int DEFAULT_LIMITFREERELAY = 15;
static const bool DEFAULT_RELAYPRIORITY = true;

/** Default for -permitbaremultisig */
static const bool DEFAULT_PERMIT_BAREMULTISIG = true;
static const bool DEFAULT_CHECKPOINTS_ENABLED = true;
static const bool DEFAULT_TXINDEX = false;
static const unsigned int DEFAULT_BANSCORE_THRESHOLD = 100;

static const bool DEFAULT_TESTSAFEMODE = false;

/** Maximum number of headers to announce when relaying blocks with headers message.*/
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;

/** coin value reward in mining */
static const int CONF_REWARD_COIN_VALUE = 10000000;
static const int CONF_NUMBER_BLOCK_HAS_REWARD = 3000;

//end main.h

//net.h

/** Time between pings automatically sent out for latency probing and keepalive (in seconds). */
static const int PING_INTERVAL = 2 * 60;
/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static const int TIMEOUT_INTERVAL = 20 * 60;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** The maximum number of new addresses to accumulate before announcing. */
static const unsigned int MAX_ADDR_TO_SEND = 1000;
/** Maximum length of incoming protocol messages (no message over 2 MiB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 2 * 1024 * 1024;
/** Maximum length of strSubVer in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** -upnp default */
#ifdef USE_UPNP
static const bool DEFAULT_UPNP = USE_UPNP;
#else
static const bool DEFAULT_UPNP = false;
#endif
/** The maximum number of entries in mapAskFor */
static const unsigned int MAPASKFOR_MAX_SZ = MAX_INV_SZ;
/** The maximum number of entries in setAskFor (larger due to getdata latency)*/
static const unsigned int SETASKFOR_MAX_SZ = 2 * MAX_INV_SZ;
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
/** The default for -maxuploadtarget. 0 = Unlimited */
static const uint64_t DEFAULT_MAX_UPLOAD_TARGET = 0;
/** Default for blocks only*/
static const bool DEFAULT_BLOCKSONLY = false;

static const bool DEFAULT_FORCEDNSSEED = false;
static const unsigned int DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const unsigned int DEFAULT_MAXSENDBUFFER    = 1 * 1000;

// NOTE: When adjusting this, update rpcnet:setban's help ("24h")
static const unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24;  // Default 24-hour ban

//txmempool.h
/** Fake height value used in CCoins to signify they are only in the memory pool (since 0.8) */
static const unsigned int MEMPOOL_HEIGHT = 0x7FFFFFFF;


/** Crytocurrency addresses start with character */
#define CONF_PUBKEY_ADDRESS 28 // 38-G; 28-C;
#define CONF_SCRIPT_ADDRESS 5
#define CONF_SECRET_KEY 156

#define CONF_PUBKEY_ADDRESS_TEST 111
#define CONF_SCRIPT_ADDRESS_TEST 196
#define CONF_SECRET_KEY_TEST 239

//Genesis block
#define  CONF_GENESIS_BLOCK "0xa24c6ffe1f9e1d3a70f3c16861c0111558cbe2c8ff7348787acfdcc08c113c38"
#define  CONF_GENESIS_BLOCK_TESTNET "0xe583a384c0517ab115379ca535b7af203436232b56c38573ba9e8840d17dffc1"
#define  CONF_BLOCK_HASH_MERKLE_ROOT "0x3b40bc8273afa3646b561e5ea318687902183c62bd7fc897f07e80a29d15b907"
#define  CONF_PSZTIMESTAMP "CoolCash 15/01/2015 The new future has begun"
#define  CONF_SCRIPT_PUBKEY "040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9"
static const uint32_t CONF_BLOCK_NVERSION = 1;
static const uint32_t CONF_BLOCK_NTIME = 1421280000; //15/01/2015
static const uint32_t CONF_BLOCK_NBITS = 0x1e0ffff0;
static const uint32_t CONF_BLOCK_NNONCE = 2085383345;
static const uint32_t CONF_BLOCK_NTIME_TESTNET = 1421270000;
static const uint32_t CONF_BLOCK_NNONCE_TESTNET = 2084851830;

//Speed generate block
static const int64_t def_nTargetTimespan = 0.25 * 24 * 60 * 60; // Coolcash: 0.25 days
static const int64_t def_nTargetSpacing = 30; // Coolcash: 30s
static const int64_t nInterval = def_nTargetTimespan / def_nTargetSpacing;

//Port for network p2p
#define  CONF_TESTNET_PORT 19109
#define  CONF_PORT 9109
#define  CONF_TESTNET_PORT_STRING "19109"
#define  CONF_PORT_STRING "9109"

#define  CONF_DEFAULT_CLENT_NAME "Coolcash Desktop Wallet"
#define  CONF_DEFAULT_DATA_DIR_WINDOW "Coolcash"
#define  CONF_DEFAULT_DATA_DIR_MAC_OSX "Coolcash"
#define  CONF_DEFAULT_DATA_DIR_UNIX ".coolcash"

//#define  CONF_DEFAULT_CONFIG_FILE "coolcash.conf"
//#define  CONF_DEFAULT_PIG_FILE "coolcash.pid"

const char * const BITCOIN_CONF_FILENAME = "coolcash.conf";
const char * const BITCOIN_PID_FILENAME = "coolcash.pid";


#endif // CONFIG_H
