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
/** @file CNSPrecompiled.cpp
 *  @author chaychen
 *  @date 20181119
 */
#include "CNSPrecompiled.h"
#include "libstorage/EntriesPrecompiled.h"
#include "libstorage/TableFactoryPrecompiled.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/easylog.h>
#include <libethcore/ABI.h>

using namespace dev;
using namespace dev::blockverifier;
using namespace dev::storage;

const std::string CNS_METHOD_INS_STR4 = "insert(string,string,string,string)";
const std::string CNS_METHOD_SLT_STR = "selectByName(string)";
const std::string CNS_METHOD_SLT_STR2 = "selectByNameAndVersion(string,string)";

CNSPrecompiled::CNSPrecompiled()
{
    name2Selector[CNS_METHOD_INS_STR4] = getFuncSelector(CNS_METHOD_INS_STR4);
    name2Selector[CNS_METHOD_SLT_STR] = getFuncSelector(CNS_METHOD_SLT_STR);
    name2Selector[CNS_METHOD_SLT_STR2] = getFuncSelector(CNS_METHOD_SLT_STR2);
}


std::string CNSPrecompiled::toString(ExecutiveContext::Ptr)
{
    return "CNS";
}

Table::Ptr CNSPrecompiled::openTable(ExecutiveContext::Ptr context, const std::string& tableName)
{
    STORAGE_LOG(DEBUG) << "CNS open table:" << tableName;
    TableFactoryPrecompiled::Ptr tableFactoryPrecompiled =
        std::dynamic_pointer_cast<TableFactoryPrecompiled>(
            context->getPrecompiled(Address(0x1001)));
    return tableFactoryPrecompiled->getmemoryTableFactory()->openTable(tableName);
}

bytes CNSPrecompiled::call(
    ExecutiveContext::Ptr context, bytesConstRef param, Address const& origin)
{
    STORAGE_LOG(TRACE) << "this: " << this << " call CNSPrecompiled [param=" << toHex(param) << "]";

    // parse function name
    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "CNSPrecompiled call [func=" << std::hex << func << "]";

    dev::eth::ContractABI abi;
    bytes out;

    if (func == name2Selector[CNS_METHOD_INS_STR4])
    {
        // insert(string,string,string,string)
        // insert(name, version, address, abi), 4 fields in table, the key of table is name field
        std::string contractName, contractVersion, contractAddress, contractAbi;
        abi.abiOut(data, contractName, contractVersion, contractAddress, contractAbi);
        Table::Ptr table = openTable(context, SYS_CNS);

        auto condition = table->newCondition();
        condition->EQ(SYS_CNS_FIELD_VERSION, contractVersion);
        auto entries = table->select(contractName, condition);
        if (entries->size() != 0u)
        {
            STORAGE_LOG(DEBUG)
                << "The contractName and contractVersion already exist [contractName="
                << contractName << ", contractVersion=" << contractVersion << "]";
            out = abi.abiIn("", 0);
        }
        else
        {
            // do insert
            auto entry = table->newEntry();
            entry->setField(SYS_CNS_FIELD_NAME, contractName);
            entry->setField(SYS_CNS_FIELD_VERSION, contractVersion);
            entry->setField(SYS_CNS_FIELD_ADDRESS, contractAddress);
            entry->setField(SYS_CNS_FIELD_ABI, contractAbi);
            int count = table->insert(contractName, entry, getOptions(origin));
            if (count == -1)
            {
                STORAGE_LOG(WARNING)
                    << "CNSPrecompiled insert operation is not authorized [origin=" << origin.hex()
                    << "]";
            }
            else
            {
                STORAGE_LOG(DEBUG)
                    << "CNSPrecompiled insert operation is successful [contractName="
                    << contractName << ", contractVersion=" << contractVersion << "]";
            }
            out = abi.abiIn("", count);
        }
    }
    else if (func == name2Selector[CNS_METHOD_SLT_STR])
    {
        // selectByName(string) returns(string)
        // Cursor is not considered.
        std::string contractName;
        abi.abiOut(data, contractName);
        Table::Ptr table = openTable(context, SYS_CNS);

        json_spirit::Array CNSInfos;
        auto entries = table->select(contractName, table->newCondition());
        if (entries.get())
        {
            for (size_t i = 0; i < entries->size(); i++)
            {
                auto entry = entries->get(i);
                if (!entry)
                    continue;
                json_spirit::Object CNSInfo;
                CNSInfo.push_back(json_spirit::Pair(SYS_CNS_FIELD_NAME, contractName));
                CNSInfo.push_back(json_spirit::Pair(
                    SYS_CNS_FIELD_VERSION, entry->getField(SYS_CNS_FIELD_VERSION)));
                CNSInfo.push_back(json_spirit::Pair(
                    SYS_CNS_FIELD_ADDRESS, entry->getField(SYS_CNS_FIELD_ADDRESS)));
                CNSInfo.push_back(
                    json_spirit::Pair(SYS_CNS_FIELD_ABI, entry->getField(SYS_CNS_FIELD_ABI)));
                CNSInfos.push_back(CNSInfo);
            }
        }
        json_spirit::Value value(CNSInfos);
        std::string str = json_spirit::write_string(value, true);
        out = abi.abiIn("", str);
    }
    else if (func == name2Selector[CNS_METHOD_SLT_STR2])
    {
        // selectByNameAndVersion(string,string) returns(string)
        std::string contractName, contractVersion;
        abi.abiOut(data, contractName, contractVersion);
        Table::Ptr table = openTable(context, SYS_CNS);

        json_spirit::Array CNSInfos;
        auto entries = table->select(contractName, table->newCondition());
        if (entries.get())
        {
            for (size_t i = 0; i < entries->size(); i++)
            {
                auto entry = entries->get(i);
                if (contractVersion == entry->getField(SYS_CNS_FIELD_VERSION))
                {
                    json_spirit::Object CNSInfo;
                    CNSInfo.push_back(json_spirit::Pair(SYS_CNS_FIELD_NAME, contractName));
                    CNSInfo.push_back(json_spirit::Pair(
                        SYS_CNS_FIELD_VERSION, entry->getField(SYS_CNS_FIELD_VERSION)));
                    CNSInfo.push_back(json_spirit::Pair(
                        SYS_CNS_FIELD_ADDRESS, entry->getField(SYS_CNS_FIELD_ADDRESS)));
                    CNSInfo.push_back(
                        json_spirit::Pair(SYS_CNS_FIELD_ABI, entry->getField(SYS_CNS_FIELD_ABI)));
                    CNSInfos.push_back(CNSInfo);
                    // Only one
                    break;
                }
            }
        }
        json_spirit::Value value(CNSInfos);
        std::string str = json_spirit::write_string(value, true);
        out = abi.abiIn("", str);
    }
    else
    {
        STORAGE_LOG(ERROR) << "CNSPrecompiled error [func=" << std::hex << func << "]";
    }

    return out;
}
