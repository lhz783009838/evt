/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <functional>
#include <boost/signals2/signal.hpp>
#include <evt/chain/block_state.hpp>
#include <evt/chain/trace.hpp>
#include <evt/chain/contracts/genesis_state.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>

namespace chainbase {
class database;
}

namespace evt { namespace chain {

class fork_database;
class token_database;
class apply_context;

struct controller_impl;
using boost::signals2::signal;
using chainbase::database;
using contracts::genesis_state;
using contracts::abi_serializer;

class dynamic_global_property_object;
class global_property_object;
using apply_handler = std::function<void(apply_context&)>;

class controller {
public:
    struct config {
        path     block_log_dir          = chain::config::default_block_log_dir;
        path     tokendb_dir            = chain::config::default_tokendb_dir;
        uint64_t unconfirmed_cache_size = chain::config::default_unconfirmed_cache_size;;
        path     shared_memory_dir      = chain::config::default_shared_memory_dir;
        uint64_t shared_memory_size     = chain::config::default_shared_memory_size;
        bool     read_only              = false;

        genesis_state genesis;
    };

    controller(const config& cfg);
    ~controller();

    void startup();

    /**
          * Starts a new pending block session upon which new transactions can
          * be pushed.
          */
    void start_block(block_timestamp_type time = block_timestamp_type(), uint16_t confirm_block_count = 0);

    void abort_block();

    /**
          *  These transactions were previously pushed by have since been unapplied, recalling push_transaction
          *  with the transaction_metadata_ptr will remove them from the source of this data IFF it succeeds.
          *
          *  The caller is responsible for calling drop_unapplied_transaction on a failing transaction that
          *  they never intend to retry
          *
          *  @return vector of transactions which have been unapplied
          */
    vector<transaction_metadata_ptr> get_unapplied_transactions() const;
    void                             drop_unapplied_transaction(const transaction_metadata_ptr& trx);

    /**
          *
          */
    transaction_trace_ptr push_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline);

    void finalize_block();
    void sign_block(const std::function<signature_type(const digest_type&)>& signer_callback);
    void commit_block();
    void pop_block();

    void push_block(const signed_block_ptr& b, bool trust = false /* does the caller trust the block*/);

    /**
          * Call this method when a producer confirmation is received, this might update
          * the last bft irreversible block and/or cause a switch of forks
          */
    void push_confirmation(const header_confirmation& c);

    chainbase::database& db() const;
    fork_database& fork_db() const;
    token_database& token_db() const;

    const global_property_object&         get_global_properties() const;
    const dynamic_global_property_object& get_dynamic_global_properties() const;

    uint32_t            head_block_num() const;
    time_point          head_block_time() const;
    block_id_type       head_block_id() const;
    account_name        head_block_producer() const;
    const block_header& head_block_header() const;
    block_state_ptr     head_block_state() const;

    time_point      pending_block_time() const;
    block_state_ptr pending_block_state() const;

    const producer_schedule_type&    active_producers() const;
    const producer_schedule_type&    pending_producers() const;
    optional<producer_schedule_type> proposed_producers() const;

    uint32_t      last_irreversible_block_num() const;
    block_id_type last_irreversible_block_id() const;

    signed_block_ptr fetch_block_by_number(uint32_t block_num) const;
    signed_block_ptr fetch_block_by_id(block_id_type id) const;

    block_id_type get_block_id_for_num(uint32_t block_num) const;

    void validate_expiration(const transaction& t) const;
    void validate_tapos(const transaction& t) const;

    bool set_proposed_producers(vector<producer_key> producers);

    signal<void(const block_state_ptr&)>          accepted_block_header;
    signal<void(const block_state_ptr&)>          accepted_block;
    signal<void(const block_state_ptr&)>          irreversible_block;
    signal<void(const transaction_metadata_ptr&)> accepted_transaction;
    signal<void(const transaction_trace_ptr&)>    applied_transaction;
    signal<void(const header_confirmation&)>      accepted_confirmation;

    flat_set<public_key_type> get_required_keys(const transaction& trx, const flat_set<public_key_type>& candidate_keys) const;

    const apply_handler* find_apply_handler(action_name act) const;

    const abi_serializer& get_abi_serializer() const;

    template <typename T>
    fc::variant
    to_variant_with_abi(const T& obj) {
        // TODO: Remove account parameter
        fc::variant pretty_output;
        abi_serializer::to_variant(obj, pretty_output, [&] { return get_abi_serializer(); });
        return pretty_output;
    }

private:
    std::unique_ptr<controller_impl> my;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::controller::config,
           (block_log_dir)(tokendb_dir)(unconfirmed_cache_size)(shared_memory_dir)(shared_memory_size)(read_only)(genesis))