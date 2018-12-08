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
/** @file AES.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * AES
 * todo: use openssl
 */

#pragma once

#include "Common.h"

namespace dev
{
bytes aesDecrypt(bytesConstRef _cipher, std::string const& _password, unsigned _rounds = 2000,
    bytesConstRef _salt = bytesConstRef());
bytes aesCBCEncrypt(bytesConstRef plainData, std::string const& keyData, int keyLen,
    bytesConstRef ivData);  ////AES encrypt
bytes aesCBCDecrypt(bytesConstRef cipherData, std::string const& keyData, int keyLen,
    bytesConstRef ivData);  // AES decrypt

// bytes origAesCBCEncrypt(
//     bytesConstRef plainData, std::string const& keyData, int keyLen, bytesConstRef ivData);
// bytes origAesCBCDecrypt(
//     bytesConstRef cipherData, std::string const& keyData, int keyLen, bytesConstRef ivData);
}  // namespace dev
