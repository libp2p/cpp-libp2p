/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/crypto/cipher_suite.hpp>

#include <sstream>

namespace libp2p::security::noise {

  CipherSuiteImpl::CipherSuiteImpl(std::shared_ptr<DiffieHellman> dh,
                                   std::shared_ptr<NamedHasher> hash,
                                   std::shared_ptr<NamedAEADCipher> cipher)
      : dh_{std::move(dh)},
        hash_{std::move(hash)},
        cipher_{std::move(cipher)} {}

  outcome::result<DHKey> CipherSuiteImpl::generate() {
    return dh_->generate();
  }

  outcome::result<ByteArray> CipherSuiteImpl::dh(const ByteArray &private_key,
                                                 const ByteArray &public_key) {
    return dh_->dh(private_key, public_key);
  }

  int CipherSuiteImpl::dhSize() const {
    return dh_->dhSize();
  }

  std::string CipherSuiteImpl::dhName() const {
    return dh_->dhName();
  }

  std::shared_ptr<crypto::Hasher> CipherSuiteImpl::hash() {
    return hash_->hash();
  }

  std::string CipherSuiteImpl::hashName() const {
    return hash_->hashName();
  }

  std::shared_ptr<AEADCipher> CipherSuiteImpl::cipher(Key32 key) {
    return cipher_->cipher(key);
  }

  std::string CipherSuiteImpl::cipherName() const {
    return cipher_->cipherName();
  }

  std::string CipherSuiteImpl::name() {
    std::stringstream s;
    s << dhName() << "_" << cipherName() << "_" << hashName();
    return s.str();
  }

}  // namespace libp2p::security::noise
