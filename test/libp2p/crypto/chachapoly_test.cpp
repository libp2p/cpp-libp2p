/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/crypto/chachapoly/chachapoly_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/common_functions.hpp>
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using libp2p::crypto::chachapoly::ChaCha20Poly1305Impl;
using libp2p::crypto::chachapoly::Key;
using libp2p::crypto::chachapoly::Nonce;

using libp2p::crypto::asArray;
using libp2p::common::operator""_unhex;

/**
 * The data is took from golang crypto implementation tests' sources.
 */
class ChaChaPolyTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();
  }

  using Bytes = std::vector<uint8_t>;
  Key key = asArray<Key>(
      "f3dac58738ce057d3140d68d2b3e651c00ff9dbb2ca0f913be50219dd36f23c6"_unhex);
  Bytes plaintext =
      "4ab8068988d4bbe0bf1e5bc2fe1c668cbe58019c958dd2ec97164aea7f3f41c9f747527f"
      "1c0e5fdb2cbb9d2ad704b6955cb731f14403dddb1a28c5996707635e4eb5dd6ac33d46ef"
      "f8e319cfe7cf6443869534ca9812a5b23a6b4ca172afffc064dc2b28197117115431e03c"
      "00447f87d9b45172c6f724006270a1d41fa094847cbfac9630c3a785f488c1f5cc407ca6"
      "f4cd18bac43cba26ad5bfaccfb8f50784efc0e7fc0b504b43dc5a90a0525b0faf3c8b4b7"
      "046fdeb1cad87ec667ce3eb6cb4c358b01393f3ffee949030ef9fd01c1b2b9c5219777eb"
      "6ff5b1d7c3ef8d8e3bc2193dfb597cf942c5fc50befa527fac0b44cda2bbb811b06ae874"
      "59750295371cd232754e2bb7132807d1225950ce64949b0650531800bd0074177677acad"
      "937ee008cc0bbfdf33c6b0552000238494be8be412a3e5cfa359e619d092c76310a76bdc"
      "b22abbe6f16b3b116b5f95001d20e42fc3c9ff6723e580f378475788eec265a1ed2087de"
      "8cc2eff72184f73fa5dc6e68a56dcfc85350bccb97135386d5b827c2d9aea065708f5c92"
      "1454d1b9303f21d5adf19e00415acbd86d1e5e42d78505b033a515a435713649c50702f5"
      "4623cbf31469f355c3be2e30dd8c72b4127764451d79e952ea1f9bb0269da56dc07060d5"
      "d9542a9c1258ccefe53fa3f7b6073cd38026256b45c01b6c5dc0d91e3139f30a8d1da7a0"
      "76738f5bb23352693a8e3cbbb46226fa22416680013f9e3278913d06aee4a62457357f0a"
      "68d173a360af5e1411840e34c574b4c6b352f92ce33632911ad8b6710d357b7607ee1967"
      "9e777baffb8ae3c0fe9786b2e97fdeccb5105ecfe81441f549bc6b50ab84b749fb33f8f6"
      "bddcb6bb733d6d5dbc4b29725b8741439b8239e53fa435ea29ed3324202b1bdd07d1987b"
      "0e06d8cb51013dad897ef02401290940ce3f2af72c5d1b4c8836299008c10b16c7e3e119"
      "e41ec66d9db6929ee09bdeaeda08a50665c052edf77b7dff3d8815046bf71d5015e3bdb2"
      "9a4f507aeb2e28c536cdcc9b8d1e89849a0683d78f99dbfa90f94aa5dc08587657a8f042"
      "d718080de5d4a973f232f78c387b63c7143fc2a4380c491414a18b6c4a7bae2194b62e79"
      "8ad7ec7d09e409425f6d0973accb17e4d860f8ec0283584cff076d93bd9b0c4873f9c57c"
      "ddcebe3c3bc8afe793c6cb6b26c4582847b07446b7e1d9757de6bdf0df826cbc502bf88c"
      "f3a773866d3ff293034abc4afa3091b2126a278f50e47f2f66ebebb616e342098ab690f7"
      "f5828bf8cc4742c677d378893e9f188e8397bee983a9a0998de2a31798330f8db59a8581"
      "e1c847589bc0e2d95ffa68e39226cc15cf6cae5c4f5174e7848375391dfabafec202565e"
      "c2383721339f04c5c5d1da953d88f18cda65745ee8e99805e35203a6545a0416923b38c5"
      "db3c8aa00d64354bed27d7c78c4b257534bd7a18107ebe64d8c27b6afdb330d8efba79fd"
      "1fae480cd51fd3626bf8d79fb651b7c6cf752aa737a5123558420d48fc86451b358d270a"
      "acfa6c17f343b7a9956e6f64e4990c1b3f1e5097605edf5ce4247819b19f245e9a90758d"
      "d42c36699ba5cd7f3ed99a7df7eb155749f4b42d192c47cacb6b2865fb9ef2cfca283865"
      "cd06e40cdf7f89d76a9e2eb393e2e0ac0e2776da929f3f8e3d325d075a966d289c51347b"
      "d0bd523a5c81edef63ce9b72f5114c88b08b16edbd73f518096240a5b37421843173be8d"
      "f4ac7c587a17ca6f2916f7d9a10dc75f81bc778a1eb730d12b51555cc414eab9c066113a"
      "7edba9a7f1a18092ae47f12f0368ba211feaf34a3b48a7ff5c91b81cf7c95675a4001c95"
      "a19d284fe4197fe8823909a123fcec5e45935da12416be1bdf14918414ad19b54a41052f"
      "5b8417ddbd207ee01d6a3e62fd9b0321b1c13d91d6ce15ea7b2ea0c670a5f5cb290ca8e6"
      "2c26c6499104ab8e9fafb05170ede246bbf7313625d1fc9576f1609ffd08852a2f4b73c0"
      "4f1f4eeecefe3f3eeb2185a618b6dd3e87d9d3fdcb349cc83c21f26b6c662bbb857aa953"
      "78e991640a160a23cce76153c134508c68ec54a5"_unhex;
  Bytes aad =
      "0d471079ad3c3432b6de852ec71692d12d9df4f984554d458a9dd1f28a2697976da8111a"
      "e4454c9a23d1c8eae75bbc14f8b00e7c065bc290f8938282b91a1a26c22b40a6708c4094"
      "5d087e45633a595beb67d8f1c29a81"_unhex;
  Nonce nonce = asArray<Nonce>("bb2d033de71d570ddf824e85"_unhex);
  Bytes ciphertext =
      "238c4e6be84bfb151557327095c88f6dc2889bce2d6f0329e0c42a5cd7554ab16c8b5a4d"
      "b26eab30f519c24766b1085e11d40823053ca77adfe2af387b4dcde12bc3850222951060"
      "6ff086265f45b1087375dc4a022eb0b641101c74ad566ab6f230133b7aa61861aa8202b6"
      "7beddc30dda506691a42032357010d45adc7ee633b536a2fefb3b2143837bb46db04f66a"
      "6e2bc628d6041b3d306ff78e96205ab66847036efa1fb6e6a387cf8d5a105738be7163df"
      "9da0db48e3d8fd6a786f0f887968e180ad6888e110fb3d7919c42a7f8c92491d795c813f"
      "30ea645fafcddf877f5035f133f864fd0ba1415b3d698f2349ebe03d9e76610355e7fc23"
      "221c5c72b1b2628a40b14badf93288fc4abeaff5306d274f21938650ab236a39496d3f8a"
      "6e9086eac058e365d4335b51eafac813f9175bb7bebb75605909ec3fde6515694e119f7b"
      "6e96aa1d6d6454c3a7dddeacc83bf0c1f5f6c2a9dd2f460f3e5b074a33b8d7904e6988ae"
      "43a22a87f0933f812e45c4c518bf83e606bad4c3c55422ab2207e9d3cfcbc5819049f55e"
      "35b9663273d9d3a6f8a897fa38b0dca77eb6c344290cc007b68d913187f2cd480a402626"
      "23a4e95d90d5701ac2b9d858d70a27f0672f919c2ded1fb89134ac9a8ba6ac62931c8323"
      "72abb70e811dc50cce264ece65e87338231f18ac007c5f68f3b1c5904ffbb2e1dc361d53"
      "914917770d66afe28c547d8cd5896d892cbdadc34cd6af348c93bdb8b072f38b085361e6"
      "2ded7a38b4368824c759ec7d2cf4caddb9191e5deedc8b8388bc4ba2c0672321bcda3a73"
      "43c9ea71ef03750912f35624d81da5fa8a6ee676c4efd99d0c7258b844ded7b35d8c8233"
      "a316b508d79c7c0b3edabad5db9543615179b1c111bfd78b79327ac5b4155336d670baa5"
      "92d441c810cb1b7c07f3d35473a45b57e780b7d997782aeecfc0363976fb608d6967844e"
      "d00b63ba75996054d090aeb605c195b1ff86f9d9ab5892d27632cbb59c06b3ccd69d33ed"
      "5dea9398f00b7c6404fcfe2fcb5924e4cb75cbcae0a1b084ea8b15eaa5847431e9ab70e4"
      "afe15b4c82239f6165e243e3b76d6c91d23b16edecad8bcb16898641f8e323671452034a"
      "8ec9b42b29cec0db210bad0444f1c5bf3505cc41d514d5a270d556f0a34333bd06cd6509"
      "ba253a6ba7a6db8f1a60c99f0c3d566a038a72f1271a178cc3ff890b0df1e7438c0c1a12"
      "d9873643e2d7bfeb92379545de50834abe2a345faf7ca49beeab87ee516dd8598b71196b"
      "8cdb15e7200cb5bd814338babd74c565faaf33d9a8ed4209b417345a1ae611880ea22ab2"
      "e894d5d14a28fe3835d3b2718125f0e6daabd85327455646290ceab89e579ed5e1d72a01"
      "72e4a6d8da70290b5022c941f3866f96cc4218de5d2622d13af6dab15760a1ec5d109182"
      "67f9585284058aba611ba07b1d5711cef505869831699bedc2b190fe1d578814065c91d8"
      "7a8c8dc9b0d4dae0c80cd241f0bda3a6d5e714c894b7a48b1e5eed4555f103eb03c9db30"
      "efcb855df422d7451a6d70f28174c7ebff536dd2cd2891f6c3f264d632ca924c4e0d84b3"
      "7cf8e06e6f2e29efac6cf008cc27f062441278dbc9f09cf44987e0e9ca088a48437b0b89"
      "efb9cf00d3d0c5fb449fd4b64e21dc48cf300c2d80a502cb583219f1881e78e647783d91"
      "dd2f3b389a1594eefd8ea07d4786f983d13e33cf7a34e4c9a0ec4b791f1666a4eef4e63b"
      "de7a241f49b5cf615888bd8130743bc8a6d502bfc73ab64d1184ead9a611832b7e24483a"
      "1a0fc475d9ff6166b86a18a3dc96910ff182cf326456c4461ce8acb3467f801890eaf1ce"
      "0b24791da9c650876e718c0bf43c475174f9712dd4a228695e8f8b2b23fc4a06358b4a6a"
      "8e1afa87a0280c3e098f218f7a6d6bd716f8c105a7eb799ba0220837fa5a96c8a22a826a"
      "6f7ea9d7216a24acbc7b0133210cc17c8190507badb421bc54997ff9340cdc1ee415126a"
      "c46a4fec9fee12d40f06300f7e397b228250f36d6f0d2ddad5fe1898ea690e4c7cc3a116"
      "a70bfaf6d2dc996753fffae40ba5280b8356b7ab4ffbc914ec74eaa070581fdd1d9e5aa"
      "2"_unhex;
};

