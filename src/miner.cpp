// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "hash.h"
#include "crypto/scrypt.h"
#include "main.h"
#include "net.h"
#include "pow.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.
// The COrphan class keeps track of these 'temporary orphans' while
// CreateBlock is figuring out which transactions to include.
//
class COrphan
{
public:
    const CTransaction* ptx;
    set<uint256> setDependsOn;
    CFeeRate feeRate;
    double dPriority;

    COrphan(const CTransaction* ptxIn) : ptx(ptxIn), feeRate(0), dPriority(0)
    {
    }
};

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

// We want to sort transactions by priority and fee rate, so:
typedef boost::tuple<double, CFeeRate, const CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;

public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }

    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

void UpdateTime(CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    pblock->nTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    // Updating time can change work required on testnet:
    if (Params().AllowMinDifficultyBlocks())
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock);
}

CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn)
{
    // Create new block

    int64_t start_time = GetTimeMillis(); //WL added

    auto_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if(!pblocktemplate.get())
        return NULL;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (Params().MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

    // Create coinbase tx
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);
    txNew.vout[0].scriptPubKey = scriptPubKeyIn;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SIZE-1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    CAmount nFees = 0;

    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        CCoinsViewCache view(pcoinsTip);

        // Priority order to process transactions
        list<COrphan> vOrphan; // list memory doesn't move
        map<uint256, vector<COrphan*> > mapDependers;
        bool fPrintPriority = GetBoolArg("-printpriority", false);

        // This vector will be sorted into a priority queue:

        int64_t start_time_2 = GetTimeMillis();

//        vector<TxPriority> vecPriority;
//        vecPriority.reserve(mempool.mapTx.size());
//        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin();
//             mi != mempool.mapTx.end(); ++mi)
//        {
//            const CTransaction& tx = mi->second.GetTx();
//            if (tx.IsCoinBase() || !IsFinalTx(tx, nHeight))
//                continue;

//            COrphan* porphan = NULL;
//            double dPriority = 0;
//            CAmount nTotalIn = 0;
//            bool fMissingInputs = false;
//            BOOST_FOREACH(const CTxIn& txin, tx.vin)
//            {
//                // Read prev transaction
//                if (!view.HaveCoins(txin.prevout.hash))
//                {
//                    // This should never happen; all transactions in the memory
//                    // pool should connect to either transactions in the chain
//                    // or other transactions in the memory pool.
//                    if (!mempool.mapTx.count(txin.prevout.hash))
//                    {
//                        LogPrintf("ERROR: mempool transaction missing input\n");
//                        if (fDebug) assert("mempool transaction missing input" == 0);
//                        fMissingInputs = true;
//                        if (porphan)
//                            vOrphan.pop_back();
//                        break;
//                    }

//                    // Has to wait for dependencies
//                    if (!porphan)
//                    {
//                        // Use list for automatic deletion
//                        vOrphan.push_back(COrphan(&tx));
//                        porphan = &vOrphan.back();
//                    }
//                    mapDependers[txin.prevout.hash].push_back(porphan);
//                    porphan->setDependsOn.insert(txin.prevout.hash);
//                    nTotalIn += mempool.mapTx[txin.prevout.hash].GetTx().vout[txin.prevout.n].nValue;
//                    continue;
//                }
//                const CCoins* coins = view.AccessCoins(txin.prevout.hash);
//                assert(coins);

//                CAmount nValueIn = coins->vout[txin.prevout.n].nValue;
//                nTotalIn += nValueIn;

//                int nConf = nHeight - coins->nHeight;

//                dPriority += (double)nValueIn * nConf;
//            }
//            if (fMissingInputs) continue;

//            // Priority is sum(valuein * age) / modified_txsize
//            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
//            dPriority = tx.ComputePriority(dPriority, nTxSize);

//            uint256 hash = tx.GetHash();
//            mempool.ApplyDeltas(hash, dPriority, nTotalIn);

//            CFeeRate feeRate(nTotalIn-tx.GetValueOut(), nTxSize);

//            if (porphan)
//            {
//                porphan->dPriority = dPriority;
//                porphan->feeRate = feeRate;
//            }
//            else
//                vecPriority.push_back(TxPriority(dPriority, feeRate, &mi->second.GetTx()));
//        }

        start_time_2 = GetTimeMillis() - start_time_2; //wl added
        LogPrintf("CreateNewBlock() - Priority order to process transactions - Duration: %u\n", start_time_2); // WL added

        start_time_2 = GetTimeMillis();

        // Collect transactions into block
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;

        list<CTransaction> listSpentTx; listSpentTx.clear();
        list<CTransaction> spentTxRemoved;
        list<CTransaction>::iterator it;

        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin();
             mi != mempool.mapTx.end(); ++mi)
        {
            const CTransaction& tx = mi->second.GetTx();
            if (tx.IsCoinBase() || !IsFinalTx(tx, nHeight))
                continue;

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nTxSigOps = GetLegacySigOpCount(tx);
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            if (!view.HaveInputs(tx)){
                // WL Add spent tx to List for removing afterward
                listSpentTx.push_back(tx);
                continue;
            }


            CAmount nTxFees = view.GetValueIn(tx)-tx.GetValueOut();

            nTxSigOps += GetP2SHSigOpCount(tx, view);
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            // Note that flags: we don't want to set mempool/IsStandard()
            // policy here, but we still have to ensure that the block we
            // create only contains transactions that are valid in new blocks.
//            CValidationState state;
//            if (!CheckInputs(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS, true))
//                continue;

//            CTxUndo txundo;
//            UpdateCoins(tx, state, view, txundo, nHeight);

            // Added
            pblock->vtx.push_back(tx);
            pblocktemplate->vTxFees.push_back(nTxFees);
            pblocktemplate->vTxSigOps.push_back(nTxSigOps);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;


        }

        // WL remove all spent Txs from MemPool
        for (it=listSpentTx.begin(); it!=listSpentTx.end(); ++it){
          mempool.remove(*it, spentTxRemoved, false);
        }

        start_time_2 = GetTimeMillis() - start_time_2; //wl added
        LogPrintf("CreateNewBlock() - Collect transactions into block - Duration: %u\n", start_time_2); // WL added

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;
        LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);
        start_time = GetTimeMillis() - start_time; //wl added
        LogPrintf("CreateNewBlock() - Duration: %u\n", start_time); // WL added

        // Compute final coinbase transaction.
        txNew.vout[0].nValue = GetBlockValue(nHeight, nFees);
        txNew.vin[0].scriptSig = CScript() << nHeight << OP_0;
        pblock->vtx[0] = txNew;
        pblocktemplate->vTxFees[0] = -nFees;

        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        UpdateTime(pblock, pindexPrev);
        pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock);
        pblock->nNonce         = 0;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        start_time = GetTimeMillis(); //WL added

