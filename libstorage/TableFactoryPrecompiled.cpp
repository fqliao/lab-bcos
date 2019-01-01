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
/** @file TableFactoryPrecompiled.h
 *  @author ancelmo
 *  @date 20180921
 */
#include "TableFactoryPrecompiled.h"
#include "MemoryTable.h"
#include "TablePrecompiled.h"
#include <libdevcore/easylog.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/Hash.h>
#include <libethcore/ABI.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace dev;
using namespace dev::blockverifier;
using namespace std;
using namespace dev::storage;

const std::string TABLE_METHOD_OPT_STR = "openTable(string)";
const std::string TABLE_METHOD_CRT_STR_STR = "createTable(string,string,string)";

TableFactoryPrecompiled::TableFactoryPrecompiled()
{
    name2Selector[TABLE_METHOD_OPT_STR] = getFuncSelector(TABLE_METHOD_OPT_STR);
    name2Selector[TABLE_METHOD_CRT_STR_STR] = getFuncSelector(TABLE_METHOD_CRT_STR_STR);
}

std::string TableFactoryPrecompiled::toString(std::shared_ptr<ExecutiveContext>)
{
    return "TableFactory";
}

bytes TableFactoryPrecompiled::call(
    ExecutiveContext::Ptr context, bytesConstRef param, Address const& origin)
{
    STORAGE_LOG(DEBUG) << "this: " << this
                       << " call TableFactoryPrecompiled [param=" << toHex(param) << "]";


    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "TableFactoryPrecompiled call [func=" << std::hex << func << "]";

    dev::eth::ContractABI abi;
    bytes out;

    if (func == name2Selector[TABLE_METHOD_OPT_STR])
    {  // openTable(string)
        string tableName;
        abi.abiOut(data, tableName);

        STORAGE_LOG(DEBUG) << "DBFactory open table:" << tableName;
        tableName = storage::USER_TABLE_PREFIX + tableName;
        Address address;
        auto table = m_memoryTableFactory->openTable(tableName);
        if (table)
        {
            TablePrecompiled::Ptr tablePrecompiled = make_shared<TablePrecompiled>();
            tablePrecompiled->setTable(table);
            address = context->registerPrecompiled(tablePrecompiled);
        }
        else
        {
            STORAGE_LOG(DEBUG) << "Open new table:" << tableName << " failed.";
        }

        out = abi.abiIn("", address);
    }
    else if (func == name2Selector[TABLE_METHOD_CRT_STR_STR])
    {  // createTable(string,string,string)
        string tableName;
        string keyField;
        string valueFiled;

        abi.abiOut(data, tableName, keyField, valueFiled);
        vector<string> fieldNameList;
        boost::split(fieldNameList, valueFiled, boost::is_any_of(","));
        for (auto& str : fieldNameList)
            boost::trim(str);
        valueFiled = boost::join(fieldNameList, ",");
        tableName = storage::USER_TABLE_PREFIX + tableName;
        auto table =
            m_memoryTableFactory->createTable(tableName, keyField, valueFiled, true, origin);
        // set createTableCode
        int createTableCode = m_memoryTableFactory->getCreateTableCode();
        if (createTableCode == -1)
        {
            STORAGE_LOG(WARNING)
                << "TableFactoryPrecompiled createTable operation is not authorized [origin="
                << origin.hex() << "]";
        }
        else
        {
            STORAGE_LOG(DEBUG)
                << "TableFactoryPrecompiled createTable operation is successful [tableName="
                << tableName << "]";
        }
        out = abi.abiIn("", u256(createTableCode));
    }
    return out;
}

h256 TableFactoryPrecompiled::hash()
{
    return m_memoryTableFactory->hash();
}