/**
 * @given an implementation of CCP cipher
 * @when a test string gets encrypted and then decrypted
 * @then the result is equal to the test string
 */
TEST_F(ChaChaPolyTest, Basic) {
  ChaCha20Poly1305Impl codec(key);
  Bytes data = "afafafdeadbeef"_unhex;
  EXPECT_OUTCOME_TRUE(ciphertext, codec.encrypt(nonce, data, aad))
  EXPECT_OUTCOME_TRUE(plaintext, codec.decrypt(nonce, ciphertext, aad))
  ASSERT_EQ(data, plaintext);
}

/**
 * @given CCP encoder implementation
 * @when the predefined input is processed
 * @then the encryption result equals to expected
 */
TEST_F(ChaChaPolyTest, Encrypt) {
  ChaCha20Poly1305Impl codec(key);

  EXPECT_OUTCOME_TRUE(result, codec.encrypt(nonce, plaintext, aad));
  ASSERT_EQ(result, ciphertext);
}

/**
 * @given CCP decoder implementation
 * @when the predefined input is processed
 * @then the decryption result equals to expected
 */
TEST_F(ChaChaPolyTest, Decrypt) {
  ChaCha20Poly1305Impl codec(key);

  EXPECT_OUTCOME_TRUE(result, codec.decrypt(nonce, ciphertext, aad));
  ASSERT_EQ(result, plaintext);
}
