#include "libdnf/rpm/nevra.hpp"
#include "libdnf/transaction/rpm_package.hpp"
#include "libdnf/transaction/transaction.hpp"
#include "libdnf/transaction/Transformer.hpp"

#include "TransactionTest.hpp"

using namespace libdnf::transaction;

CPPUNIT_TEST_SUITE_REGISTRATION(TransactionTest);

void
TransactionTest::setUp()
{
    conn = new libdnf::utils::SQLite3(":memory:");
    Transformer::createDatabase(*conn);
}

void
TransactionTest::tearDown()
{
    delete conn;
}

void
TransactionTest::testInsert()
{
    Transaction trans(*conn);
    trans.set_dt_begin(1);
    trans.set_dt_end(2);
    trans.set_rpmdb_version_begin("begin - TransactionTest::testInsert");
    trans.set_rpmdb_version_end("end - TransactionTest::testInsert");
    trans.set_releasever("26");
    trans.set_user_id(1000);
    trans.set_cmdline("dnf install foo");
    trans.set_state(TransactionState::DONE);

    trans.add_runtime_package("rpm-4.14.2-1.fc29.x86_64");
    trans.add_runtime_package("dnf-3.5.1-1.fc29.noarch");
    // test adding a duplicate; only a single occurrence of the rpm is expected
    trans.add_runtime_package("rpm-4.14.2-1.fc29.x86_64");

    trans.begin();

    // get_runtime_packages returns results directly from the db
    // that's why it has to be called after begin(), where the records are saved
    CPPUNIT_ASSERT_EQUAL(2ul, trans.get_runtime_packages().size());

    // 2nd begin must throw an exception
    CPPUNIT_ASSERT_THROW(trans.begin(), std::runtime_error);

    // load the saved transaction from database and compare values
    Transaction trans2(*conn, trans.get_id());
    CPPUNIT_ASSERT(trans2.get_id() == trans.get_id());
    CPPUNIT_ASSERT(trans2.get_dt_begin() == trans.get_dt_begin());
    CPPUNIT_ASSERT(trans2.get_dt_end() == trans.get_dt_end());
    CPPUNIT_ASSERT(trans2.get_rpmdb_version_begin() == trans.get_rpmdb_version_begin());
    CPPUNIT_ASSERT(trans2.get_rpmdb_version_end() == trans.get_rpmdb_version_end());
    CPPUNIT_ASSERT(trans2.get_releasever() == trans.get_releasever());
    CPPUNIT_ASSERT(trans2.get_user_id() == trans.get_user_id());
    CPPUNIT_ASSERT(trans2.get_cmdline() == trans.get_cmdline());
    CPPUNIT_ASSERT(trans2.get_state() == trans.get_state());
    CPPUNIT_ASSERT_EQUAL(2ul, trans2.get_runtime_packages().size());
}

void
TransactionTest::testInsertWithSpecifiedId()
{
    // it is not allowed to save a transaction with arbitrary ID
    Transaction trans(*conn);
    trans.set_id(INT64_MAX);
    CPPUNIT_ASSERT_THROW(trans.begin(), std::runtime_error);
}

void
TransactionTest::testUpdate()
{
    Transaction trans(*conn);
    trans.set_dt_begin(1);
    trans.set_dt_end(2);
    trans.set_rpmdb_version_begin("begin - TransactionTest::testInsert");
    trans.set_rpmdb_version_end("end - TransactionTest::testInsert");
    trans.set_releasever("26");
    trans.set_user_id(1000);
    trans.set_cmdline("dnf install foo");
    trans.set_state(TransactionState::ERROR);
    trans.begin();
    trans.finish(TransactionState::DONE);

    Transaction trans2(*conn, trans.get_id());
    CPPUNIT_ASSERT(trans2.get_id() == trans.get_id());
    CPPUNIT_ASSERT(trans2.get_dt_begin() == trans.get_dt_begin());
    CPPUNIT_ASSERT(trans2.get_dt_end() == trans.get_dt_end());
    CPPUNIT_ASSERT(trans2.get_rpmdb_version_begin() == trans.get_rpmdb_version_begin());
    CPPUNIT_ASSERT(trans2.get_rpmdb_version_end() == trans.get_rpmdb_version_end());
    CPPUNIT_ASSERT(trans2.get_releasever() == trans.get_releasever());
    CPPUNIT_ASSERT(trans2.get_user_id() == trans.get_user_id());
    CPPUNIT_ASSERT(trans2.get_cmdline() == trans.get_cmdline());
    CPPUNIT_ASSERT_EQUAL(TransactionState::DONE, trans2.get_state());
}

void
TransactionTest::testComparison()
{
    // test operator ==, > and <
    Transaction first(*conn);
    Transaction second(*conn);

    // id comparison test
    first.set_id(1);
    second.set_id(2);
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // begin timestamp comparison test
    second.set_id(1);
    first.set_dt_begin(1);
    second.set_dt_begin(2);
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // rpmdb comparison test
    second.set_dt_begin(1);
    first.set_rpmdb_version_begin("0");
    second.set_rpmdb_version_begin("1");
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // equality
    second.set_rpmdb_version_begin("0");
    CPPUNIT_ASSERT(first == second);
}
