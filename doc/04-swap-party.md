SIDEPOOL-04 Swap Party Protocol

Overview
========

Sidepool version 1 pools are operated in sessions called "*swap
parties*".
The pool leader declares the start of a swap party, and the pool
participants then arrange for protocols on top of sidepools to
occur, in order to manage the liquidity of the Lightning Network
channels.

It is called a "*swap party*" since the expected use of sidepools
is for Lightning Network nodes to initiate swaps between the
sidepool and individual channels in order to manage their
liquidity.
Multiple swaps are thus expected to occur simultaneously in a
single swap party.
The swap party is used so that:

* Multiple swaps can occur at each update of the pool mechanism;
  the pool state mechanism is fairly heavyweight and has a
  limited number of updates, so batching multiple swaps together
  in a single update of the state mechanism is beneficial.
* A regular swap party cadence is recommended, so that human node
  operators can ensure that their node is online at the time
  of a swap party, and can bring their node offline for regular
  maintenance or updates when a swap party is not expected to
  occur soon.

A swap party is composed of at least two phases, with an optional
third phase that is also declared by the pool leader.
The third phase is mandatory if the pool update counter would
reach the maximum value after the two "normal" phases.

The phases are:

* Expansion Phase - Participants spend their funds to new
  transaction outputs.
  * This is intended to be used by participants to offer HTLCs
    to other participants.
* Contraction Phase - Participants consolidate their funds to
  fewer transaction outputs.
  * This is intended to be used by participants to claim HTLCs
    offered by other participants, or to reclaim rejected HTLCs.
* Reseat Phase - Participants create an onchain transaction to
  reset the pool update counter, and optionally add or remove
  funds and/or participants from the pool.

This specification primarily discusses the Expansion and
Contraction Phases.
Details of the Reseat Phase are on its own document (TODO).

Scheduling
----------

As mentioned above, swap parties are done at a regular real-world
cadence, so that human operators can schedule maintenance or
update of the hardware or software of the node at a time when no
swap party is expected to occur.

The pool leader is free to select this cadence.
However, for conformity, pool leaders SHOULD schedule swap
parties for production sidepools daily, at 00:00 UTC.

In case of a regular production sidepool schedule, the pool
leader SHOULD start one swap party at any time from 00:00 UTC to
00:30 UTC each day.
The total allowed time for a swap party is up to 30 minutes
(ideally, swap parties will take much less time than that, but in
the worst case that is the allowed delay).

> **Rationale** If all sidepools have the same scheduled cadence
> for swap parties, then the human operators of a node that
> happens to be a participant in multiple sidepools only need to
> arrange maintenance windows around that single cadence.
>
> At the same time, for testing and evaluation purposes, it may
> be useful for a pool leader to simply declare the start of a
> swap party at any time.
> Thus, the pool leader is allowed to select when to start a
> swap party.
>
> Some leeway is given for regular production schedule in order
> to reduce "thundering herd" problems.
> For example, if a pool leader is leading two different pools,
> it can schedule one to start swap parties at 00:00 UTC and the
> other to start at 00:15 UTC to spread out the network bandwidth
> and other resources.
>
> Finally, the human operators of sidepool-using nodes may agree
> to temporarily suspend swap parties for a pool for multi-day
> periods.
> For example, there may be a major update in Lightning Network
> node software, and for safety, swap parties may be suspended
> until all the human operators of affected nodes are satisfied
> that the update has completed and problems are resolved.
> Such suspension is arranged out-of-band to this specification;
> (**non-normative**) actual implementations should support this
> kind of suspension, as well as triggering swap parties "early".

Non-Persistent MuSig2
---------------------

The swap party requires two MuSig2 signing session, one for the
Expansion Phase, another for the Contraction Phase.

During a MuSig2 signing session, participants need to generate
and retain a structure, called `secnonce` in [BIP-327 Nonce
Generation][].
Participants first need to generate `secnonce`, then derive a
structure called `pubnonce`, the latter of which is then sent to
the pool leader, which aggregates the `pubnonce`s into an
`aggnonce`.
The pool leader then broadcasts the `aggnonce` to the pool
followers, which then need the `secnonce` to generate their
partial signatures.

