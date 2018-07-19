#include "BlockchainTests.h"

#include "blockchain.h"
#include "crypto.h"
#include "schnorr.h"
#include "consensus/regtest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(BlockchainTest);

BlockchainTest::BlockchainTest() {
    log.reset(new CryptoKernel::Log("tests.log"));
}

BlockchainTest::~BlockchainTest() {}

void BlockchainTest::setUp() {
    blockchain.reset(new testChain(log.get()));
    consensus.reset(new CryptoKernel::Consensus::Regtest(blockchain.get()));
    blockchain->loadChain(consensus.get(), "genesistest.json");
    consensus->start();
}

void BlockchainTest::tearDown() {
    blockchain.reset();
    consensus.reset();
    std::remove("genesistest.json");
    CryptoKernel::Storage::destroy("./testblockdb");
}

BlockchainTest::testChain::testChain(CryptoKernel::Log* GlobalLog) : CryptoKernel::Blockchain(GlobalLog, "./testblockdb") {}

BlockchainTest::testChain::~testChain() {}

std::string BlockchainTest::testChain::getCoinbaseOwner(const std::string& publicKey) {
    return publicKey;
}

uint64_t BlockchainTest::testChain::getBlockReward(const uint64_t height) {
    return 100000000;
}

void BlockchainTest::testVerifyMalformedSignature() {
    consensus->mineBlock(true, "BL2AcSzFw2+rGgQwJ25r7v/misIvr3t4JzkH3U1CCknchfkncSneKLBo6tjnKDhDxZUSPXEKMDtTU/YsvkwxJR8=");

    const auto block = blockchain->getBlockByHeight(2);

    const auto output = *block.getCoinbaseTx().getOutputs().begin();

    CryptoKernel::Blockchain::output outp(output.getValue() - 20000, 0, Json::nullValue);

    Json::Value spendData;
    Json::Value randomData;
    randomData["this is"] = "malformed";

    // Something malformed (not a string)
    spendData["signature"] = randomData;

    CryptoKernel::Blockchain::input inp(output.getId(), spendData);

    CryptoKernel::Blockchain::transaction tx({inp}, {outp}, 1530888581);

    const auto res = blockchain->submitTransaction(tx);

    CPPUNIT_ASSERT(!std::get<0>(res));
}

void BlockchainTest::testListUnspentOutputsFromCoinbase() {
    const std::string pubKey = "BL2AcSzFw2+rGgQwJ25r7v/misIvr3t4JzkH3U1CCknchfkncSneKLBo6tjnKDhDxZUSPXEKMDtTU/YsvkwxJR8=";

    // Mine three blocks
    for(int i = 0; i < 3; i++) {    
        consensus->mineBlock(true, pubKey);
    }

    // Ensure three blocks were actually mined
    blockchain->getBlockByHeight(4);

    const auto outs = blockchain->getUnspentOutputs(pubKey);

    CPPUNIT_ASSERT_EQUAL(size_t(3), outs.size());
    
    for(const auto& out : outs) {
        CPPUNIT_ASSERT_EQUAL(pubKey, out.getData()["publicKey"].asString());
        CPPUNIT_ASSERT_EQUAL(uint64_t(100000000), out.getValue());
    }
}

void BlockchainTest::testSingleSchnorrSignature() {
    CryptoKernel::Crypto crypto(true);

    const auto ECDSAPubKey = crypto.getPublicKey();

    consensus->mineBlock(true, ECDSAPubKey);

    const auto outs = blockchain->getUnspentOutputs(ECDSAPubKey);
    const auto& out = *outs.begin();

    CryptoKernel::Schnorr schnorr;

    Json::Value outData;
    outData["schnorrKey"] = schnorr.getPublicKey();

    CryptoKernel::Blockchain::output out2(out.getValue() - 20000, 0, outData);

    const std::string outputSetId = CryptoKernel::Blockchain::transaction::getOutputSetId({out2}).toString();

    Json::Value spendData;
    spendData["signature"] = crypto.sign(out.getId().toString() + outputSetId);

    CryptoKernel::Blockchain::input inp(out.getId(), spendData);
    CryptoKernel::Blockchain::transaction tx({inp}, {out2}, 1530888581);

    const auto res = blockchain->submitTransaction(tx);
    CPPUNIT_ASSERT(std::get<0>(res));

    consensus->mineBlock(true, ECDSAPubKey);

    Json::Value outData2;
    outData2["publicKey"] = "BL2AcSzFw2+rGgQwJ25r7v/misIvr3t4JzkH3U1CCknchfkncSneKLBo6tjnKDhDxZUSPXEKMDtTU/YsvkwxJR8=";
    CryptoKernel::Blockchain::output out3(out2.getValue() - 20000, 0, outData2);

    const auto outputSetId2 = CryptoKernel::Blockchain::transaction::getOutputSetId({out3}).toString();

    Json::Value spendData2;
    spendData2["signature"] = schnorr.sign(out2.getId().toString() + outputSetId2);
    CryptoKernel::Blockchain::input inp2(out2.getId(), spendData2);
    CryptoKernel::Blockchain::transaction tx2({inp2}, {out3}, 1530888581);

    const auto res2 = blockchain->submitTransaction(tx2);
    CPPUNIT_ASSERT(std::get<0>(res2));

    consensus->mineBlock(true, ECDSAPubKey);

    const auto outs2 = blockchain->getUnspentOutputs(outData2["publicKey"].asString());
    CPPUNIT_ASSERT_EQUAL(size_t(1), outs2.size());
}
