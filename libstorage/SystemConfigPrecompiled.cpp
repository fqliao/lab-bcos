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
/** @file SystemConfigPrecompiled.cpp
 *  @author chaychen
 *  @date 20181211
 */
#include "SystemConfigPrecompiled.h"
#include "libstorage/EntriesPrecompiled.h"
#include "libstorage/TableFactoryPrecompiled.h"
#include <libdevcore/easylog.h>
#include <libethcore/ABI.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
using namespace dev;
using namespace dev::blockverifier;
using namespace dev::storage;


const std::string SYSCONFIG_METHOD_SET_STR = "setValueByKey(string,string)";

SystemConfigPrecompiled::SystemConfigPrecompiled()
{
    name2Selector[SYSCONFIG_METHOD_SET_STR] = getFuncSelector(SYSCONFIG_METHOD_SET_STR);
}

bytes SystemConfigPrecompiled::call(
    ExecutiveContext::Ptr context, bytesConstRef param, Address const& origin)
{
    STORAGE_LOG(TRACE) << "this: " << this
                       << " call SystemConfigPrecompiled [param=" << toHex(param) << "]";

    // parse function name
    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "SystemConfigPrecompiled call [func=" << std::hex << func << "]";

    dev::eth::ContractABI abi;
    bytes out;
    u256 count = 0;

    if (func == name2Selector[SYSCONFIG_METHOD_SET_STR])
    {
        // setValueByKey(string,string)
        std::string configKey, configValue;
        abi.abiOut(data, configKey, configValue);
        // Uniform lowercase configKey
        boost::to_lower(configKey);
        STORAGE_LOG(DEBUG) << "SystemConfigPrecompiled params:" << configKey << ", " << configValue;

        if (!checkValueValid(configKey, configValue))
        {
            // The transaction returns 0, indicating that the system config setting failed.
            out = abi.abiIn("", 0);
            STORAGE_LOG(WARNING) << "SystemConfigPrecompiled set invalid value [configKey="
                                 << configKey << ", configValue=" << configValue << "]";
            return out;
        }

        storage::Table::Ptr table = openTable(context, SYS_CONFIG);

        auto condition = table->newCondition();
        auto entries = table->select(configKey, condition);
        auto entry = table->newEntry();
        entry->setField(SYSTEM_CONFIG_KEY, configKey);
        entry->setField(SYSTEM_CONFIG_VALUE, configValue);
        entry->setField(SYSTEM_CONFIG_ENABLENUM,
            boost::lexical_cast<std::string>(context->blockInfo().number + 1));

        if (entries->size() == 0u)
        {
            count = table->insert(configKey, entry, getOptions(origin));
            if (count == -1)
            {
                STORAGE_LOG(WARNING)
                    << "SystemConfigPrecompiled insert operation is not authorized [origin="
                    << origin.hex() << "]";
            }
            else
            {
                STORAGE_LOG(DEBUG)
                    << "SystemConfigPrecompiled insert operation is successful [configKey="
                    << configKey << ", configValue=" << configValue << "]";
            }
        }
        else
        {
            count = table->update(configKey, entry, condition, getOptions(origin));
            if (count == -1)
            {
                STORAGE_LOG(WARNING)
                    << "SystemConfigPrecompiled update operation is not authorized [origin="
                    << origin.hex() << "]";
            }
            else
            {
                STORAGE_LOG(DEBUG)
                    << "SystemConfigPrecompiled update operation is successful [configKey="
                    << configKey << ", configValue=" << configValue << "]";
            }
        }

        out = abi.abiIn("", count);
    }
    else
    {
        STORAGE_LOG(ERROR) << "SystemConfigPrecompiled error [func=" << std::hex << func << "]";
    }
    return out;
}

bool SystemConfigPrecompiled::checkValueValid(std::string const& key, std::string const& value)
{
    if (SYSTEM_KEY_TX_COUNT_LIMIT == key)
    {
        return (boost::lexical_cast<uint64_t>(value) >= TX_COUNT_LIMIT_MIN);
    }
    else if (SYSTEM_KEY_TX_GAS_LIMIT == key)
    {
        return (boost::lexical_cast<uint64_t>(value) >= TX_GAS_LIMIT_MIN);
    }
    return true;
}
