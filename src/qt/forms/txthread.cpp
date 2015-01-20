#include "txthread.h"
#include "QTime"
#include <QDebug>
#include <QtCore>

#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "base58.h"

#include <boost/assign/list_of.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

extern CBlockIndex* pindexGenesisBlock;

TxThread::TxThread(std::string _walletfile, int _ns, int _ne, int _txn, int _nAmount, int _sleepTx, bool _sendOO, bool _resc, bool _sendtx, bool _creadd, int _wpth, QMap<int, QString> _arrAddress)
{
    now="";
    stopped = false;
    walletfile = _walletfile;
    nStart = _ns;
    nEnd = _ne;
    txNumber = _txn;
    sendTx = _sendtx;
    reScan = _resc;
    creAdd = _creadd;
    walletPerThread = _wpth;
    arrAddr = _arrAddress;
    nAmount = _nAmount;
    sleepTx = _sleepTx;
    sendOneOne = _sendOO;
}
TxThread::~TxThread()
{
}
TxThread::TxThread()
{
}

std::string intToString(int i)
{
    std::stringstream ss;
    std::string s;
    ss << i;
    s = ss.str();

    return s;
}
int64 xGetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64 nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!wtx.IsFinal())
            continue;

        int64 nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}
void TxThread::test_case_1(){
    while(!stopped) {
        InitBlockIndex();
        CWallet* pwalletThread_tmp;
        bool ffirstrun;
        int j = 1;
        for (int i = nStart; i<nEnd; i++){
            std::string walletfile_tmp = "wallet"+intToString(i)+".dat";
            //std::string walletfile_tmp = walletfile;
            pwalletThread_tmp = new CWallet(walletfile_tmp);
            pwalletThread_tmp->LoadWallet(ffirstrun);
            RegisterWallet(pwalletThread_tmp);

            CWalletDB walletdb_tmp(walletfile_tmp);
            string strAccount_tmp = "";
            CAccount account_tmp;
            walletdb_tmp.ReadAccount(strAccount_tmp, account_tmp);

            bool is_exist_address = false;
            CBitcoinAddress address_tmp;
            BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, pwalletThread_tmp->mapAddressBook)
            {
                address_tmp = item.first;
                is_exist_address = true;
            }
            if(!is_exist_address){
                // Generate a new key that is added to wallet
                CPubKey newKey;
                if (!pwalletThread_tmp->GetKeyFromPool(newKey, false))
                    throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
                CKeyID keyID = newKey.GetID();

                pwalletThread_tmp->SetAddressBookName(keyID, strAccount_tmp);
                address_tmp = CBitcoinAddress(keyID);
                qDebug() << "keyID.ToString() " << QString::fromStdString(walletfile_tmp) << ":" << QString::fromStdString(address_tmp.ToString());
            }

            qDebug() << "address_tmp.ToString() " << QString::fromStdString(walletfile_tmp) << ":" << QString::fromStdString(address_tmp.ToString())  << "Balance: " << QString::number(pwalletThread_tmp->GetBalance());

            //----------------------------------------------------------
            // Amount 1 Coin
            int64 nAmount = AmountFromValue(1);

            CWalletTx wtx;
            //wtx.mapValue["comment"] = QString("Send test coin from thread at: "+now);
            //wtx.mapValue["to"]  = mapAddress[j].toStdString();
            j++;

            //Send 1 coin to mapAddress[i]
//            std::string strError = pwalletThread_tmp->SendMoneyToDestination(CBitcoinAddress(mapAddress[j].toStdString()).Get(), nAmount, wtx);
//            qDebug() << "Send from wallet " << QString::fromStdString(walletfile_tmp) << "Tx 1 Coin to Address: " << mapAddress[i];
//            if (strError != "")
//                throw JSONRPCError(RPC_WALLET_ERROR, strError);
//            msleep(10);

            walletdb_tmp.Close();

            //delete pwalletThread_tmp;
        }

        stopped = true;
    }
}
std::string TxThread::create_address(CWallet* _pwallet){
    string strAccount_tmp = "";
    CBitcoinAddress address_tmp;
    bool is_exist_address = false;

    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, _pwallet->mapAddressBook)
    {
        address_tmp = item.first;
        is_exist_address = true;
    }
    if(!is_exist_address){
        // Generate a new key that is added to wallet
        CPubKey newKey;
        if (!_pwallet->GetKeyFromPool(newKey, false)){
            qDebug() << QString::fromStdString(_pwallet->strWalletFile) << " Error: Keypool ran out, please call keypoolrefill first";
        }
        else{
            CKeyID keyID = newKey.GetID();

            _pwallet->SetAddressBookName(keyID, strAccount_tmp);
            address_tmp = CBitcoinAddress(keyID);
            //qDebug() << "keyID.ToString() " << QString::fromStdString(_pwallet->strWalletFile) << ":" << QString::fromStdString(address_tmp.ToString());
        }
    }
    qDebug() << QString::fromStdString(_pwallet->strWalletFile) << " " << QString::fromStdString(address_tmp.ToString());

    return address_tmp.ToString();
}