//        CValidationState state;
//        if (!TestBlockValidity(state, *pblock, pindexPrev, false, false))
//            throw std::runtime_error("CreateNewBlock() : TestBlockValidity failed");

        start_time = GetTimeMillis() - start_time; //wl added
        LogPrintf("TestBlockValidity() - Duration: %u\n", start_time); // WL added
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerSec = 0.0;
int64_t nHPSTimerStart = 0;

CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey)
{
    CPubKey pubkey;
    if (!reservekey.GetReservedKey(pubkey))
        return NULL;

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
    return CreateNewBlock(scriptPubKey);
}

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey)
{
    //LogPrintf("%s\n", pblock->ToString());
    //LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("flashcoinMiner : generated block is stale");
    }

    // Remove key from key pool
    reservekey.KeepKey();

    // Track how many getdata requests this block gets

    int64_t start_time = GetTimeMillis(); //WL added

    {
        LOCK(wallet.cs_wallet);
        wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    start_time = GetTimeMillis() - start_time; //WL added
    LogPrintf("Track how many getdata requests this block gets - Duration: %u\n", start_time); // WL added

    // Process this block the same as if we had received it from another node

    start_time = GetTimeMillis(); //WL added

    CValidationState state;
    if (!ProcessNewBlock(state, NULL, pblock))
        return error("flashcoinMiner : ProcessNewBlock, block not accepted");

    start_time = GetTimeMillis() - start_time; //WL added
    LogPrintf("Process this block the same as if we had received it from another node [Total ProcessNewBlock] - Duration: %u\n", start_time); // WL added

    return true;
}

