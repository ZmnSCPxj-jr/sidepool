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
Details of the Reseat Phase are on its own document
[SIDEPOOL-05][].

[SIDEPOOL-05]: ./05-reseat-splice.md

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
> arrange maintenance windows to avoid the time from 00:00 to
> 01:00 UTC daily.
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
> kind of suspension, as well as triggering swap parties "early"
> or at any time.


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
  * The timeout ends at `swap_party_expand_done` message sent from
    the pool leader to pool followers.
* For the Contraction Phase:
  * The timouet begins at `swap_party_expand_done` message sent
    from the pool leader to pool followers.
  * The timeout ends at `swap_party_contract_done` message sent
    from the pool leader to pool followers.

In case of a timeout, the sidepool is aborted.

Network is unreliable, thus, it is possible for pool leaders and
pool followers to see a disconnection of their Lightning Network
connections, but become able to reconnect before the above
timeout expires.
This disconnection may occur even if both pool follower and pool
leader have remained running and have not crashed.

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

| Pool Leader Broadcasts      | Pool Follower Responds         |
|-----------------------------|--------------------------------|
| `swap_party_begin`          | `swap_party_expand_request`    |
| `swap_party_expand_state`   | `swap_party_expand_sign`       |
| `swap_party_expand_done`    | `swap_party_contract_request`  |
| `swap_party_contract_state` | `swap_party_contract_sign`     |
| `swap_party_contract_done`  | `swap_party_contract_done_ack` |

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
       | swap_party_contract_done_ack  |
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
30 seconds), then the pool leader SHOULD defer the swap party for
up to 30 minutes (by delaying and retrying connecting to the
pool follower later), or until the next scheduled time.

