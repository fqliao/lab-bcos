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
/** @file EntryPrecompiled.cpp
 *  @author ancelmo
 *  @date 20180921
 */
#include "EntryPrecompiled.h"
#include <libdevcore/easylog.h>
#include <libdevcrypto/Hash.h>
#include <libethcore/ABI.h>

using namespace dev;
using namespace dev::blockverifier;
using namespace dev::storage;

const char* const ENTRYIY_METHOD_getInt_string = "getInt(string)";
const char* const ENTRYIY_METHOD_set_string_int256 = "set(string,int256)";
const char* const ENTRYIY_METHOD_set_string_string = "set(string,string)";
const char* const ENTRYIY_METHOD_getAddress_string = "getAddress(string)";
const char* const ENTRYIY_METHOD_getBytes64_string = "getBytes64(string)";

EntryPrecompiled::EntryPrecompiled()
{
    name2Selector[ENTRYIY_METHOD_getInt_string] = getFuncSelector(ENTRYIY_METHOD_getInt_string);
    name2Selector[ENTRYIY_METHOD_set_string_int256] =
        getFuncSelector(ENTRYIY_METHOD_set_string_int256);
    name2Selector[ENTRYIY_METHOD_set_string_string] =
        getFuncSelector(ENTRYIY_METHOD_set_string_string);
    name2Selector[ENTRYIY_METHOD_getAddress_string] =
        getFuncSelector(ENTRYIY_METHOD_getAddress_string);
    name2Selector[ENTRYIY_METHOD_getBytes64_string] =
        getFuncSelector(ENTRYIY_METHOD_getBytes64_string);
}

std::string EntryPrecompiled::toString(std::shared_ptr<ExecutiveContext>)
{
    return "Entry";
}

bytes EntryPrecompiled::call(std::shared_ptr<ExecutiveContext> context, bytesConstRef param)
{
    STORAGE_LOG(DEBUG) << "call Entry:";


    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "func:" << std::hex << func;

    dev::eth::ContractABI abi;

    bytes out;

    if (func == name2Selector[ENTRYIY_METHOD_getInt_string])
    {  // getInt(string)
        std::string str;
        abi.abiOut(data, str);

        std::string value = m_entry->getField(str);

        u256 num = boost::lexical_cast<u256>(value);
        out = abi.abiIn("", num);
    }
    else if (func == name2Selector[ENTRYIY_METHOD_set_string_int256])
    {  // set(string,int256)
        std::string str;
        u256 value;
        abi.abiOut(data, str, value);

        m_entry->setField(str, boost::lexical_cast<std::string>(value));
    }
    else if (func == name2Selector[ENTRYIY_METHOD_set_string_string])
    {  // set(string,string)
        std::string str;
        std::string value;
        abi.abiOut(data, str, value);

        m_entry->setField(str, value);
    }
    else if (func == name2Selector[ENTRYIY_METHOD_getAddress_string])
    {  // getAddress(string)
        std::string str;
        abi.abiOut(data, str);

        std::string value = m_entry->getField(str);
        Address ret = Address(value);
        out = abi.abiIn("", ret);
    }
    else if (func == name2Selector[ENTRYIY_METHOD_getBytes64_string])
    {  // getBytes64(string)
        std::string str;
        abi.abiOut(data, str);

        std::string value = m_entry->getField(str);
        string64 ret;
        for (unsigned i = 0; i < 64; ++i)
            ret[i] = i < value.size() ? value[i] : 0;

        out = abi.abiIn("", ret);
    }

    return out;
}
