/*
    This file is part of FISCO-BCOS.

    FISCO-BCOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FISCO-BCOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file AuthorityPrecompiled.h
 *  @author caryliao
 *  @date 20181205
 */
#include "AuthorityPrecompiled.h"
#include "libstorage/EntriesPrecompiled.h"
#include "libstorage/TableFactoryPrecompiled.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/easylog.h>
#include <libethcore/ABI.h>
#include <boost/lexical_cast.hpp>

using namespace dev;
using namespace dev::blockverifier;
using namespace dev::storage;


const std::string AUP_METHOD_INS = "insert(string,string)";
const std::string AUP_METHOD_REM = "remove(string,string)";
const std::string AUP_METHOD_QUE = "queryByName(string)";


AuthorityPrecompiled::AuthorityPrecompiled()
{
    name2Selector[AUP_METHOD_INS] = getFuncSelector(AUP_METHOD_INS);
    name2Selector[AUP_METHOD_REM] = getFuncSelector(AUP_METHOD_REM);
    name2Selector[AUP_METHOD_QUE] = getFuncSelector(AUP_METHOD_QUE);
}


std::string AuthorityPrecompiled::toString(ExecutiveContext::Ptr)
{
    return "Authority";
}

storage::Table::Ptr AuthorityPrecompiled::openTable(
    ExecutiveContext::Ptr context, const std::string& tableName)
{
    STORAGE_LOG(DEBUG) << "AuthorityPrecompiled open table:" << tableName;
    TableFactoryPrecompiled::Ptr tableFactoryPrecompiled =
        std::dynamic_pointer_cast<TableFactoryPrecompiled>(
            context->getPrecompiled(Address(0x1001)));
    return tableFactoryPrecompiled->getmemoryTableFactory()->openTable(tableName);
}

bytes AuthorityPrecompiled::call(
    ExecutiveContext::Ptr context, bytesConstRef param, Address const& origin)
{
    STORAGE_LOG(TRACE) << "this: " << this << " call AuthorityPrecompiled [param=" << toHex(param)
                       << "]";

    // parse function name
    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "AuthorityPrecompiled call [func=" << std::hex << func << "]";

    dev::eth::ContractABI abi;
    bytes out;


    if (func == name2Selector[AUP_METHOD_INS])
    {
        // insert(string tableName,string addr)
        std::string tableName, addr;
        abi.abiOut(data, tableName, addr);
        addPrefixToUserTable(tableName);

        Table::Ptr table = openTable(context, SYS_ACCESS_TABLE);

        auto condition = table->newCondition();
        condition->EQ(SYS_AC_ADDRESS, addr);
        auto entries = table->select(tableName, condition);
        if (entries->size() != 0u)
        {
            STORAGE_LOG(DEBUG) << "The tableName and address already exist [tableName=" << tableName
                               << ", address=" << addr << "]";
            out = abi.abiIn("", 0);
        }
        else
        {
            auto entry = table->newEntry();
            entry->setField(SYS_AC_TABLE_NAME, tableName);
            entry->setField(SYS_AC_ADDRESS, addr);
            entry->setField(SYS_AC_ENABLENUM,
                boost::lexical_cast<std::string>(context->blockInfo().number + 1));
            int count = table->insert(tableName, entry, getOptions(origin));
            if (count == -1)
            {
                STORAGE_LOG(WARNING)
                    << "AuthorityPrecompiled insert operation is not authorized [origin="
                    << origin.hex() << "]";
            }
            else
            {
                STORAGE_LOG(DEBUG)
                    << "AuthorityPrecompiled insert operation is successful [tableName="
                    << tableName << ", address=" << addr << "]";
            }
            out = abi.abiIn("", count);
        }
    }
    else if (func == name2Selector[AUP_METHOD_REM])
    {
        // remove(string tableName,string addr)
        std::string tableName, addr;
        abi.abiOut(data, tableName, addr);
        addPrefixToUserTable(tableName);

        Table::Ptr table = openTable(context, SYS_ACCESS_TABLE);

        bool exist = false;
        auto condition = table->newCondition();
        condition->EQ(SYS_AC_ADDRESS, addr);
        auto entries = table->select(tableName, condition);
        if (entries->size() == 0u)
        {
            STORAGE_LOG(WARNING) << "The tableName and address does not exist [tableName="
                                 << tableName << ", address=" << addr << "]";
            out = abi.abiIn("", u256(0));
        }
        else
        {
            int count = table->remove(tableName, condition, getOptions(origin));
            if (count == -1)
            {
                STORAGE_LOG(WARNING)
                    << "AuthorityPrecompiled remove operation is not authorized, origin: "
                    << origin.hex();
            }
            else
            {
                STORAGE_LOG(DEBUG)
                    << "AuthorityPrecompiled remove operation is successful [tableName="
                    << tableName << ", address=" << addr << "]";
            }
            out = abi.abiIn("", count);
        }
    }
    else if (func == name2Selector[AUP_METHOD_QUE])
    {
        // queryByName(string table_name)
        std::string tableName;
        abi.abiOut(data, tableName);
        addPrefixToUserTable(tableName);

        Table::Ptr table = openTable(context, SYS_ACCESS_TABLE);

        auto condition = table->newCondition();
        auto entries = table->select(tableName, condition);
        json_spirit::Array AuthorityInfos;
        if (entries)
        {
            for (size_t i = 0; i < entries->size(); i++)
            {
                auto entry = entries->get(i);
                json_spirit::Object AuthorityInfo;
                AuthorityInfo.push_back(json_spirit::Pair(SYS_AC_TABLE_NAME, tableName));
                AuthorityInfo.push_back(
                    json_spirit::Pair(SYS_AC_ADDRESS, entry->getField(SYS_AC_ADDRESS)));
                AuthorityInfo.push_back(
                    json_spirit::Pair(SYS_AC_ENABLENUM, entry->getField(SYS_AC_ENABLENUM)));
                AuthorityInfos.push_back(AuthorityInfo);
            }
        }
        json_spirit::Value value(AuthorityInfos);
        std::string str = json_spirit::write_string(value, true);

        out = abi.abiIn("", str);
    }
    else
    {
        STORAGE_LOG(ERROR) << "AuthorityPrecompiled error [func=" << std::hex << func << "]";
    }
    return out;
}

void AuthorityPrecompiled::addPrefixToUserTable(std::string& table_name)
{
    if (table_name == SYS_ACCESS_TABLE || table_name == SYS_MINERS || table_name == SYS_TABLES ||
        table_name == SYS_CNS || table_name == SYS_CONFIG)
    {
        return;
    }
    else
    {
        table_name = USER_TABLE_PREFIX + table_name;
    }
}