After ensuring that it has a connection to all pool followers,
the pool leader MUST send a `ping` message with
`num_pong_bytes = 0` to all pool followers, as described in
[BOLT #1 The `ping` and `pong` messages][].

[BOLT #1 The `ping` and `pong` messages]: https://github.com/lightning/bolts/blob/master/01-messaging.md#the-ping-and-pong-messages

The pool leader MUST wait for a corresponding `pong` message from
all pool followers before continuing with this flow.

If a pool follower does not respond with a `pong` within 60
seconds, the pool leader:

* MAY retry the `ping`-`pong` with *all* pool followers.
  * MAY disconnect from the unresponsive pool follower(s), then
    reconnect before retrying the `ping`-`pong` with *all* pool
    followers.
* MAY instead defer this swap party, or abort the sidepool.
  * SHOULD defer the swap party or abort the sidepool if it has
    been `ping`ing pool followers for more than a few times.

> **Rationale** Once the pool leader has signalled the formal 
> start of the swap party via broadcasting `swap_party_begin`
> below, it has committed the entire participant set to actually
> completing the swap party, and an unresponsive participant
> *will* cause the entire sidepool to abort due to the timeout
> required by this specification.
> Thus, the pool leader must reduce the probability of an
> unresponsive participant by checking if all participants can at
> least respond to the `ping`-`pong` protocol.
>
> A TCP connection may be alive as far as the local operating
> system is concerned, but as TCP is built on top of IP, some IP
> node along the path between the pool leader and pool follower
> may have died in the mean time, in such a way that there are no
> other alternate paths.
> Thus, the pool leader first ensures that there is a viable IP
> path by doing an application-level `ping`-`pong` with all
> pool followers.
> While an IP node can still die between this `ping`-`pong` and
> the broadcast of `swap_party_begin`, the scope is smaller and
> thus this reduces the risk of a swap party timeout and
> subsequent sidepool abort.
>
> If the pool leader can `ping`-`pong` all pool followers, that
> implies that there exists IP paths from the pool leader to all
> pool followers that work in both directions.
> That also implies that each pool follower has a viable IP path
> from themselves to every other pool follower as well: at the
> minimum, it can take an IP path from itself to the IP node of
> the pool leader, then from there to the IP node of another
> pool follower.
> Thus, this coordination implies that the pool followers can
> now (re-)establish connections with each other and perform
> higher-level protocols triggered by the swap party.

As noted above, the pool leader SHOULD defer the swap party if it
is unable to connect and get `pong` responses in a reasonable
time frame.

However, the pool leader MAY instead abort the entire pool, if
swap parties have been persistently deferred due to persistent
inability to raise all the pool followers.
See [SIDEPOOL-03][] (TODO: section) for how sidepools are aborted.

> **Rationale** For example, the pool leader may defer a swap
> party for a few days, but if it is still unable to raise all the
> pool followers for a swap party after a few days, then the
> sidepool has become pointless as it cannot be used to manage
> liquidity, and it may be better to just abort the entire pool.

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

If the above validation fails, the receiver ignores the message,
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
        Absent otherwise.
    * `swap_party_expand_request_nonces` (40206)
      - Length: 396 (= 6 * (33 + 33))
      - Value: A length 6 array of entries:
        - 33 bytes: First MuSig2 nonce
        - 33 bytes: Second MuSig2 nonce
      - Required.
    * `swap_party_expand_request_still_contract` (40208)
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
signs a [BIP-340 Design][] tagged hash:

    tag = "sidepool version 1 expansion phase"
    content = Funding Transaction Output ||
              Next Pool Update Counter ||
              Current X-only Pubkey ||
              Current Amount ||
              value in swap_party_expand_request_newouts

[BIP-340 Design]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Design

`Funding Transaction Output` is the current funding transaction
output of the sidepool.
It is composed of 32 bytes of the funding transaction ID, and 2
bytes, a big-endian 16-bit number specifying the output index of
the funding transaction output, for a total of 34 bytes.

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

> **Rationale** In principle, only the output to be consumed and
> the new outputs to create need to be signed, but the tag, the
> funding transaction output, and the counter all provide a
> context for the signature, reducing the scope in which it could
> be accepted as valid.
>
> More specifically, the tag restricts the signature to this
> specific step in the protocol, the funding transaction output
> restricts the signature to a specific sidepool using this
> protocol, and the pool update counter restricts the signature
> to a specific swap party in the lifetime of a pool.
> In particular, a reseat of the sidepool, which would reuse
> pool update counter values, would require a change in the
> funding transaction output, so a signature from the same
> sidepool before a reseat cannot be reused after the reseat,
> even if they have the same pool update counter.

`Current X-only Pubkey` is the Taproot X-only public key for the
output specified in `swap_party_expand_request_prevout_signed`,
in its 32-byte serialization, and `Current Amount` is the 64-bit
amount in satoshis, in big-endian order (8 bytes).

The signature itself should validate against the `Current X-only
Pubkey`.

`swap_party_expand_request_nonces` are the MuSig2 nonces
generated by the pool follower.
Each entry is either a `pubnonce` from [BIP-327 Nonce
Generation][], or an all-0s entry.

[BIP-327 Nonce Generation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Nonce_Generation-2

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
  * It is of valid length.
  * The indicated output exists in the current output state, and
     has not been indicated by a different
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
* `swap_party_expand_request_nonces` exists and is of valid
  length, and:
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
and nothing is published onchain (i.e. a swap party cancel **is
not** a sidepool abort).

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
      - Value: A structure composed of:
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
* A swap party was initiated and is in the Expansion Phase, and
  the receiver recently sent a `swap_party_expand_request`.
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
sidepool, as per [SIDEPOOL-03][] (TODO: section).

[SIDEPOOL-03]: ./03-setup-and-teardown.md

> **Rationale** The only legitimate reason for an expansion
> request to be denied is if it would cause the sidepool to
> exceed the specified maximum number of outputs.
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

The pool follower now computes the onchain fee contribution tax,
as well as the onchain fee contribution overpayment.
It can now determine how the recreated update transactions look
like, based on the next output state, the onchain fee contribution
tax, the overpayment, and the overpayment beneficiary.

The pool follower then checks if it has already generated
partial signatures in response to some previous
`swap_party_expand_state` message.
If it already has genereated a partial signature, the pool
follower:

* MUST validate that the existing generated partial signatures
  validly sign the recreated update transactions for this set of
  outputs.
  * MAY implement this validation by instead caching the previous
    `swap_party_expand_state` message and comparing the TLVs of
    the current `swap_party_expand_state` with the previous one,
    and failing validation if they are different.

If the above validation fails, the pool follower MUST abort the
sidepool.
If the above validation passes, the pool follower resends the
partial signatures it already generated.

If the pool follower has not generated partial signatures yet,
it generates partial signatures for each recreated update
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
    * `swap_party_expand_sign_id` (40600)
      - Length: 16
      - Value: The sidepool identifier of the sidepool which the
        pool follower is providing signatures for.
      - Required.
    * `swap_party_expand_sign_signatures` (40602)
      - Length: 192 (= 6 * 32)
      - Value: A length 6 array of BIP-327 partial signatures.
      - Required.

`swap_party_expand_sign_signatures` are the partial signatures
that the pool follower generated after it has validated the output
state sent in a previous `swap_party_expand_state`.

Each entry in the `swap_party_expand_sign_signatures` array
corresponds to a recreated update transaction.
If a particular update transaction does not need to be recreated,
the corresponding entry is set to all 0s.
As noted in previous sections, the last two entries, corresponding
to the Fifth and Last update transactions, must always be set.

Receivers of this message MUST validate the following:

* It recognizes that it is the pool leader of the given sidepool
  with the given pool identifier.
* The message sender is a pool follower of the given sidepool
  with the given pool identifier.
* `swap_party_expand_sign_signatures` exists and is of valid
  length.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

The pool leader then validates the following:

* It recently sent a `swap_party_expand_state` message.
* It has not received a `swap_party_expand_sign` message from this
  pool follower yet.
* The entries in `swap_party_expand_sign_signatures` that
  correspond to recreated update transactions are non-0, and pass
  [BIP-327 Partial Signature Validation][] for the corresponding
  recreated update transaction.

[BIP-327 Partial Signature Validation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Partial_Signature_Verification

If any of the above validation fails, the pool leader MUST abort
the sidepool.

Once the pool leader has received `swap_party_expand_sign` from
all pool followers, the pool leader generates its own partial
signature for the recreated update transactions.
The pool leader then performs [BIP-327 Partial Signature
Aggregation][] on all the partial signatures for each recreated
update transaction.

[BIP-327 Partial Signature Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Partial_Signature_Aggregation

The pool leader MUST, atomically, update the below in persistent
storage:

* Store the signatures for the recreated update transactions.
* Increment the pool update counter.
* Set the current output state to the Expansion Phase next output
  state, that was sent in the previous `swap_party_expand_state`.

The pool leader then creates a copy of the current output state
and labels it the next output state.

Expansion Phase Completion
--------------------------

The pool leader then broadcasts the aggregated signatures for the
new, post-Expansion state, using the `swap_party_expand_done`
message.

1.  `swap_party_expand_done` (408)
    - Sent from pool leader to pool followers to provide completed
      signatures for the post-Expansion state.
2.  TLVs:
    * `swap_party_expand_done_id` (40800)
      - Length: 16
      - Value: The sidepool identifier of the sidepool whose
        Expansion Phase has completed.
      - Required.
    * `swap_party_expand_done_signatures` (40802)
      - Length: 384 (= 6 * 64)
      - Value: A length 6 array of signatures, in the format defined
        by [BIP-340 Default Signing][].
      - Required.

[BIP-340 Default Signing]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Default_Signing

`swap_party_expand_done_signatures` are the aggregated signatures
for each update transaction, for the post-Expansion state.
Each entry corresponds to an update transaction.
Not all update transactions are recreated in the Expansion Phass;
only recreated update transactions have a valid entry in this
array, and entries corresponding to update transactions that are
not recreated are all 0s.

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the sidepool with the
  given pool identifier.
* `swap_party_expand_done_signatures` exists and is of valid
  length.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

The pool follower then MUST validate the following:

* The signatures for recreated update transactions are valid for
  the post-Expansion state sent in the `swap_party_expand_state`,
  as per [BIP-340 Verification][].

[BIP-340 Verification]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Verification

If the above validation fails, the pool follower MUST abort the
sidepool as per [SIDEPOOL-03][].

After validation, the pool follower MUST, atomically, update the
below in persistent storage:

* Store the signatures for the recreated update transactions.
* Increment the pool update counter.
* Set the current output state to the Expansion Phase next output
  state, that was sent in the previous `swap_party_expand_state`.

Signatures for update transactions that were not recreated are not
changed and are retained.

Once the pool follower has persisted the above updates, it should
update any higher-level protocols relying on the creation of new
outputs, as those outputs have now been instantiated into the
latest state of the sidepool.
Once the pool follower has continued the higher-level protocols, it
then proceeds to the next step.

Contraction Phase Requests
--------------------------

In response to `swap_party_expand_done`, pool followers send a
`swap_party_contract_request` message.

In effect, the end of the Expansion Phase signals the start of the
Contraction Phase.

Like the `swap_party_expand_request` message, the
`swap_party_contract_request` message can be a "no-op" request
where the pool follower does not request for any changes to any
output it controls, or outputs it has an interest in.

Otherwise, the `swap_party_contract_request` indicates one or
more outputs to be consumed, and a single new output to be
created.

Regardless, the message always provides nonces for the next
state.

1.  `swap_party_contract_request` (412)
    - Sent from pool follower to pool leader, in response to
      `swap_party_expand_done` message.
2.  TLVs:
    * `swap_party_contract_request_id` (41200)
      - Length: 16
      - Value: The sidepool identifier of the pool.
      - Required.
    * `swap_party_contract_request_prevouts_signed` (41202)
      - Length: 104 * N, 1 <= N <= `sidepool_version_1_max_outputs`.
      - Value: An array of the following structure:
        - 32 bytes: Taproot X-only Pubkey.
        - 8 bytes: Amount, in satoshis, big-endian 64-bit.
        - 64 bytes: Taproot signature.
      - Optional.
    * `swap_party_contract_request_newout` (40204)
      - Length: 40
      - Value: A structure composed of:
        - 32 bytes: Taproot X-only Pubkey.
        - 8 bytes: Amount, in satoshis, big-endian 64-bit.
      - Required if `swap_party_contract_request_prevouts_signed`
        is specified.
        Absent otherwise.
    * `swap_party_contract_request_nonces` (40206)
      - Length: 66
      - Value: A structure composed of:
        - 33 bytes: First MuSig2 nonce.
        - 33 bytes: Second MuSig2 nonce.

`swap_party_contract_request_prevouts_signed` specifies one or
more outputs present in the post-Expansion output state.
Each entry in the array specifies the Taproot X-only Pubkey and
the amount of the output that will be consumed.
Each entry also includes a [BIP-340][] signature that is signed
with the same Taproot X-only Pubkey of the output to be consumed.

The signatures in `swap_party_contract_request_prevouts_signed`
sign a [BIP-340 Design][] tagged hash:

    tag = "sidepool version 1 contraction phase"
    content = Funding Transaction Output ||
              Next Pool Update Counter ||
              Current X-only Pubkey ||
              Current Amount ||
              Next X-only Pubkey

`Funding Transaction Output` is the current funding transaction
output of the sidepool.
It is composed of 32 bytes of the funding transaction ID, and 2
bytes, a big-endian 16-bit number specifying the output index of
the funding transaction output, for a total of 34 bytes.

`Next Pool Update Counter` is the pool update counter,
zero-extended from 11 bits to 16 bits, in big-endian order (two
bytes).
The exact value is the one the pool update counter will have at
successful completion of the Contraction Phase, and MUST be odd.

`Current X-only Pubkey` and `Current Amount` are the output
indicated in the same entry, i.e. the first 40 bytes of the
entry in the `swap_party_contract_request_prevouts_signed`

> **Rationale** In principle, instead of one signature for each
> output that is consumed, a single signature can be used instead,
> aggregated from each individual output consumed.
> However, in the case where higher-level protocols have decided
> that some partcipant now owns an output, but the output was
> created with shared control, consuming that output would require
> contributions from multiple parties already, and if any of the
> outputs consumed are also aggregated, would require an
> aggregate-within-aggregate, with attendant complexity in
> implementing the aggregation of aggregates signing algorithm, as
> well as potentially increasing complexity of higher-level
> protocols.

`Next X-only Pubkey` is the Taproot X-only Pubkey in the
`swap_party_contract_request_newout` TLV, i.e. it is the public
key of the output that will be created to hold all the funds being
spent in the Contraction Phase by this participant.

> **Rationale** While in the Expansion Phase the signature covers
> the amounts to associate with the new outputs to be created, the
> signature in the Contraction Phase does not.
> This allows higher-level protocols, or software modules that
> handle signing, to generate signatures for a single destination
> output wwithout knowing the sum total of amount going to that
> output.
> This may be useful if other signing attempts may fail, or to
> limit parts of the implementation from signing keys.
> Contraction Phase requests spend from funds that were created by
> other participants, and those funds are likely to be in shared
> control; this allows a little more flexibility in higher layers.

`swap_party_contract_request_newout` indicates the new output that
will be created on consumption of the indicated previous outputs.

`swap_party_contract_request_nonces` provides the MuSig2 nonces
for the new signature of the recreated Last update transaction.
It is equivalent to a `pubnonce` structure from [BIP-327 Nonce
Generation][].

Receivers of this message MUST validate the following:

* It recognizes that it is the pool leader of the given sidepool
  with the given pool identifier.
* The message sender is a pool follower of the given sidepool
  with the given pool identifier.
* It has recently sent `swap_party_expand_done` and the sender has
  not sent this response yet.
* If `swap_party_contract_request_prevouts_signed` exists:
  * It is of valid length.
  * The indicated outputs currently exist in the current output
    state, and has not been indicated by a different
    `swap_party_contract_request` message by a different pool
    follower.
    * Note that the output state is a bag, not a set.
  * `swap_party_contract_request_newout` must exist.
    * The sum total of all amounts in
      `swap_party_contract_request_prevouts_signed` is exactly
      equal to the amount specified in
      `swap_party_contract_request_newout`.
    * Each signature in
      `swap_party_contract_request_prevouts_signed` signs the
      correct message based on the value of
      `swap_party_contract_request_newout`.
* `swap_party_contract_request_nonces` exists and is of valid
  length, and is not all 0s.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

On receiving and validating this message, and if the message requests
to consume outputs and create a new output, the pool leader removes
the outputs to be consumed and adds the new output in the next output
state.
Again, the output state is a bag, not a set, thus if the request
specifies an output once, but the next output state lists the output
multiple times, only one will be deleted, and so on.

Once the pool leader has received a `swap_party_contract_request`
from each pool follower, it proceeds to the next step.

Contraction Phase State Validation
----------------------------------

The pool leader also aggregates the MuSig2 nonces of all
participants, including fresh MuSig2 nonces for itself, as per
[BIP-327 Nonce Aggregation][].

The pool leader then broadcasts the `swap_party_contract_state`
message, so that the pool followers can validate the new state.

1. `swap_party_contract_state` (414)
   - Sent from pool leader to pool followers, once the pool
     leader has received `swap_party_contract_request` from all
     pool followers.
2.  TLVs:
    * `swap_party_contract_state_id` (41400)
      - Length: 16
      - Value: The sidepool identifier of the sidepool whose
        next state post-Contraction is being broadcasted.
      - Required.
    * `swap_party_contract_state_nonces` (41402)
      - Length: 66
      - Value: A structure composed of:
        - 33 bytes: First MuSig2 nonce
        - 33 bytes: Second MuSig2 nonce
      - Required.
    * `swap_party_contract_state_beneficiary` (41404)
      - Length: 40
        - 32 bytes: Taproot X-only Pubkey.
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
          number.
      - Required.
    * `swap_party_contract_state_next` (41406)
      - Length: N * 40, 1 <= N <= `sidepool_version_1_max_outputs`.
        See [SIDEPOOL-02][] for the maximum N.
      - Value: An array of 40-byte entries:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit
      - Required.

`swap_party_contract_state_nonces` are the aggregated MuSig2
nonces for the recreated Last update transaaction, i.e. the
`aggnonce` from [BIP-327 Nonce Aggregation][].

`swap_party_contract_state_beneficiary` nominats an output of the
next state that will benefiy from the onchain fee contribution
overpayment, if the overpayment is non-zero.

`swap_party_contract_state_next` contains the next output state
generated by the pool leader from the `swap_party_contract_request`
messages from the pool followers.
The array is in the canonical order described in [SIDEPOOL-02][]
(TODO section).

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the given sidepool
  with the given pool identifier.
* A swap party was initiated and is in the Contraction Phase, and
  the receiver recently sent a `swap_party_contract_request`.
* TLV fields `swap_party_contract_state_nonces`,
  `swap_party_contract_state_beneficiary`, and
  `swap_party_contract_state_next` all exist and have valid
  lengths.
* Outputs in `swap_party_contract_state_next` are in canonical
  order defined in [SIDEPOOL-02][] (TODO: section).
* The sum total of all amounts in
  `swap_party_contract_state_next` is equal to the satoshi value
  of the funding transaction output backing the entire pool.
* Each amount in `swap_party_contract_state_next` is greater than
  or equal to `sidepool_version_1_min_amount` defined in
  [SIDEPOOL-02][] (TODO: section).
* The output nominated as overpayment beneficiary in
  `swap_party_contract_state_beneficiary` exists in
  `swap_party_contract_state_next`.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

On receiving and validating this message, the pool follower
computes the onchain fee contribution tax, as well as the onchain
fee contribution overpayment.

The pool follower then checks if any outputs it expects exist in
the given next state:

* If the pool follower did not request to remove old outputs in
  the `swap_party_contract_request` message:
  * If the pool follower is relying on the existence of
    particular outputs, the outputs must still exist in the next
    state.
* If the pool follower requested to remove old outputs in the
  `swap_party_contract_request` message:
  * The new output it wanted to create exists in the next state.
  * If the pool follower is relying on the existence of
    particular outputs, the outputs must still exist in the next
    state.

If any of the above checks fail, the pool follower MUST abort the
sidepool, as per [SIDEPOOL-03][].

The pool follower now computes the onchain fee contribution tax,
as well as the onchain fee contribution overpayment.
It can now determine how the recreated update transactions look
like, based on the next output state, the onchain fee contribution
tax, the overpayment, and the overpayment beneficiary.

The pool follower then checks if it has already generated
partial signatures in response to some previous
`swap_party_contract_state` message.
If it already has genereated a partial signature, the pool
follower:

* MUST validate that the existing generated partial signatures
  validly sign the recreated update transactions for this set of
  outputs.
  * MAY implement this validation by instead caching the previous
    `swap_party_contract_state` message and comparing the TLVs of
    the current `swap_party_contract_state` with the previous one,
    and failing validation if they are different.

If the above validation fails, the pool follower MUST abort the
sidepool.
If the above validation passes, the pool follower resends the
partial signatures it already generated.

It generates partial signatures for each recreated update
transaction, using [BIP-327 Signing][].
The pool follower MUST delete its `secnonce` or equivalent
structure (by zeroing out its memory and then freeing it or
dropping all references to it), which it can use to determine if
it already created signatures, and must also retain its partial
signatures in-memory.

Contraction Phase State Signing
-------------------------------

The pool follower indicates that it validated the next state by
sending its partial signature for the recreated Last update
transaction in a `swap_party_contract_sign` message:

1.  `swap_party_contract_sign` (416)
    - Sent from pool followers to pool leaders in response to
      `swap_party_expand_state`.
2.  TLVs:
    * `swap_party_contract_sign_id` (40600)
      - Length: 16
      - Value: The sidepool identifier of the sidepool which the
        pool follower is providing a signature for.
      - Required.
    * `swap_party_contract_sign_signature` (40602)
      - Length: 192 (= 6 * 32)
      - Value: A BIP-327 partial signature.
      - Required.

`swap_party_contract_sign_signature` is the partial signature
that the pool follower generated after it has validated the output
state sent in a previous `swap_party_contract_state`.

Receivers of this message MUST validate the following:

* It recognizes that it is the pool leader of the given sidepool
  with the given pool identifier.
* The message sender is a pool follower of the given sidepool
  with the given pool identifier.
* `swap_party_contract_sign_signatures` exists and is of valid
  length.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

The pool leader then validates the following:

* It recently sent a `swap_party_contract_state` message.
* It has not received a `swap_party_contract_sign` message from
  this pool follower yet.
* `swap_party_contract_sign_signatures` passes [BIP-327 Partial
  Signature Validation][] for the recreated Last update
  transaction.

If any of the above validation fails, the pool leader MUST abort
the sidepool.

Once the pool leader has received `swap_party_expand_sign` from
all pool followers, the pool leader generates its own partial
signature for the recreated Last update transaction.
The pool leader then performs [BIP-327 Partial Signature
Aggregation][] on all the partial signatures.

The pool leader MUST, atomically, update the below in persistent
storage:

* Store the signature for the recreated Last update transactions.
* Increment the pool update counter.
* Set the current output state to the Contraction Phase next
  output state, that was sent in the previous
  `swap_party_contract_state`.

Contraction Phase Completion
----------------------------

The pool leader then broadcasts the aggregated signature for the
new, post-Contraction state using the `swap_party_contract_done`
message.

1.  `swap_party_contract_done` (418)
    - Sent from pool leader to pool followers to provide completed
      signatures for the post-Expansion state.
2.  TLVs:
    * `swap_party_contract_done_id` (41800)
      - Length: 16
      - Value: The sidepool identifier of the sidepool whose
        Contraction Phase has completed.
      - Required.
    * `swap_party_contract_done_signature` (41802)
      - Length: 64
      - Value: A signature for the Last update transaction, in the
        format defined by [BIP-340 Default Signing][].
      - Required.
    * `swap_party_contract_done_will_reseat` (41804)
      - Length: 0
      - Optional.

`swap_party_contract_done_signature` is the aggregated signature
for the Last update transaction, for the post-Contraction state.

`swap_party_contract_done_will_reseat`, if present, indicates that
there will be a Reseat Phase after the Contraction Phase.
See [SIDEPOOL-05][] for more details on Reseat Phase.

Receivers of this message MUST validate the following:

* It recognizes that it is a pool follower (and not the pool
  leader) of the given sidepool with the given pool identifier.
* The message sender is the pool leader of the sidepool with the
  given pool identifier.
* `swap_party_contract_done_signature` exists and is of valid
  length.
* If `swap_party_contract_done_will_reseat` exists, it has length
  0.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

The pool follower then MUST validate the following:

* The signature for the recreated Last update tranasction is
  valid for the post-Contraction state sent in the
  `swap_party_contract_state`, as per [BIP-340 Verification][].
* OR, if the current pool update counter is odd, the signature
  is bit-per-bit exactly the same as the persisted signature for
  the Last update transaction.

If the above validation fails, the pool follower MUST abort the
sidepool as per [SIDEPOOL-03][].

If the current pool update counter is odd and the above validation
succeeded, then the pool follower MUST simply send the
`swap_party_contract_done_ack` message, and MUST NOT update any
persisted data.

> **Rationale** In the very edge case where:
>
> 1.  The pool leader sends `swap_party_contract_done`.
> 2.  The pool follower is able to persist the signature and
>     update the pool update counter.
> 3.  The pool follower crashes before it can send
>     `swap_party_contract_done_ack`.
> 4.  The pool follower comes back online and reconnects.
>
> Then the pool leader will reasonably re-send
> `swap_party_contract_done`, on the presumption that this is
> "just" the network connection being unreliable.
> However, swap party information is recommended in this
> specification to be in-memory, thus at restart, the pool follower
> thinks that there is no swap party currently on-going.
> In that case, the pool follower can see that the message is
> simply a resend of a message it has already received and whose
> result it has persisted, and it can safely pretend that it knows
> of the swap party.

After validation, if the current pool update counter is even, the
pool follower MUST, atomically, update the below in persistent
storage:

* Store the signature for the recreated Last update transaction.
* Increment the pool update counter.
* Set the current output state to the Contraction Phase next output
  state, that was sent in the previous `swap_party_contract_state`.

Once the pool follower has persisted the above updates, it should
update any higher-level protocols that the Contraction Phase has
completed.

If `swap_party_contract_done_will_reseat` exists, then the pool
follower knows that there will be an upcoming Reseat Phase, and the
pool leader will send, after all pool followers have replied with
`swap_party_contract_done_ack`, a separate message `reseat_begin`,
as described in [SIDEPOOL-05][].

### Acknowledging Contraction Phase Completion

The pool follower will then send a `swap_party_contract_done_ack`
message, to inform the pool leader that it has received and
persisted the aggregated signature for the post-Contraction state.

> **Rationale** This message is necessary to handle the edge case
> where the pool follower disconnects after the pool leader has
> sent the aggregated signature for the state.
> In that case, the pool leader cannot be sure that the pool
> follower has received the aggregated signature.
> This message is a definite information that the pool follower
> has indeed received the signature for the last state.

1.  `swap_party_contract_done_ack` (420)
    - Sent by pool followers to the pool leader to acknowledge that
      the Contraction Phase has ended.
2.  TLVs:
    * `swap_party_contract_done_ack_id` (42000)
      - Length: 16
      - Value: The sidepool identifier of the pool.
      - Required.

Receivers of this message MUST validate the following:

* It recognizes that it is the pool leader of the given sidepool
  with the given pool identifier.
* The message sender is a pool follower of the given sidepool with
  the given pool identifier.

If any of the above validation fails, the receiver SHOULD ignore
the message and SHOULD log it as a warning or alarm it to the
human operator.

If the pool leader did not receive a previous
`swap_party_contract_done_ack` from the sending pool follower yet,
then the pool leader simply marks that the specific pool follower
has sent it and there is no need to re-send the
`swap_party_contract_done` message when that pool follower
reconnects.

Once all pool followers have responded with this message, the
pool leader proceeds:

* If the pool leader has signalled an upcoming Reseat Phase via
  including `swap_party_contract_done_will_reseat` to the previous
  `swap_party_contract_done` message, then the pool leader
  initiates the Reseat Phase as described in [SIDEPOOL-05][].
* Otherwise, the pool leader can now forget about the swap party.

Swap Parties And Higher-Level Protocols
=======================================

> **Non-normative** This section describes what the specification
> author believes to be best practice, but is not a strict
> requirement for conformance to the specification.

Roughly speaking, the swap party is the primary point of
interaction between sidepools and higher-level protocols.

A sidepool software might want to provide some "hooks" for
software implementing higher-level protocols built on top of it.
Roughly, such a software might have a sequence diagram
approximately like the below:

                                   Generic                      High-level
     Leader                        Follower                      Protocol
        |                             |                              |
        |      swap_party_begin       |                              |
        |---------------------------->|  (swap party started notif)  |
        |                             |----------------------------->|
        |                             |                              |
        |                             |                 +--------------------------+
        |                             |                 |   Decide to run or not   |
        |                             |                 |   Negotiate with peers   |
        |                             |                 |  Figure out new outputs  |
        |                             |                 +--------------------------+
        |                             |                              |
        |                             |     (request new outputs)    |
        |                             |<-----------------------------|
        |  swap_party_expand_request  |                              |
        |<----------------------------|                              |
        |                             |                              |
        |                             |                              |
        |   swap_party_expand_state   |                              |
        |---------------------------->|                              |
        |                             |                              |
        |                +-------------------------+                 |
        |                |   Check if new outputs  |                 |
        |                |      were created       |                 |
        |                +-------------------------+                 |
        |                             |                              |
        |   swap_party_expand_sign    |                              |
        |<----------------------------|                              |
        |                             |                              |
        |                             |                              |
        |    swap_party_expand_done   |                              |
        |---------------------------->|                              |
        |                             |                              |
        |                          Generic                      High-level
     Leader                        Follower                      Protocol
        |                             |                              |
        |                             | (outputs were created notif) |
        |                             |----------------------------->|
        |                             |                              |
        |                             |                 +--------------------------+
        |                             |                 |Acquire/Release control of|
        |                             |                 |   outputs from/to peers  |
        |                             |                 +--------------------------+
        |                             |                              |
        |                             |   (request to claim output)  |
        |                             |<-----------------------------|
        | swap_party_contract_request |                              |
        |<----------------------------|                              |
        |                             |                              |
        |                             |                              |
        |  swap_party_contract_state  |                              |
        |---------------------------->|                              |
        |                             |                              |
        |                             |                              |
        |  swap_party_contract_sign   |                              |
        |<----------------------------|                              |
        |                             |                              |
        |                             |                              |
        |   swap_party_contract_done  |                              |
        |---------------------------->|                              |
        |                             |                              |
        |swap_party_contract_done_ack |                              |
        |<----------------------------|    (outputs claimed notif)   |
        |                             |----------------------------->|
        |                             |                              |

