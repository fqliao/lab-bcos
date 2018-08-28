/**
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
 *
 * @brief: RLP test functions.
 *
 * @file rlp.h
 * @author: yujiechen
 * @date 2018-08-24
 */
#pragma once

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Exceptions.h>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/easylog.h>

namespace fs = boost::filesystem;
namespace js = json_spirit;
using namespace dev;
namespace dev
{
namespace test
{
enum class RlpType
{
    Valid,
    Invalid,
    Test
};
class RlpTest
{
public:
    static void runRlpTest(std::string _name, fs::path const& _path);
    /*
     * @ function: check string transformed to serialized RLP &&
     *             serialized RLP transformed to RLP structure
     * @ params: json object with "in" to be transformed into rlp or "VALID" "INVALID"
     *           "out" is the valid or invalid  rlp string
     * @ return:
     * */
    static void doRlpTests(js::mValue const& jsonObj);

private:
    // check encode
    static void inline checkEncode(std::string const& testName, js::mObject const& jsonObj)
    {
        RLPStream transedRlp;
        // transform in into rlp
        buildRLP(jsonObj.at("in"), transedRlp);
        std::string hexRlp = toHex(transedRlp.out());
        std::string expectedRlp = (jsonObj.at("out").get_str());
        transform(expectedRlp.begin(), expectedRlp.end(), expectedRlp.begin(), ::tolower);

        BOOST_CHECK_MESSAGE(hexRlp == expectedRlp, testName + "Encoding Failed:\n" +
                                                       "\t Expected:" + expectedRlp +
                                                       "\n Computed:" + hexRlp);
    }
    // check decode
    static void inline checkDecode(
        js::mObject const& jsonObj, RlpType const& rlpType, bool& is_exception)
    {
        is_exception = false;
        try
        {
            bytes rlpBytes = fromHex(jsonObj.at("out").get_str());
            RLP rlpObj(rlpBytes);
            std::cout << "#######RLP:" << rlpObj.itemCount() << std::endl;
            if (rlpType == RlpType::Test)
                checkRlp(jsonObj.at("in"), rlpObj);
        }
        catch (Exception const& e)
        {
            is_exception = true;
            LOG(ERROR) << "RLP Decode Exception:" << e.what();
        }
    }
    static void buildRLP(js::mValue const& jsonObj, dev::RLPStream& retRlp);
    static void checkRlp(js::mValue const& jsonObj, dev::RLP const& rlp);
};
}  // namespace test
}  // namespace dev
