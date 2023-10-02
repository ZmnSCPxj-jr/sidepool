SIDEPOOL-10 Peerswap In Sidepool

Overview
========

Sidepool version 1 pools are intended to manage liquidity on
Lightning Network channels by allowing participants to use
in-pool funds to move liquidity to channels with insufficient
liquidity,

This is done by doing a swap between two direct peers on the
Lightning Network, a.k.a. a peerswap.

With sidepools, peerswaps are triggered every swap party.
Swap parties are described in [SIDEPOOL-04][].

[SIDEPOOL-04]: ./04-swap-party.md

SIDEPOOL-protocol Feature Bit
-----------------------------

Peerswap-in-sidepool has the SIDEPOOL feature bit 1
(`peerswap_in_sidepool_feature_bit`).

Pool participants that support peerswap-in-sidepool MUST NOT
send peerswap-in-sidepool messages unless the counterparty has
indicated support of the above feature bit.

Process
-------

The initiator of a peerswap-in-sidepool always offers a fund in
the sidepool in exchange for additional funds in channels, i.e.
sometimes called a "loop in" or "swap in".
Participants cannot initiate moving funds from their channel to
the sidepool.
Initiating participants first offer an HTLC in the Expansion Phase
of the swap party, then once the Expansion Phase ends, the
accepting participant offers a corresponding HTLC inside one or
more Lightning Network channels.

Once the in-Lightning HTLC is irrevocably committed and fulfilled,
the initiator of the peerswap-in-sidepool then releases its
ephemeral key for the sidepool HTLC, as described in
[SIDEPOOL-06 Private Key Handover][].

[SIDEPOOL-06 Private Key Handover]: ./06-htlcs.md#private-key-handover

On receiving the initiator ephemeral key, the acceptor then claims
the sidepool HTLC in the Contraction Phase of the swap party.
Once the Contraction Phase completes, the peerswap-in-sidepool now
completes.

Balance Measurement
-------------------

The ideal is to ensure that channels are "perfectly balanced",
with the funds split equally between both participants in the
channel.

In a channel without pending HTLCs, we can easily get the sense of
"perfectly balanced" as the state where exactly half the channel
is owned by one party, and the rest (exactly half as well) is owned
by the other party.
However, HTLCs are netheir here nor there, and channels may have
pending HTLCs while a peerswap-in-sidepool is being negotiated.

As a simplification, we assign HTLCs, regardless of size, age, or
who offered it, as a fund that is shared equally between both
participants in the channel, i.e. we assign half the value of the
HTLC to each party.

Then, we compute the "*imbalance error*  for actual Lightning
Network channels as below:

* Set "total channel capacity" and "twice our-side funds"
  variables to 0.
