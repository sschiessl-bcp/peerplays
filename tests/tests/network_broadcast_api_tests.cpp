/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

namespace
{
    transfer_operation make_transfer_operation(const account_id_type& from, const account_id_type& to, const asset& amount)
    {
        transfer_operation transfer;
        transfer.from = from;
        transfer.to   = to;
        transfer.amount = amount;
        
        return transfer;
    }
    
    void create_proposal(database_fixture& fixture, const std::vector<operation>& operations)
    {
        signed_transaction transaction;
        set_expiration(fixture.db, transaction);
        
        transaction.operations = operations;
        
        fixture.db.create<proposal_object>([&](proposal_object& proposal)
        {
            proposal.proposed_transaction = transaction;
        });
    }
    
    signed_transaction make_signed_transaction_with_proposed_operation(database_fixture& fixture, const std::vector<operation>& operations)
    {
        proposal_create_operation operation_proposal;
        
        for (auto& operation: operations)
        {
            operation_proposal.proposed_ops.push_back(op_wrapper(operation));
        }
        
        signed_transaction transaction;
        set_expiration(fixture.db, transaction);
        transaction.operations = {operation_proposal};
        
        return transaction;
    }
}

BOOST_FIXTURE_TEST_SUITE( check_tansaction_for_duplicated_operations, database_fixture )

BOOST_AUTO_TEST_CASE( test_exception_throwing_for_the_same_operation_proposed_twice )
{
    try
    {
        ACTORS((alice))
        
        create_proposal(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))});
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))});
        BOOST_CHECK_THROW(db.check_tansaction_for_duplicated_operations(trx), fc::exception);
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( check_passes_without_duplication )
{
    try
    {
        ACTORS((alice))
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))});
        BOOST_CHECK_NO_THROW(db.check_tansaction_for_duplicated_operations(trx));
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( check_passes_for_the_same_operation_with_different_assets )
{
    try
    {
        ACTORS((alice))
        
        create_proposal(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))});
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(501))});
        BOOST_CHECK_NO_THROW(db.check_tansaction_for_duplicated_operations(trx));
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( check_fails_for_duplication_in_transaction_with_several_operations )
{
    try
    {
        ACTORS((alice))
        
        create_proposal(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))});
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(501)),
                                                                           make_transfer_operation(account_id_type(), alice_id, asset(500))}); //duplicated one
        BOOST_CHECK_THROW(db.check_tansaction_for_duplicated_operations(trx), fc::exception);
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( check_fails_for_duplicated_operation_in_existed_proposal_with_several_operations_and_transaction_with_several_operations )
{
    try
    {
        ACTORS((alice))
        
        create_proposal(*this, {make_transfer_operation(account_id_type(), alice_id, asset(499)),
                                make_transfer_operation(account_id_type(), alice_id, asset(500))}); //duplicated one
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(501)),
                                                                           make_transfer_operation(account_id_type(), alice_id, asset(500))}); //duplicated one
        BOOST_CHECK_THROW(db.check_tansaction_for_duplicated_operations(trx), fc::exception);
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE( check_fails_for_duplicated_operation_in_existed_proposal_with_several_operations )
{
    try
    {
        ACTORS((alice))
        
        create_proposal(*this, {make_transfer_operation(account_id_type(), alice_id, asset(499)),
                                make_transfer_operation(account_id_type(), alice_id, asset(500))}); //duplicated one
        
        auto trx = make_signed_transaction_with_proposed_operation(*this, {make_transfer_operation(account_id_type(), alice_id, asset(500))}); //duplicated one
        BOOST_CHECK_THROW(db.check_tansaction_for_duplicated_operations(trx), fc::exception);
    }
    catch( const fc::exception& e )
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