[BIP-327 Nonce Generation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Nonce_Generation-2

Pool participants MUST NOT store the `secnonce`, or equivalent
structure, in persistent storage.
Pool participants SHOULD store these structures in non-swappable
memory.

> **Rationale** If a `secnonce`-equivalent is stored on-disk, and
> then that participant crashes, the human operator may
> accidentally recover a snapshot of the `secnonce`-equivalent
> from before the software has sent a partial signature, even
> though the software has actually already sent the partial
> signature.
> If a counterparty detects this situation, then it could induce
> the software to send a different partial signature for a
> different version of the transaction, but with the same nonces,
> allowing exflitration of the private key.
>
> This is in contrast with plans for Lightning Network Taproot
> channels, where the nonces for the next signing session are
> sent with the partial signatures of the current signing
> session, implying that MuSig2 signing rituals are stored in
> persistent storage.
> This makes more sense for Lightning Network channels in order
> to reduce latency and thereby increase actions-per-second.
> In the case of sidepools, swap parties are once-a-day events
> and extra latency is immaterial compared to the rarity of swap
> parties.

Pool participants MUST persistently store only the completed
MuSig2 aggregate signature after it has locally validated that
the aggregate signatures are valid.

This also implies that nonce selection occurs at the start of
each phase, and the MuSig2 signing session only persists for
the duration of that phase.

A final implication is that in case of a crash before the
aggregate signature can be computed, the software cannot recover
the `secnonce`, which is required to generate a partial signature
(and from there, the aggregate signature).
This implies that in case of a crash during a swap party, the
only recovery is to abort the entire sidepool.

> **Rationale** A sidepool abort due to a crash during a swap
> party is expected to be rare due to swap parties being rare,
> once-a-day events.
> More complex crash recovery is possible, but these increase the
> risk of private key exfiltration, as noted above.

Swap Party Flow
===============

When the pool leader decides to start a swap party:

* It first ensures a connection to all pool followers via the
  Lightning Network wire protocol.
* The pool leader announces the start of the first phase, the
  Expansion Phase, of the swap party.
* The pool participants provide change proposals for the
  Expansion Phase, i.e. they indicate one of their funds to
  be split into multiple outputs.
  * They also share the MuSig2 nonces to be used for signing of
    the next state.
* The pool leader provides the next state that will be created
  for the Expansion Phase, and the pool followers validate that
  the outputs they expect are in the next state.
  * The pool leader also provides the aggregated MuSig2 nonces to
    be used for signing.
* The pool followers contribute their individual partial
  signatures for each affected update transaction.
* The pool leader aggregates the partial signatures and
  broadcasts the aggregate signatures to the pool followers.
  * Once a pool follower has received the aggregate signatures
    and validated that they indeed compose a valid Schnorr
    signature for the transactions affected, the pool follower
    can now treat any new outputs in the new state as irrevocably
    committed.
* The pool participants provide change proposals for the
  Contraction Phase, i.e. they indicate one or more funds to be
  merged into a single fund.
  * They also share the MuSig2 nonces to be used for signing of
    the next state.
* The pool leader provides the next state that will be created
  for the Contraction Phase, and thte pool folowers validate that
  the outputs they expect are in the next state.
  * The pool leader also provides the aggregated MuSig2 nonces to
    be used for signing.
* The pool followers contribute their individual partial
  signature for the last update transaction.
* The pool leader aggregates the partial signatures and
  broadcasts the aggregate signature to the pool followers.
  * At this point, the pool leader indicates if a succeeding
    Reseat Phase will be performed as well.
* If a Reseat Phase will be performed, then the flow enters
  Reseat Phase; see the documentation on reseat and splicing
  (TODO).

While we separate the pool leader from the pool followers in the
specification as a whole, the pool leader is also one of the
participants, and thus also contributes its own partial
signatures for all update transactions.
The pool leader may also need to split out its own funds during
the Expansion Phase and merge its funds during the Contraction
Phase.
(**Non-normative**) This may be implemented by having the pool
leader use an internal message queue that includes messages from
both pool followers and the higher-level-protocol implementation
running on the pool leader.
Code for higher-level protocols would directly push to that queue
if running on a pool leader, but would instead send messages if
running on a pool follower.

Timeouts And Retransmissions
----------------------------

All participants MUST impose timeouts:

* Each phase can take no more than 10 minutes.
  * If a phase takes longer than 10 minutes, the participant MUST
    abort the entire pool.
    Higher-level protocols MUST then switch to onchain
    enforcement.
  * Pool leaders measure from the time they are able to broadcast
    the message that starts the phase, to the time they are able
    to broadcast the message that ends the phase.
  * Pool followers measure from the time they receive the starting
    message to the time they receive the ending message.
* For the Expansion Phase:
  * The timeout begins at `swap_party_begin` message sent from
    the pool leader to pool followers.
  * The timeout ends at `swap_party_expand_complete` message
    sent from the pool leader to pool followers.
* For the Contraction Phase:
  * The timouet begins at `swap_party_expand_complete` message
    sent from the pool leader to pool followers.
  * The timeout ends at `swap_party_contract_complete` message
    sent from the pool leader to pool followers.

In case of a timeout, the sidepool is aborted.

Network is unreliable, thus, it is possible for pool leaders and
pool followers to see a disconnection of their Lightning Network
connections, but become able to reconnect before the above
timeout expires.

When a connection breaks, it is possible that one end of the
connection believes it has completely sent a message, but the
other end of the connection has not received it.
Thus, on reconnection, both ends need to reestablish any
interrupted multi-round communication protocol.

Messages related to swap parties come in pairs, with a message
broadcasted by the pool leader to all followers, and the pool
leader waiting for responses from each follower.

On reconnection, if the pool leader has most recently broadcast
some message, and has not seen the pool follower respond, the
pool leader MUST re-send that message.
If the pool leader has received a response from that pool
follower, it SHOULD NOT re-send that message.

Pool followers correspondingly MUST retain the most recent
message they sent to the pool leader, and if there is a
reconnection and the pool leader re-sends the previous message,
the pool follower MUST re-send the response.

The message pairs are:

| Pool Leader Broadcasts      | Pool Follower Responds        |
|-----------------------------|-------------------------------|
| `swap_party_begin`          | `swap_party_expand_request`   |
| `swap_party_expand_state`   | `swap_party_expand_sign`      |
| `swap_party_expand_done`    | `swap_party_contract_request` |
| `swap_party_contract_state` | `swap_party_contract_sign`    |
| `swap_party_contract_done`  | `swap_party_done_ack`         |

MuSig2 requires two communication rounds, and a swap party has by
default two phases, each with its own MuSig2 signing session.
The "extra" communication round at the start is effectively the
signal to each participant to initiate any higher-level protocols
used to manage liquidity.

Sequence Diagram
----------------

    Leader                         Follower
       |                               |
       |        swap_party_begin       |
       |------------------------------>|
       |                               |
       |   swap_party_expand_request   |
       |<------------------------------|
       |                               |
       |                               |
       |    swap_party_expand_state    |
       |------------------------------>|
       |                               |
       |     swap_party_exaand_sign    |
       |<------------------------------|
       |                               |
       |                               |
       |     swap_party_expand_done    |
       |------------------------------>|
       |                               |
       |  swap_party_contract_request  |
       |<------------------------------|
       |                               |
       |                               |
       |   swap_party_contract_state   |
       |------------------------------>|
       |                               |
       |    swap_party_contract_sign   |
       |<------------------------------|
       |                               |
       |                               |
       |    swap_party_contract_done   |
       |------------------------------>|
       |                               |
       |      swap_party_done_ack      |
       |<------------------------------|
       |                               |

Connection
----------

The pool leader MUST first ensure that it can, at minimum,
connect to all pool followers, before sending *any* swap party
messages.

If a pool follower has no connection, and the pool leader is
unable to even complete the BOLT8 handshake for that pool
follower in some reasonable number of seconds (recommended
30 seconds), then the pool leader MAY decide to defer the swap
party for up to 30 minutes, or until the next scheduled period.

Alternately, the pool leader MAY abort the entire pool, i.e.
initiate a unilateral close by itself.
See Setup And Teardown (TODO) for how aborts are handled.

> **Rationale** For example, the pool leader may defer a swap
> party once or a few times, but if too many deferments happen in
> sequence then the sidepool has become pointless as it cannot be
> used to manage liquidity, and it may be better to just abort
> the entire pool.

Swap Party Initiation
---------------------

On initiating a swap party, the pool leader creates a copy of the
current output state and labels it the next output state.

The pool leader then broadcasts the message `swap_party_begin` to
all pool followers.

1.  `swap_party_begin` (400)
    - Sent from pool leader to pool followers.
2.  TLVs:
    * `swap_party_begin_id` (40000)
      - Length: 16
      - Value: The sidepool identifier of the sidepool whose
        swap party begins now.
      - Required.

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the sidepool with the
  given pool identifier.

If the above validation fails, the reeiver ignores the message,
and SHOULD log a warning about the protocol violation.

If validation succeeds, the receiver MUST:

* Trigger any supported higher-level protocols that would help
  it manage its liquidity in the pool and in channels it has with
  other pool participants.
* Respond with a `swap_party_expand_request` message below.

Expansion Phase Requests
------------------------

In response to `swap_party_begin`, pool followers send a
`swap_party_expand_request` message.

The `swap_party_expand_request` message may be a "no-op" request,
where the pool follower only sends the MuSig2 nonces for the
signing of the new post-expansion state.

Otherwise, the `swap_party_expand_request` indicates a single
transaction output to be consmed, and then one or more new
transaction outputs.

1.  `swap_party_expand_request` (402)
    - Sent from pool follower to pool leader, in response to a
      `swap_party_begin` message.
2.  TLVs:
    * `swap_party_expand_request_id` (40200)
      - Length: 16
      - Value: The sidepool identifier of the sidepool indicated
        in the previous `swap_party_begin` message.
      - Required.
    * `swap_party_expand_request_prevout_signed` (40202)
      - Length: 104
      - Value: A structure composed of:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
        - 64 bytes: Taproot signature, as described in
          [BIP-340][].
      - Optional.
    * `swap_party_expand_request_newouts` (40204)
      - Length: N * 40, 1 <= N <= `sidepool_version_1_max_outputs`.
        See [SIDEPOOL-02][] for the maximum N.
      - Value: An array of 40-byte entries:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
      - Required if `swap_party_expand_request_prevout_signed` is
        specified.
        Absent if `swap_party_expand_request_prevout_signed` is
        not specified.
    * `swap_party_expand_request_nonces` (40208)
      - Length: 396 (= 6 * (33 + 33))
      - Value: A length 6 array of entries:
        - 33 bytes: First MuSig2 nonce
        - 33 bytes: Second MuSig2 nonce
      - Required.
    * `swap_party_expand_request_still_contract` (40210)
      - Length: 0
      - Optional.

[BIP-340]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki

`swap_party_expand_request_prevout_signed`, if present, indicates
a request to spend the specified output.
If specified, the pool follower must also specify
`swap_party_expand_request_newouts`.

Each entry in `swap_party_expand_request_newouts` contains a
new output, that is, a tuple of a Taproot X-only Pubkey and an
amount.
Each entry also includes a signature.

The signature in `swap_party_expand_request_prevout_signed`
signs a tagged hash:

    tag = "sidepool version 1 expansion phase"
    content = Next Pool Update Counter ||
              Current X-only Pubkey ||
              Current Amount ||
              value in swap_party_expand_request_newouts

`Next Pool Update Counter` is the pool update counter,
zero-extended from 11 bits to 16 bits, in big-endian order (two
bytes).
The exact value is the one the pool update counter will have at
successful completion of the Expansion Phase.
For the Expansion Phase, it MUST be even, as at the start of the
Expansion Phase it would be odd (the counter starts at 1, and
each swap party involves two increments, thus the Expansion Phase
will always start with the pool update counter odd and will end
with it even).

`Current X-only Pubkey` is the Taproot X-only public key for the
output specified in `swap_party_expand_request_prevout_signed`,
in its 32-byte serialization, and `Current Amount` is the 64-bit
amount in satoshis, in big-endian order (2 bytes).

The signature itself should validate against the `Current X-only
Pubkey`.

`swap_party_expand_request_nonces` are the MuSig2 nonces
generated by the pool follower.
Each entry is either a `pubnonce` from [BIP-327 Nonce
Generation][], or an all-0s entry.

As described in [SIDEPOOL-02][] (TODO: section), for the
Expansion Phase, a variable number of update transactions need to
be recreated, depending on the current value of the pool update
counter.

[SIDEPOOL-02]: ./02-transactions.md

Each entry in `swap_party_expand_request_nonces` corresponds
to an update transaction.
If an update transaction is *not* to be recreated in the current
Expansion Phase, the corresponding entry MUST be all 0 bytes.
Otherwise, if the update transaction *is* to be recrated in the
current Expansion Phase, the corresponding entry must be a valid
`pubnonce`.

For example, if the current pool update counter is 1279 (binary
`10011111111`), then the Second, Third, Fourth, Fifth, and Last
update transactions need to be recreated.
Thus, the 0th entry (corresponding to the First update
transaction) must be all 0s, as the corresponding update
transaction does not need to be updated and new signatures for
the First update transaction are unnecessary.
However, the other entries in the array MUST have valid values.

The entries at indexes 4 and 5, corresponding to the Fifth and
Last update transactions respectively, MUST always have valid
values regardless of the value of the pool update counter, as
those are always updated during the Expansion Phase.

`swap_party_expand_request_still_contract` is a flag.
If present, it indicates to the pool leader that even if nobody
wants to expand, the pool leader MUST NOT broadcast a
cancellation of a swap party (see `swap_party_but_nobody_came`
below).
For example, the participant might not want to create a new
output now, but wants to claim an output created in a *previous*
swap party, thus needs a Contraction Phase.

Receivers of this message MUST validate the following:

* It recognizes that it is the pool leader of the given sidepool
  with the given pool identifier.
* The message sender is a pool follower of the given sidepool
  with the given pool identifier.
* It has recently sent `swap_party_begin` and the sender has not
  sent this response yet.
* If `swap_party_expand_request_prevout_signed` exists:
  * The indicated output currently exists in the current output
    state, and has not been indicated by a different
    `swap_party_expand_request` message by a different pool
    follower.
    * Note that the output state is a bag, not a set.
      In case of duplicates in the current output state, the
      specified output can be indicated multiple times by
      different pool followers, up to the number of duplicates.
  * `swap_party_expand_request_newouts` must exist.
    * The sum total of all amounts is exactly equal to the
      amount specified in
      `swap_party_expand_request_prevout_signed`.
    * Each amount must be greater than or equal to the
      `sidepool_version_1_min_amount` specification in
      [SIDEPOOL-02][] (TODO: section).
    * The signature in `swap_party_expand_request_prevout_signed`
      signs the correct message based on the value of
      `swap_party_expand_request_newouts`.
* The entries in `swap_party_expand_request_nonces` that are
  required are non-0.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

On receiving and validating this message, and if the message
requests an output to spend into new outputs, the pool leader
determines if adding the new outputs will cause the pool output
state to exceed the limit on the number of outputs, as per
[SIDEPOOL-02][] (TODO: section).
If the number of outputs would be exceeded, the pool leader will
not destroy the current output in
`swap_party_expand_request_prevout_signed` and will not create
the outputs indicated in `swap_party_expand_request_newouts`;
the new state will still contain the current output instead of
deleting it.

There is no explicit message sent by the pool leader to tell the
pool follower that its request was rejected due to reaching the
limit.
Instead, the pool follower has to validate the next state, and
determine whether or not its request was fulfilled.

If the pool leader determines that the request will not cause the
output state to exceed the limit specified in [SIDEPOOL-02][],
then the pool leader updates its next output state to remove the
indicated transaction output and create the requested transaction
outputs.

Once the pool leader has received a `swap_party_expand_request`
from each pool follower, it proceeds to the next step.

### Swap Party Cancellation

If:

* None of the participants (including the pool leader) wants to
  spend an existing output to create one or more new outputs.
* And none of the pool followers included the
  `swap_party_expand_request_still_contract` TLV.
* And the pool leader itself also does not want to use the
  upcoming Contraction Phase to merge funds it now controls
  unilaterally.
* And the pool leader has not received any splice-in or
  splice-out requests, which would require a Reseat Phase to
  handle.

...then the pool leader MAY early-out and cancel the swap party
with a `swap_party_but_nobody_came` message:
  
1.  `swap_party_but_nobody_came` (498)
    - Sent from pool leader to pool followers, if there are no
      new outputs to create and nobody expects to use the
      Contraction Phase.
2.  TLVs:
    * `swap_party_but_nobody_came_id` (49800)
      - Length: 16
      - Value: The sidepool ID.
      - Required.

`swap_party_but_nobody_came_id` is the sidepool ID.

Cancelling a swap party simply means that the swap party ends
without any updates in the current state.
The sidepool can still continue operating on future swap parties,
and nothing is published onchain.

> **Rationale** The underlying mechanism is heavyweight and has a
> limited number of updates, and each update increases the total
> fees deducted via the onchain fee contribution tax.
> If nobody wants to perform any swaps during a scheduled swap
> party anyway, then it is better to preserve the limited updates
> for a later time when participants want to actually perform a
> swap.

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the given pool
  identifier.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

The pool follower then checks the following:

* It sent a `swap_party_expand_request` in response to a
  `swap_party_begin` message:
  * The message did not have `swap_party_expand_request_prevout`
     AND did not have `swap_party_expand_request_still_contract`.

If the above validation fails, the pool follower MUST abort the
sidepool.

Expansion Phase State Validation
--------------------------------

The pool leader also aggregates the MuSig2 nonces of all
participants, including fresh MuSig2 nonces for itself, as
per [BIP-327 Nonce Aggregation][].
It aggregates nonces for each update transaction that needs to be
recreated.

[BIP-327 Nonce Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Nonce_Aggregation

The pool leader then broadcasts the `swap_party_expand_state`
message, so that the pool followers can validate the new state.

1.  `swap_party_expand_state` (404)
    - Sent from pool leader to pool followers, once the pool
      leader has received `swap_party_expand_request` from all
      pool followers.
2.  TLVs:
    * `swap_party_expand_state_id` (40400)
      - Length: 16
      - Value: The sidepool identifier of the sidepool whose
        next state post-Expansion is being broadcasted.
      - Required.
    * `swap_party_expand_state_nonces` (40402)
      - Length: 396 (= 6 * (33 + 33))
      - Value: A length 6 array of entries:
        - 33 bytes: First MuSig2 nonce
        - 33 bytes: Second MuSig2 nonce
      - Required.
    * `swap_party_expand_state_beneficiary` (40404)
      - Length: 40
      - Value: A structure:
        - 32 bytes: Taproot X-only Pubkey.
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
          number.
      - Required.
    * `swap_party_expand_state_next` (40406)
      - Length: N * 40, 1 <= N <= `sidepool_version_1_max_outputs`.
        See [SIDEPOOL-02][] for the maximum N.
      - Value: An array of 40-byte entries:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
      - Required.

`swap_party_expand_state_nonces` are the aggregated MuSig2
nonces for each recreated update transaction, i.e. the
`aggnonce` from [BIP-327 Nonce Aggregation][].
Not all entries contain valid data; only the fields that
correspond to update transactions that would be recreated in this
Expansion Phase have valid nonces, and the earlier entries are
all 0s.
See [SIDEPOOL-02][] (TODO: section) for how to determine which
update transactions need to be recreated.

`swap_party_expand_state_beneficiary`  nominates an output of the
next state (a Taproot X-only Pubkey and amount) that will benefit
from the onchain fee contribution overpayment, if the overpayment
is non-zero.

`swap_party_expand_state_next` contains the next output state
generated by the pool leader from the `swap_party_expand_request`
messages from the pool followers.
The array is in the canonical order described in [SIDEPOOl-02][]
(TODO: section).

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the given sidepool
  with the given pool identifier.
* TLV fields `swap_party_expand_state_nonces`,
  `swap_party_expand_state_beneficiary`, and
  `swap_party_expand_state_next` all exist and have valid
  lengths.
* Outputs in `swap_party_expand_state_next` are in canonical
  order defined in [SIDEPOOL-02][] (TODO: section).
* The sum total of all amounts in
  `swap_party_expand_state_next` is equal to the satoshi value
  of the funding transaction output backing the entire pool.
* Each amount in `swap_party_expand_state_next` is greater than
  or equal to `sidepool_version_1_min_amount` defined in
  [SIDEPOOL-02][] (TODO: section).
* The output nominated as overpayment beneficiary in
  `swap_party_expand_state_beneficiary` exists in
  `swap_party_expand_state_next`.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

On receiving and validating this message, the pool follower
computes the onchain fee contribution tax, as well as the onchain
fee contribution overpayment.

The pool follower then checks if any outputs it expects exist in
the given next state:

* If the pool follower did not request to add new outputs in the
  `swap_party_expand_request` message:
  * If the pool follower is relying on the existence of
    particular outputs (e.g. it has funds it controls
    unilaterally in the sidepool, or it has a contract that
    survived previous swap parties), the outputs must still
    exist in the next state.
* If the pool follower requested to add new outputs in the
  `swap_party_expand_request` message:
  * It checks if the previous output it indicated to be spent
    still exists in the next state.
    If so:
    * It checks if the number of outputs in the next state,
      minus one, plus the number of new outputs it requested,
      is greater than `sidepool_version_1_max_outputs`
      specified in [SIDEPOOL-02][].
  * If the previous output no longer exists:
    * It checkes that *all* new outputs it requested now
      exist in the fund.
  * If the pool follower is relying on the existence of
    particular outputs, and it did not indicate those outputs
    as the previous output, the outputs must still exist in
    the next state.

If any of the above checks fail, the pool follower MUST abort the
sidepool, as per SIDEPOOL-03 (TODO).

> **Rationale** The only legitimate reason for an expansion
> request to be denied is if it would cause the sidepool to
> exceed the specified limit.
> Thus, it is an error of the pool leader if it rejects some
> single request, if the limit would not be reached by that
> request.

If the pool follower is participating in a higher-level protocol,
such as a peerswap-in-sidepool, and it expects a new output to
have been created in the sidepool Expansion Phase, it MUST check
for the existence of that output.
However, if the output does not exist, it MUST NOT abort the
sidepool; it MUST abort only the higher-level protocol.

> **Rationale** Participants in the sidepool cannot know if other
> participants will add a large number of outputs during the
> Expansion Phase, and thus participants may have initiated the
> higher-level protocol in good faith, fully intending to
> actually instantiate any needed contract outputs, only for the
> pool leader to deny their request due to hitting the maximum
> number of outputs a sidepool pool can have.
> Thus, other participants in the same higher-level protocol
> need to be tolerant if the output did not exist in the next
> state of the Expansion Phase.

The pool follower then checks if it has already generated
partial signatures in response to some previous
`swap_party_expand_state` message.
If it already has genereated a partial signature, the pool
follower:

* MUST validate that the existing generated partial signatures
  validly sign the recreated update transactions for this set of
  outputs.

If the above validation fails, the pool follower MUST abort the
sidepool.

Otherwise, if the pool follower has not generated partial
signatures for the Expansion Phase of this swap party, the pool
follower generates partial signatures for each recreated update
transaction, using [BIP-327 Signing][].
The pool follower MUST delete its `secnonce` or equivalent
structure (by zeroing out its memory and then freeing it or
dropping all references to it), which it can use to determine if
it already created signatures, and must also retain its partial
signatures in-memory.

[BIP-327 Signing]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Signing

> **Rationale** An attacking pool leader might simulate a
> "network failure" by deliberately disconnecting after receiving
> a partial signature via `swap_party_expand_sign` below, then
> pretending to not have received it and re-sending the
> `swap_party_expand_state` with a different state (such as by
> changing the address of the fund it controls, which is not
> necessarily validated by the pool follower, as the pool
> follower does not own that fund and does not care about it).
> This would cause the pool follower to sign multiple times with
> the same nonce, allowing the pool leader to exfiltrate the
> private key.
>
> The safest protection against this is to always delete the
> `secnonce` as soon as you generate the partial signature,
> before sending the partial signature to the pool leader.
>
> In case of a true network failure where the partial signature
> was not received by the pool leader, an honest pool leader
> would resend a `swap_party_expand_state` with the same state
> as before, and the previously-generated partial signatures
> would still be correct for that state.

Expansion Phase State Signing
-----------------------------

The pool follower indicates that it validated the next state by
sending its partial signatures for each recreated update
transaction in a `swap_party_expand_sign` message:

1.  `swap_party_expand_sign` (406)
    - Sent from pool followers to pool leaders in response to
      `swap_party_expand_state`.
2.  TLVs:
    * TODO