* Over all channels in "normal operation" with the peer:
  * "Normal operation" is after both participants have emitted
    [BOLT #2 `channel_ready`][] messages, and before either
    participant has emitted [BOLT #1 `error`][] or [BOLT #2
    `shutdown`][] messages or has detected a unilateral close of
    the channel.
  * Convert the channel capacity (the amount of the backing
    funding transaction output; do not deduct channel reserves)
    into millisatoshis.
  * Add the channel capacity (in millisatoshis) to the "total
    channel capacity" variable.
  * Add the channel capacity (in millisatoshis), plus our funds
    in that channel, minus their funds in that channel, to the
    "twice our-side funds" variable.
    * Do not deduct channel reserves.
    * Use the funding amount that would be considered
      unambiguously owned by our side and unambiguously their
      side.
* Take the absolute value of the difference between "total channel
  capacity" and "twice our-side funds", and use it as the
  "*imbalance error*"

[BOLT #2 `channel_ready`]: https://github.com/lightning/bolts/blob/master/02-peer-protocol.md#the-channel_ready-message
[BOLT #1 `error`]: https://github.com/lightning/bolts/blob/master/01-messaging.md#the-error-and-warning-messages
[BOLT #2 `shutdown`]: https://github.com/lightning/bolts/blob/master/02-peer-protocol.md#closing-initiation-shutdown

As a concrete example:

* Suppose participant A and participant B have a single 1000
  millisatoshi channel, with state: Participant A = 1
  millisatoshi, HTLCs = 4 millisatoshis, and Participant B = 995
  millisatoshis.
  * "total channel capacity" is 1000.
  * Participant A "twice our-side funds" is (1 + 1000 - 995) = 6.
    * This is equivalent to "our-side funds" being 3 millisatoshi
      (half of 6, the "*twice* our-side funds"):
      1 millisatoshi owned unambiguously by A and half of the
      HTLCs being 4 millisatoshis, or 2, summing to 3.
  * Participant B "twice our-side funds" is (995 + 1000 - 1)
    = 1994, equivalent to saying it has 997 millisatoshis (995
    unambiguously owned by B, 2 from its share of the HTLCs).
  * "imbalance error" is 994; this is the same for both
    participants (= absolute(1000 - 6) = absolute(1000 - 1994)).

> **Rationale** Adding the funds we own, plus the channel
> capacity, minus the funds they own, *and then* dividing that by
> 2 results in a value that assigns half the amount of pending
> HTLCs (or more pedantically, assigns anything that is neither
> unambiguously ours or theirs, which in the future might include
> PTLCs and DLCs) to us and half to them.
>
> We then want to compare that value to the "perfectly balanced
> point" that is half the total channel capacity, i.e. the total
> of all channel capacities, divided by 2.
>
> As both are divided by 2, we can cancel out the division on both
> sides of the comparison.
>
> The score described above is thus twice the actual imbalance
> error in millisatoshis.
> However, by avoiding division by two and instead effectively
> multiplying both sides by 2, we avoid issues when using integer
> division and round-up vs round-down.
>
> Another way of viewing it is that the resulting imbalance error
> is actually a fixed-point format where the binary point is after
> the lowest bit, i.e. the lowest bit is the fractional part,
> representing half-millisatoshi amounts.

> **Rationale** The computation is described in detail above to
> ensure that all implementations use the same calculation and
> avoid disagreements.
> Yes, it is a given that the above is ***only*** an estimation,
> and that information about pending HTLCs, their size, their
> hash, their age, who offerred them, and at what time, can be
> used to better determine whether it is likely that the HTLC will
> be fulfilled or not, and from there assign a probability
> estimate times HTLC amount to the correct participant.
> However, such heuristics may change as the network and its users
> change, and having multiple implementations with diverging ideas
> of how to determine "the current balanced-ness" of the channels
> between nodes is likely to simply cause failures, whereas a
> simple rule like the above is easy to implement and reduces
> divergences between different implementations; it is more
> important in this case to be consistent than to be absolutely
> accurate but disagreeing with simpler implementations.
> The obsessively perfect is the enemy of the good enough.

The rules for offering peerswap-in-sidepool are:

* If a participant has "twice our-side funds" as equal or
  greater than "total channel capacity", then the counterparty
  is the one that is allowed to offer peerswap-in-sidepool and
  this participant MUST NOT offer a peerswap-in-sidepool.
* Conversely, if a participant has "twice our-side funds" as
  less than "total channel capacity", then it MAY offer a
  peerswap-in-sidepool:
  * If the imbalance error is less than the minimum amount
    mandated in [SIDEPOOL-02 Output States][], then the
    participant MUST NOT offer a peerswap-in-sidepool.
  * Otherwise, SHOULD offer an amount equal to half of the
    imbalance error, or the minimum amount specified, whichever is
    higher, rounded either way to an exact satoshi value.
    It MAY offer anywhere from the [SIDEPOOL-02 Output States][]
    minimum amount, to one less than the imbalance error,
    inclusive, provided the offerred amount is an exact satoshi
    value.

[SIDEPOOL-02 Output States]: ./02-transactions.md#output-states

When a participant receives a peerswap-in-sidepool offer:

* If the receiver "twice our-side funds" is less than the
  "total channel capacity", then reject the peerswap.
* Otherwise, if the offered peerswap amount is greater than or
  equal to the "imbalance error", or is less than the minimum
  amount specified in [SIDEPOOL-02 Output States][], then reject
  the peerswap.
* Otherwise:
  * If the funds unambiguously on our side are less than the
    offerred amount, reject the peerswap.
  * Otherwise, accept the peerswap.

> **Rationale** The above follows "be strict in what you send,
> lenient in what you accept": the offerrer should offer a
> peerswap that causes the imbalance error to drop to 0 or close
> to 0 (= half the imbalance error), but the aceptor must accept
> anything that causes the imbalance error to become lower (if
> the imbalance is such that the offerrer has less funds, any
> amount lower than the imbalance error improves the balance).
>
> As a concrete example, suppose that participants A and B have a
> 1000 millisatoshi channel; A has 1 millisatoshis and B has 999
> millisatoshis.
> Participant A would see a "twice our-side funds" of 2
> (= 1 + 1000 - 999) and an imbalance error of 998.
>
> Participant A could offer 997 millisatoshi in a swap, which
> would move the state to A has 998 millisatoshis and B has
> 2 millisatoshis, which results in a "twice our-side funds"
> of 1996 (= 998 + 1000 - 2) and an imbalance error of 996.
> Participant B should accept such an offer, even though it does
> not reduce the imbalance error to 0; as long as the imbalance
> error goes lower, it is still a better situation.
>
> If participant A had instead offerred half of the imbalance
> error (= 998 / 2 = 499) then the new state would have been the
> perfect A has 500 millisatoshis and B has 500 millisatoshis.
>
> Such leeway is allowed since forwardable peerswaps, described
> below, would result in offerring peerswaps that do not move
> the channel state to perfectly balanced, but may cause the
> post-peerswap state to undershoot or overshoot the
> perfectly-balanced state.
> Forwardable peerswaps mean that even if two participants in a
> sidepool do not have a direct channel, if they do have a path
> between them where all nodes are participants in the same
> sidepool, they can still benefit each other by being terminal
> points of forwarded peerswaps.
> In addition, forwardable peerswaps allow participants with
> no or little funds in the sidepool to get their channels
> balanced during a swap party, instead of accepting the peerswap
> on one channel to get sidepool funds, then waiting for the next
> swap party to use the funds to balance its other channel.
>
> Another reason is that an imbalanced long-lived channel that
> has seen a lot of activity implies that the imbalance is due to
> one side being popular, e.g. continuing the example above, it
> implies that participant B is a popular destination.
> In that case, the peerswap offerrer might want to overbalance
> in favor of the other side, as it expects that until the next
> swap party, payment flow towards the other side will remain
> high, and it is better to overshoot a little so that actual
> payment flows will bring it closer to perfectly balanced.
>
> Finally, there is an inherent race condition here in that
> participants may sample their channel states at slightly
> different times.
> By being lenient in what you accept, small differences in
> sampled states do not cause rejections that would still have
> benefited the channel balance.

> **Rationale** Targeting a "perfectly balanced" 50-50 split stems
> from the observation that as a Lightning Network channel is
> used, its current balance is the integral of all payment flows
> on that channel.
>
> Integrals, as anyone who has studied calculus knows, include a
> `+ C` constant term.
>
> By always targeting the 50-50 split, the `+ C` constant for the
> integral of payment flows becomes the 50-50 halfway point.
> This maximizes the amount of space in which the balance (= the
> integral of payment flows) can move in, which provides more
> information to your node as to the general trend of payments
> via that channel, which can then be used to drive other business
> decisions.

Forwaradable Peerswaps
======================

Peerswaps, provided they are "swap in" or from outside the
Lightning Network to inside a Lightning Network channel, may be
forwarded safely.
This simply means that an "acceptor" can turn around and become an
offerrer of the peerswap to another participant of the same
sidepool, provided that it also has an imbalance away from itself
with the participant being forwarded to.

Forwardable peerswaps have the following benefits:

* Participants in the same sidepool might not have direct channels
  with each other, but if there is a path between them with other
  participants in the same sidepool, they can still effectively
  peerswap with each other.
* Sidepool participants with little or no funds in the sidepool
  can still offer peerswaps by forwarding a peerswap offered by
  another participant, allowing more channels to be put nearer to
  the balanced state in a single swap party.
* Forwardable peerswaps naturally reset the state of the network
  from net senders to net receivers, without requiring that any
  participant have a coherent global view of the entire network.

Forwardable peerswaps simply require that peerswap-related messages
be forwarded back and forth, with necessary changes to those
messages done by the forwarder.
In effect, the forwarder acts as a proxy between the direct
offerrer and the direct acceptor (either or both of which may also
be forwarders).

Sidepool participants SHOULD implement forwardable peerswaps in
sidepools.

Forwardable Peerswap Algorithm
------------------------------

> **Non-normative** This section is a recommendation on how to
> implement forwardable peerswaps within a sidepool, such that
> more peerswaps occur in a single swap party, but does not have
> to be followed strictly for compliance with this specification.

When a swap party is initiated by the pool leader:

* The participant samples the channel states and computes the
  "total channel capacity", "twice our-side funds", and
  "imbalance error" for each other participant in the same
  sidepool.
* The participant classifies counterparties as follows:
  * To-offer: "twice our-side funds" is less than "total
    channel capacity".
  * To-accept: "twice our-side funds" is greater than "total
    channel capacity".
  * Drop the counterparty if both variables are equal.
* Sort the to-offer counterparties according to their "imbalance
  error", with highest imbalance error first.
* Go through the sorted list, allocating funds available in the
  sidepool to each other participant.
  * Allocate funds equal to half (or some reasonable fraction)
    of the "imbalance error" with that participant, rounded to an
    exact satoshi amount.
  * If we run out of funds in the sidepool, move the remaining
    to-offer list to a pending-to-offer list.
* Go through the sorted to-offer list and offer peerswaps to them,
  of the amount allocated.
* Wait for incoming peerswap offers from to-accept participants.
  Reject peerswap offers if they are not coming from a participant
  in the to-accept list.
  * If the incoming peerswap amount equals or exceeds the
    imbalance error, reject the peerswap.
  * If the pending-to-offer list is non-empty, look for the
    pending-to-offer participant where the amount is closest to
    half (or the above reasonable fraction) of the imbalance
    error, and where the amount is lower than the imbalance
    error.
    * If so, forward the peerswap to that participant and remove
      it from the pending-to-offer.
  * If the incoming amount is greater than all the imbalance
    errors of the pending-to-offer list, or the pending-to-offer
    list is empty, accept the incoming peerswap.

Protocol Flow Detail
====================

* When the pool leader announces a swap party via a [SIDEPOOL-04][]
  `swap_party_begin` message, pool participants determine if they
  want to perform a peerswap with some other pool participant.
  If so, the pool participant becomes a peerswap offerrer and
  offers a peerswap-in-sidepool to the counterparty, giving it the
  in-sidepool HTLC details (amount, its HTLC ephemeral and
  persistent public keys, hash, minimum timelock, etc.).
* The counterparty determines if it wants to accept the given
  peerswap, and if so, responds with its assent and the
  in-sidepool HTLC details (its HTLC ephemeral and persistent
  public keys, timelock, etc.).
  It is now the peerswap acceptor.
* The peerswap offerrer creates the HTLC output using
  [SIDEPOOL-04 Expansion Phase Requests][].
* Both offerrer and acceptor wait for [SIDEPOOL-04 Expansion
  Phase State Validation][] and check if the HTLC output was
  created, stopping this protocol (without aborting the sidepool)
  if the HTLC output was not created.
* The acceptor waits for [SIDEPOOL-04 Expansion Phase
  Completion][].
  Once complete, the acceptor sends corresponding HTLCs over
  Lightning via standard BOLT #2 `update_add_htlc` messages.
* The offerrer waits for the in-Lightning HTLCs to become
  "irrevocably committed" as per [BOLT #2 Forwarding HTLCs][],
  then sends BOLT #2 `update_fulfill_htlc` with the preimage.
  The offerrer also hands over its HTLC ephemeral private key
  as per [SIDEPOOL-06 Secure Private Key Handover][].
  The offerrer can now forget the peerswap details.
* The acceptor claims the HTLC output using the
  [SIDEPOOL-04 Contraction Phase Requests][], as it now has
  possession of the HTLC aggregate ephemeral key.
* The acceptor waits for [SIDEPOOL-04 Contraction Phase State
  Validation][] and [SIDEPOOL-04 Contraction Phase
  Completion][].
  The acceptor can now forget the peerswap details.

[SIDEPOOL-04]: ./04-swap-party.md
[SIDEPOOL-04 Expansion Phase Requests]: ./04-swap-party.md#expansion-phase-requests
[SIDEPOOL-04 Expansion Phase State Validation]: ./04-swap-party.md#expansion-phase-state-validation
[SIDEPOOL-04 Expansion Phase Completion]: ./04-swap-party.md#expansion-phase-completion
[SIDEPOOL-04 Contraction Phase Requests]: ./04-swap-party.md#contraction-phase-requests
[SIDEPOOL-04 Contraction Phase State Validation]: ./04-swap-party.md#contraction-phase-state-validation

[SIDEPOOL-06 Secure Private Key Handover]: ./06-htlcs.md#secure-private-key-handover

[BOLT #2 Forwarding HTLCs]: https://github.com/lightning/bolts/blob/master/02-peer-protocol.md#forwarding-htlcs