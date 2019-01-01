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
 * @brief : Imp of blockchain
 * @author: mingzhenliu
 * @date: 2018-09-21
 */
#pragma once

#include "BlockChainInterface.h"
#include <libdevcore/Exceptions.h>
#include <libethcore/Block.h>
#include <libethcore/Common.h>
#include <libethcore/Transaction.h>
#include <libethcore/TransactionReceipt.h>
#include <libexecutive/StateFactoryInterface.h>
#include <libstorage/Common.h>
#include <libstorage/Storage.h>
#include <libstorage/SystemConfigPrecompiled.h>
#include <libstoragestate/StorageStateFactory.h>
#include <boost/thread/shared_mutex.hpp>
#include <deque>
#include <map>
#include <memory>
#include <mutex>

#define BLOCKCACHE_LOG(LEVEL) LOG(LEVEL) << "[#BLOCKCACHE]"
#define BLOCKCHAIN_LOG(LEVEL) LOG(LEVEL) << "[#BLOCKCHAIN]"

namespace dev
{
namespace blockverifier
{
class ExecutiveContext;
}  // namespace blockverifier
namespace storage
{
class MemoryTableFactory;
}

namespace blockchain
{
class BlockChainImp;

class BlockCache
{
public:
    BlockCache(){};
    std::shared_ptr<dev::eth::Block> add(dev::eth::Block& _block);
    std::pair<std::shared_ptr<dev::eth::Block>, dev::h256> get(h256 const& _hash);

private:
    mutable boost::shared_mutex m_sharedMutex;
    mutable std::map<dev::h256, std::pair<std::shared_ptr<dev::eth::Block>, dev::h256> >
        m_blockCache;
    mutable std::deque<dev::h256> m_blockCacheFIFO;  // insert queue log for m_blockCache
    const unsigned c_blockCacheSize = 10;            // m_blockCache size, default set 10
};
DEV_SIMPLE_EXCEPTION(OpenSysTableFailed);

class BlockChainImp : public BlockChainInterface
{
public:
    BlockChainImp() {}
    virtual ~BlockChainImp(){};
    int64_t number() override;
    dev::h256 numberHash(int64_t _i) override;
    dev::eth::Transaction getTxByHash(dev::h256 const& _txHash) override;
    dev::eth::LocalisedTransaction getLocalisedTxByHash(dev::h256 const& _txHash) override;
    dev::eth::TransactionReceipt getTransactionReceiptByHash(dev::h256 const& _txHash) override;
    virtual dev::eth::LocalisedTransactionReceipt getLocalisedTxReceiptByHash(
        dev::h256 const& _txHash) override;
    std::shared_ptr<dev::eth::Block> getBlockByHash(dev::h256 const& _blockHash) override;
    std::shared_ptr<dev::eth::Block> getBlockByNumber(int64_t _i) override;
    CommitResult commitBlock(dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context) override;
    virtual void setStateStorage(dev::storage::Storage::Ptr stateStorage);
    virtual void setStateFactory(dev::executive::StateFactoryInterface::Ptr _stateFactory);
    virtual std::shared_ptr<dev::storage::MemoryTableFactory> getMemoryTableFactory();
    bool checkAndBuildGenesisBlock(GenesisBlockParam& initParam) override;
    virtual std::pair<int64_t, int64_t> totalTransactionCount() override;
    dev::bytes getCode(dev::Address _address) override;

    dev::h512s minerList() override;
    dev::h512s observerList() override;
    std::string getSystemConfigByKey(std::string const& key, int64_t num = -1) override;

private:
    std::shared_ptr<dev::eth::Block> getBlock(int64_t _i);
    std::shared_ptr<dev::eth::Block> getBlock(dev::h256 const& _blockHash);
    void writeNumber(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeTotalTransactionCount(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeTxToBlock(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeBlockInfo(
        dev::eth::Block& block, std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeNumber2Hash(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeHash2Block(
        dev::eth::Block& block, std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    dev::storage::Storage::Ptr m_stateStorage;
    std::mutex commitMutex;
    const std::string c_genesisHash =
        "0xeb8b84af3f35165d52cb41abe1a9a3d684703aca4966ce720ecd940bd885517c";
    std::shared_ptr<dev::executive::StateFactoryInterface> m_stateFactory;

    dev::h512s getNodeListByType(int64_t num, std::string const& type);
    mutable SharedMutex m_nodeListMutex;
    dev::h512s m_minerList;
    dev::h512s m_observerList;
    int64_t m_cacheNumByMiner = -1;
    int64_t m_cacheNumByObserver = -1;

    struct SystemConfigRecord
    {
        std::string value;
        int64_t curBlockNum = -1;  // at which block gets the configuration value
        SystemConfigRecord(std::string const& _value, int64_t _num)
          : value(_value), curBlockNum(_num){};
    };
    std::map<std::string, SystemConfigRecord> m_systemConfigRecord;
    mutable SharedMutex m_systemConfigMutex;
    BlockCache m_blockCache;
};
}  // namespace blockchain
}  // namespace dev
