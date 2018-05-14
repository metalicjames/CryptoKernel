#include "SchnorrTests.h"

CPPUNIT_TEST_SUITE_REGISTRATION(SchnorrTest);

SchnorrTest::SchnorrTest() {
}

SchnorrTest::~SchnorrTest() {
}

void SchnorrTest::setUp() {
    schnorr = new CryptoKernel::Schnorr();
}

void SchnorrTest::tearDown() {
    delete schnorr;
}

/**
* Tests that the Crypto module initialised correctly
*/
void SchnorrTest::testInit() {
    CPPUNIT_ASSERT(schnorr->getStatus());
}

/**
* Tests that a keypair has been generated
*/
void SchnorrTest::testKeygen() {
    const std::string privateKey = schnorr->getPrivateKey();
    const std::string publicKey = schnorr->getPublicKey();

    CPPUNIT_ASSERT(privateKey.size() > 0);
    CPPUNIT_ASSERT(publicKey.size() > 0);
}

/**
* Tests that signing and verifying works
*/
void SchnorrTest::testSignVerify() {
    const std::string privateKey = schnorr->getPrivateKey();
    const std::string publicKey = schnorr->getPublicKey();

    const std::string signature = schnorr->sign(plainText);

    CPPUNIT_ASSERT(signature.size() > 0);
    CPPUNIT_ASSERT(schnorr->verify(plainText, signature));
}

/**
* Tests passing key to class
*/
void SchnorrTest::testPassingKeys() {
    CryptoKernel::Schnorr *tempSchnorr = new CryptoKernel::Schnorr();

    const std::string signature = tempSchnorr->sign(plainText);
    CPPUNIT_ASSERT(signature.size() > 0);

    CPPUNIT_ASSERT(schnorr->setPublicKey(tempSchnorr->getPublicKey()));
    CPPUNIT_ASSERT(schnorr->verify(plainText, signature));

    delete tempSchnorr;
}