CWallet* TxThread::load_wallet(std::string walletfilename){
    //InitBlockIndex();
    CWallet* pwalletThread_tmp;
    bool ffirstrun;

    pwalletThread_tmp = new CWallet(walletfilename);
    pwalletThread_tmp->LoadWallet(ffirstrun);
    RegisterWallet(pwalletThread_tmp);

    if(creAdd){
        create_address(pwalletThread_tmp);
    }

    if (pwalletThread_tmp) {
        // Add wallet transactions that aren't already in a block to mapTransactions
        pwalletThread_tmp->ReacceptWalletTransactions();
    }

    if(reScan){
        CBlockIndex *pindexRescan = pindexGenesisBlock;
        if (1){
            nStart = GetTimeMillis();
            pwalletThread_tmp->ScanForWalletTransactions(pindexRescan, true);
        }
    }
    return pwalletThread_tmp;
}
void TxThread::send_transactions(CWallet* _pwallet, QMap<int, QString> _arrAdress){
    CWalletTx wtx;
    int64 Amount = AmountFromValue(nAmount);
    for (int j=0; j< txNumber; j++){
        for (int i=1; i< _arrAdress.count()+1; i++){

            //wtx.mapValue["comment"] = "Send test coin from thread: "+walletfile;
            wtx.mapValue["to"]  = _arrAdress[i].toStdString();

           // Send 1 coin to _arrAdress[i]
           std::string strError = _pwallet->SendMoneyToDestination(CBitcoinAddress(_arrAdress[i].toStdString()).Get(), Amount, wtx);
           qDebug() << "Send from wallet " << QString::fromStdString(_pwallet->strWalletFile) << "Tx/ntimes: " << QString::number(i) << "/" << QString::number(j+1) << " Coin to Address: " << _arrAdress[i] << " at: " << QTime::currentTime().toString();
           if (strError != ""){
               //throw JSONRPCError(RPC_WALLET_ERROR, strError);
               qDebug() << "throw JSONRPCError(RPC_WALLET_ERROR, strError);" << QString::fromStdString(strError);
               break;
           }
         }
        if(sleepTx>0) sleep(sleepTx);
    }
}

void TxThread::run()
{

    for (int i = nStart; i<nEnd; i++){
        std::string walletfile_tmp = "wallet"+intToString(i)+".dat";
        if(!creAdd) qDebug() << " Loading wallet: " << QString::fromStdString(walletfile_tmp);
        CWallet* walletX = load_wallet(walletfile_tmp);

        if(!creAdd)
            qDebug() << QString::fromStdString(walletfile_tmp) << " Balance: " << QString::number(walletX->GetBalance());


        if(sendTx){
            qDebug() << "TxNumber: " << QString::number(txNumber);
            if(sendOneOne){
                QMap<int, QString> arrAddrOneOne;
                arrAddrOneOne[1] = arrAddr[i-nStart+1];
                send_transactions(walletX,arrAddrOneOne);
            }else{
                send_transactions(walletX,arrAddr);
            }
        }

        if (walletX)
            walletX->SetBestChain(CBlockLocator(pindexBest));

        UnregisterWallet(walletX);
        if (walletX)
            delete walletX;

    }

    //exit(1);

}
void TxThread::stopProcess()
{
    mutex.lock();
    stopped = true;
    mutex.unlock();
}