void static BitcoinMiner(CWallet *pwallet)
{
    LogPrintf("flashcoinMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("flashcoin-miner");

    // Each thread has its own key and counter
    CReserveKey reservekey(pwallet);
    unsigned int nExtraNonce = 0;

    try {
        while (true) {
            if (Params().MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    bool fvNodesEmpty;
                    {
                        LOCK(cs_vNodes);
                        fvNodesEmpty = vNodes.empty();
                    }
                    if (!fvNodesEmpty && !IsInitialBlockDownload())
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            // Param().MiningRequiresTransaction()
            bool miningRequiresTransaction = true;
            if(true) {
                do {
                    CBlockIndex* pindexPrev_check = chainActive.Tip();
                    if( (mempool.mapTx.size()>0) || (pindexPrev_check->nHeight < CONF_NUMBER_BLOCK_HAS_REWARD+100)) break;
                    //LogPrintf("miningRequiresTransaction mempool.mapTx.size(): %u\n", mempool.mapTx.size()); // WL added
                    MilliSleep(300);
                } while (true);
            }

            int64_t start_time = GetTimeMillis(); //WL added
            int64_t start_time_0 = GetTimeMillis(); //WL added

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            auto_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey));
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in flashcoinMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }

            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            if(pblock->vtx.size()==1){
                // check: if block is empty, we reject this block
                continue;
            }

            LogPrintf("Running flashcoinMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            start_time = GetTimeMillis(); //WL added

            //
            // Search
            //
            int64_t nStart = GetTime();
            uint256 hashTarget = uint256().SetCompact(pblock->nBits);
            uint256 thash;
            while (true) {
                unsigned int nHashesDone = 0;
                char scratchpad[SCRYPT_SCRATCHPAD_SIZE];
                while(true)
                {
                    scrypt_1024_1_1_256_sp(BEGIN(pblock->nVersion), BEGIN(thash), scratchpad);
                    if (thash <= hashTarget)
                    {
                        // Found a solution
                        SetThreadPriority(THREAD_PRIORITY_NORMAL);
                        LogPrintf("flashcoinMiner:\n");
                        LogPrintf("proof-of-work found  \n  powhash: %s  \ntarget: %s\n", thash.GetHex(), hashTarget.GetHex());

                        start_time = GetTimeMillis() - start_time; //WL added
                        LogPrintf("Mined block - Duration: %u\n", start_time); // WL added

                        int64_t start_time = GetTimeMillis(); //WL added

                        ProcessBlockFound(pblock, *pwallet, reservekey);

                        start_time = GetTimeMillis() - start_time; //WL added
                        LogPrintf("ProcessBlockFound (check tx and write to disk) - Duration: %u\n", start_time); // WL added

                        SetThreadPriority(THREAD_PRIORITY_LOWEST);

                        start_time_0 = GetTimeMillis() - start_time_0; //WL added
                        LogPrintf("Created+Mined+ProcessNewBlock - Duration: %u\n", start_time_0); // WL added

                        // In regression test mode, stop mining after a block is found.
//                        if (Params().MineBlocksOnDemand())
//                            throw boost::thread_interrupted();

//                        break;
                    }
                    pblock->nNonce += 1;
                    nHashesDone += 1;
                    if ((pblock->nNonce & 0xFF) == 0)
                        break;
                }



                // Meter hashes/sec
                static int64_t nHashCounter;
                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nHashCounter = 0;
                }
                else
                    nHashCounter += nHashesDone;
                if (GetTimeMillis() - nHPSTimerStart > 4000)
                {
                    static CCriticalSection cs;
                    {
                        LOCK(cs);
                        if (GetTimeMillis() - nHPSTimerStart > 4000)
                        {
                            dHashesPerSec = 1000.0 * nHashCounter / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nHashCounter = 0;
                            static int64_t nLogTime;
                            if (GetTime() - nLogTime > 30 * 60)
                            {
                                nLogTime = GetTime();
                                LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerSec/1000.0);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();
                // Regtest mode doesn't require peers
                if (vNodes.empty() && Params().MiningRequiresPeers())
                    break;
                if (pblock->nNonce >= 0xffff0000)
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != chainActive.Tip())
                    break;

                // Update nTime every few seconds
                UpdateTime(pblock, pindexPrev);
                if (Params().AllowMinDifficultyBlocks())
                {
                    // Changing pblock->nTime can change work required on testnet:
                    hashTarget.SetCompact(pblock->nBits);
                }
            }
        }


    }
    catch (boost::thread_interrupted)
    {
        LogPrintf("flashcoinMiner terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("flashcoinMiner runtime error: %s\n", e.what());
        return;
    }
}

void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads)
{
    //WL-Add this to be disable mining mode
    if(DISABLE_MINING) return;

    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) {
        // In regtest threads defaults to 1
        if (Params().DefaultMinerThreads())
            nThreads = Params().DefaultMinerThreads();
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&BitcoinMiner, pwallet));
}

#endif // ENABLE_WALLET
