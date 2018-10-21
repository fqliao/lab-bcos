/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */
/**
 * @brief : Sync implementation
 * @author: jimmyshi
 * @date: 2018-10-15
 */

#include "SyncMaster.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::sync;
using namespace dev::p2p;
using namespace dev::blockchain;
using namespace dev::txpool;

void SyncMaster::start()
{
    startWorking();
}

void SyncMaster::stop()
{
    stopWorking();
}

void SyncMaster::doWork()
{
    if (!isSyncing())
    {
        // cout << "SyncMaster " << m_protocolId << " doWork()" << endl;
        if (m_newTransactions)
        {
            m_newTransactions = false;
            maintainTransactions();
        }
        if (m_newBlocks)
        {
            m_newBlocks = false;
            maintainBlocks();
        }
        // need download? ->set syncing and knownHighestNumber
    }

    if (isSyncing())
    {
        bool finished = maintainDownloadingQueue();
        if (finished)
            m_state = SyncState::Idle;
    }
}

void SyncMaster::workLoop()
{
    while (workerState() == WorkerState::Started)
    {
        doWork();
        if (idleWaitMs())
        {
            std::unique_lock<std::mutex> l(x_signalled);
            m_signalled.wait_for(l, std::chrono::milliseconds(idleWaitMs()));
        }
    }
}

SyncStatus SyncMaster::status() const
{
    RecursiveGuard l(x_sync);
    SyncStatus res;
    res.state = m_state;
    res.protocolId = m_protocolId;
    res.startBlockNumber = m_startingBlock;
    res.currentBlockNumber = m_blockChain->number();
    res.highestBlockNumber = m_highestBlock;
    return res;
}

bool SyncMaster::isSyncing() const
{
    return m_state != SyncState::Idle;
}

void SyncMaster::maintainTransactions()
{
    unordered_map<NodeID, std::vector<size_t>> peerTransactions;

    auto ts = m_txPool->topTransactionsCondition(
        c_maxSendTransactions, [&](Transaction const& _tx) { return !_tx.hasSent(); });


    {
        Guard l(x_transactions);
        for (size_t i = 0; i < ts.size(); ++i)
        {
            auto const& t = ts[i];
            NodeIDs peers;
            unsigned _percent = t->hasImportPeers() ? 25 : 100;

            peers = m_syncStatus->randomSelection(_percent,
                [&](std::shared_ptr<SyncPeerStatus> _p) { return !t->hasImportPeer(_p->nodeId); });

            for (auto const& p : peers)
                peerTransactions[p].push_back(i);

            if (!peers.empty())
                t->setHasSent();
        }
    }

    m_syncStatus->foreachPeer([&](shared_ptr<SyncPeerStatus> _p) {
        bytes txRLPs;
        unsigned txNumber = peerTransactions[_p->nodeId].size();
        if (0 == txNumber)
            return true;  // No need to send

        for (auto const& i : peerTransactions[_p->nodeId])
            txRLPs += ts[i]->rlp();

        SyncTransactionsPacket packet;
        packet.encode(txNumber, txRLPs);

        m_service->asyncSendMessageByNodeID(_p->nodeId, packet.toMessage(m_protocolId));
        LOG(TRACE) << "Send" << int(txNumber) << "transactions to " << _p->nodeId;

        return true;
    });
}

void SyncMaster::maintainBlocks()
{
    int64_t number = m_blockChain->number();
    h256 const& currentHash = m_blockChain->numberHash(number);

    // Just broadcast status
    m_syncStatus->foreachPeer([&](shared_ptr<SyncPeerStatus> _p) {
        SyncStatusPacket packet;
        packet.encode(number, m_genesisHash, currentHash);

        m_service->asyncSendMessageByNodeID(_p->nodeId, packet.toMessage(m_protocolId));
        LOG(TRACE) << "Send status (" << int(number) << "," << m_genesisHash << "," << currentHash
                   << ") to " << _p->nodeId;

        return true;
    });
}


bool SyncMaster::maintainDownloadingQueue()
{
    int64_t currentNumber = m_blockChain->number();
    if (currentNumber < m_syncStatus->knownHighestNumber)
    {
        BlockPtrVec blocks =
            m_syncStatus->bq().popSequent(currentNumber + 1, m_syncStatus->knownHighestNumber);
        // for (auto block : blocks)
        // m_blockChain->commitBlock(*blocks, );  // How to use executiveContext)
    }

    currentNumber = m_blockChain->number();
    return currentNumber >= m_syncStatus->knownHighestNumber;
}