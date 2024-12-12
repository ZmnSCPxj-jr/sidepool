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
However, HTLCs are neither here nor there, and channels may have
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
> on that channel (the current balance is dominated by the
> accumulation of payment flows over time).
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

Forwardable Peerswaps
=====================

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

Forwardable Peerswap Security
-----------------------------

While setting up a peerswap, the endpoint offerrer and the
endpoint acceptor exchange some public keys, as described in
[SIDEPOOL-06 HTLC Taproot Public Key][].

[SIDEPOOL-06 HTLC Taproot Public Key]: ./06-htlcs.md#htlc-taproot-public-key

The forwarding node cannot modify any of the public keys sent
between endpoint offerrer and endpoint acceptor.
Doing so would change what the endpoint offerrer and endpoint
acceptor expect to see in the sidepool output state, causing the
acceptor to believe that the peerswap did not push through (and
thus stop responding), and causing the offerrer to believe that
the acceptor aborted abnormally and that a sidepool abort is
necessary.
Any pool participant can already abort the entire sidepool at any
time for any reason, or for no reason, and the forwarder must by
necessity be a pool participant, so this is not an escalation.

The forwarding node can replace the acceptor public key with its
own so that it can acquire the funds, but again the endpoint
acceptor will not see the expected output on the sidepool output
state, and thus will not continue with the peerswap protocol.
This is no different from the case where the forwarding ndoe did
not forward in the first place and instead acted as the
terminating endpoint acceptor, so this is not an escalation.

During private key handover, as [SIDEPOOL-06 Secure Private Key
Handover][] requires that the private key of one endpoint be
encrypted to the public key of the other, the forwarding node
cannot decrypt the handed-over private key either.
As noted above, if the forwarding node had completely replaced the
ephemeral public key of either side, then the peerswap does not
continue, and would abort before either side can perform a
private key handover.

Even if the forwarding node *were* to get the handed-over private
key, two private keys are needed anyway, not just one, and normal
private key handover only hands over one key.

[SIDEPOOL-06 Secure Private Key Handover]: ./06-htlcs.md#secure-private-key-handover

Finally, there is no way for an endpoint acceptor, or an endpoint
offerrer, to determine if its direct peer in the peerwswap is
actually forwarding the peerswap or not, as the forwarder acts as
a proxy.
There exists a virtualization argument:
Any security flaw that exists in the forwarded case must by
necessity exist in the non-forwaded case, as neither endpoint can
be certain that they are talking to a proxy or not.
In other words, a "real" endpoint offerrer or acceptor can be
composed of multiple smaller components, one of which is directly
talking to the other "real" endpoint acceptor or offerrer and
translating its communication to other components of the node,
and if the protocol is secure in the non-forwarded case, it is
also secure in the case that the directly-communicating node is a
full Lightning Network node translating its communication to
another full Lightning Network node.
Both "real" endpoints need to validate the information it receives
over the peerswap-in-sidepool protocol vs. what happens in the
sidepool output state regardless of whether or not forwarding
occurred, and if validation succeeds, it does not matter to it
whether the protocol was done via proxy (forwarding) or directly.

Protocol Flow Overview
======================

Below is the overview of the "happy path" case where the forwardable
peerswap completes successfully:

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

[BOLT #2 Forwarding HTLCs]: https://github.com/lightning/bolts/blob/master/02-peer-protocol.md#forwarding-htlcs

Forwarding Peerswaps
--------------------

If a sidepool implementation also implements peerswap forwarding,
then it needs to store a different set of data in persistent
storage than from the endpoint offerrer and endpoint acceptor.

The endpoint offerrer and endpoint acceptor need to generate or
derive their persistent HTLC keys, and store the public persistent
key of the other, as well as generate ephemeral HTLC keys, and
store the aggregate ephemeral key once they have exchanged the
ephemeral HTLC public keys.
However, the forwarder does not need to store any of that, other
than for logging purposes.

What the forwarder MUST store is the forwarding relation between
the direct offerrer and the direct acceptor.
In particular, once the direct peerswap acceptor has given an HTLC
in the Lightning Network channel, the forwarder MUST store this
fact in persistent storage before it forwards the HTLC on the
Lightning Network channel to the direct peerswap offerrer.

The forwarder is not interested in any sidepool changes in state
(unless it is acting as an endpoint peerswap offerrer or endpoint
peerswap acceptor for a *different* peerswap), but MUST ensure
that Lightning Network state is correct across different channels.

In case the forwarder restarts and finds this fact in its
persistent store:

* If there is irrevocably committed no HTLC from the forwarder
  to the direct peerswap offerrer, the forwarder SHOULD fail
  the in-Lightning HTLC from the direct peerswap acceptor.
  * This condition is either the in-Lightning HTLC was not
    forwarded at all before the restart (i.e. `update_add_htlc`
    was never sent), or the HTLC was forwarded but then failed.
* Otherwise, the forwarder MUST wait for the in-Lightning
  HTLC to the direct peerswap offerrer to be either fulfilled,
  or irrevocably committed to fail, before fulfilling or failing
  the in-Lightning HTLC from the direct peerswap aceptor.
* The forwarder MUST handle the above even if the channel(s)
  with the direct peerswap offerrer, or the direct peerswap
  acceptor, have been unilaterally closed with the HTLCs
  in-flight.

> **Rationale** The above is already the behavior mandated by BOLT
> for normal payment forwards.
> However, actual Lightning Network node implementations might not
> expose the code that implements this behavior to code built on
> top of the implementation, including sidepool implementations
> not deeply integrated into the node implementation.
> This may preclude such implementations from being able to
> implement peerswap forwarding, or require re-implementation of
> that behavior in sidepool implementation code if the underlying
> implementation exposes enough functionality to implement this.
>
> Due to the possible need to re-implement it for sidepool
> implementations, the above requirements are re-iterated here.
>
> Even though the sidepool is supposed to abort if any participant
> suffers a crash or restart during a swap party (as MuSig2
> nonces are never persisted), the in-Lightning state is not
> in-pool state, and thus changes state independently of what
> happens with the sidepool.

In subsequent sections, this specification defines what a
forwarder must, should, and may do when forwarding a particular
message.

### 0-Fee Forwarded Peerswaps

Forwarders MUST NOT deduct a fee between the in-Lightning HTLC
from the direct peerswap acceptor and the in-Lightning HTLC to the
direct peerswap offerrer.
That is, the in-Lightning HTLC to the direct peerswap offerrer
MUST exactly equal the in-Lightning HTLC from the direct peerswap
acceptor.

> **Rationale** Every HTLC is a risk; in case of HTLC default, the
> constructs hosting the HTLC (the Lightning Network channel, or
> the sidepool) must be dropped onchain, with attendant onchain
> fees.
> Thus, HTLCs are not free and it is understandable that any
> involvement in HTLCs, including forwarding a peerswap, would
> induce a fee.
>
> * If a node does not want to forward peerswaps without being
>   compensated, it can simply not forward (i.e. not implement
>   peerswap forwarding, which is allowed but not recommended by
>   this specification as a "SHOULD implement").
>   HOWEVER:
>   * If the node does not forward, and:
>     * It has an imbalanced channel with another node that it
>       *would* want to offer a peerswap to.
>     * It currently has insufficient funds in the sidepool to
>       actually offer a peerswap to that node.
>   * ...then the node has to wait for at least the *next* swap
>     party before it can peerswap that channel:
>     It can accept any incoming peerswap now, but it cannot use
>     those in-sidepool funds until the next swap party.
>   * Thus, the node suffers from reduced expected fee earnings
>     from the imbalanced channel after a swap party that it has
>     no funds in the sidepool.
>     Yes there is a chance that the remaining capacity is enough,
>     there is also a chance (increasing with greater imbalance)
>     that a spike in volume will totally deplete the channel.
>   * Therefore, by forwarding the peerswap even though it gets 0
>     compensation for the forwarding, the node avoids paying an
>     opportunity cost in lost earnings by keeping that channel
>     imbalanced.
>     Every millisatoshi you save in not paying a cost is
>     equivalent to a millisatoshi you earn, thus forwarding a
>     peerswap is alreedy its own compensation, even if the node
>     earns 0 fees directly from peerswap forwarding.
>
> The expectation is that there is some level of in-Lightning
> HTLCs not defaulting (i.e. causing unilateral closes) at which
> point the effective cost of an HTLC drops below the expected
> cost of a channel being imbalanced enough to lose out on
> payment forwarding earnings.
> What that point is, is not mandated in this specification, and
> actual implementations are free to judge this for themselves
> using real-world data; the expectation is that forwarding is
> likely to remain viable and desirable even at 0 fee, as the
> fee here is effectively getting the channel rebalanced without
> having to pay for the rebalance yourself.

Protocol Flow Detail
====================

Initiating Peerswap
-------------------

A pool participant may initiate a peerswap and act as a peerswap
offerrer, where the offerrer offers in-sidepool funds for
Lightning funds from the acceptor, using the `peerswap_init`
message.

1.  `peerswap_init` (1000)
    - Sent from a peerswap offerrer to a peerswap acceptor.
2.  TLVs:
    * `peerswap_init_id` (100000)
      - Length: 16
      - Value: The sidepool identifier of the pool on which the
        peerswap offerrer wants to offer funds in exchange for
        Lightning funds.
      - Required.
    * `peerswap_init_pubkeys` (100002)
      - Length: 66
      - Value: A structure composed of:
        - 33 bytes: 33-byte DER encoding of the offerrer ephemeral
          public key for the in-sidepool HTLC.
        - 33 bytes: 33-byte DER encoding of the offerrer
          persistent public key for the in-sidepool HTLC.
      - Required.
    * `peerswap_init_timeouts` (100004)
      - Length: 8
      - Value: A structure composed of:
        - 4 bytes: unsigned big-endian 32-bit number, the
          timelock of the in-sidepool HTLC, a blockheight.
        - 4 bytes: unsigned big-endian 32-bit number, the
          timelock of the in-Lightning HTLC, a blockheight.
      - Required.
    * `peerswap_init_hash` (100008)
      - Length: 32
      - Value: The 256-bit hash, the hashlock of the HTLCs in the
        peerswap.
      - Required.
    * `peerswap_init_hops` (100010)
      - Length: 1
      - Value: unsigned 8-bit number, a number of allowed hops
        remaining for forwarding.
    * `peerswap_init_amount_sat` (100012)
      - Length: 8
      - Value: unsigned big-endian 64-bit number, the number of
        satoshis to be offered in-sidepool.
      - Required.
    * `peerswap_init_channel_view_msat` (100014)
      - Length: 24
      - Value: A structure composed of:
        - 8 bytes: `owned_by_offerrer`, an unsigned big-endian
          64-bit number, what the offerrer sees as the amount of
          millisatoshis it owns unambiguously in all channels with
          the acceptor, including reserve.
        - 8 bytes: `owned_by_acceptor`, an unsigned big-endian
          64-bit number, what the offerrer sees as the amount of
          millisatoshis the acceptor owns unambiguously in all
          channels with the offerrer, including reserve.
        - 8 bytes: `total_capacity`, an unsigned big-endian 64-bit
          number, what the offerrer sees as the total amount of
          millisatoshis in all channels with the acceptor,
          including reserve.
      - Required.
    * `peerswap_init_update_counter` (100016)
      - Length: 2
      - Value: an unsigned big-endian 16-bit number, the
        zero-extended value of the 11-bit update counter of the
        sidepool, as described in [SIDEPOOL-02 Pool Update
        Counter][].
        The value is the counter *before* the expansion phase of
        the swap party, and must be odd.
      - Required.

[SIDEPOOL-02 Pool Update Counter]: ./02-transactions.md#pool-update-counter

`peerswap_init_pubkeys` contains the ephemeral and persistent
public keys of the offerrer, as per [SIDEPOOL-06 HTLC Taproot
Public Key][].

[SIDEPOOL-06 HTLC Taproot Public Key]: ./06-htlcs.md#htlc-taproot-public-key

`peerswap_init_timeouts` contains the blockheights to be used for
the in-sidepool and in-Lightning HTLCs.

The offerrer:

* MUST set the in-sidepool timeout to greater than or equal to the
  current blockheight + 4032 + 1.
* MUST set the in-Lightning HTLC timeout to greater than or equal
  to the current blockheight + `final_cltv_delta`.
  * `final_cltv_delta` is the setting it would normally use for
    `c` / `min_final_cltv_expiry_delta` for invoices it would
    issue as per [BOLT #11 Tagged Fields Requirements][].
  * `cltv_expiry` MUST be less than or equal to 2016.

[BOLT #11 Tagged Fields Requirements]: https://github.com/lightning/bolts/blob/master/11-payment-encoding.md#requirements-3

The acceptor:

* MUST check that the in-sidepool timeout is greater than or equal
  to the current blockheight + 4032.
* MUST check that:
  * The in-Lightning timeout is less than the in-sidepool timeout.
  * The difference between the in-sidepool timeout minus the
    in-Lightning timeout is greater than or equal to its
    `cltv_delta`.
    * `cltv_delta` is the setting it would normally use for
      `cltv_expiry_delta` in [BOLT #7 The `channel_update`
       Message][].
    * `cltv_delta` MUST be less than or equal to 2016.

[BOLT #7 The `channel_update` Message]: https://github.com/lightning/bolts/blob/master/07-routing-gossip.md#the-channel_update-message

> **Rationale** The offerrer gives, for the in-sidepool timeout,
> blockheight + 4032 + 1 or higher, but the acceptor accepts
> any value >= blockheight + 4032, because it is possible for both
> to be out-of-sync.
> Thus, a small 1-block window is provided.
>
> In the worst case, a unilateral abort of the underlying
> sidepool mechanism would require 432 x 5 (maximum relative
> lock time for the First through Fifth update transactions
> in [SIDEPOOL-02 Pool Update Counter][] is 432;
> because the counter starts at 1, the Last update transaction
> will start at 0 relative lock time, and every higher update
> counter value will have equal or shorter time to complete
> the unilateral abort).
> This is 2160 blocks for a worst-case sidepool abort.
> The minimum of current blockheight + 4032 + 1 for the
> in-sidepool HTLC gives ample time for the HTLC to appear
> onchain in case of a sidepool abort.

`peerswap_init_hash` is the hash to be used in the hashlock
branches of both the in-sidepool and in-Lightning HTLCs.

`peerswap_init_hops` is the number of remaining hops allowed
for forwarding.
It MUST be a value between 0 to 10.
The ultimate offerrer SHOULD set this to a random value from
5 to 10.
The receiver of this messsage MUST NOT forward the peerswap
if the `peerswap_init_hops` value is 0.
Otherwise, if the receiver of this message decides to forward,
it MUST forward a `peerswap_init_hops` that is exactly 1 lower
than what it received.

`peerswap_init_amount_sat` is the amount, in satoshis, of the
in-sidepool HTLC that the offerrer proposes to the acceptor.

`peerswap_init_channel_view_msat` is what the offerrer believes
is the current state of all channels between the offerrer and
the acceptor.
This view may be stale, as HTLCs can continue to be offerred,
fulfilled, and failed asynchronously with the peerswap protocol.
The acceptor MUST NOT use this data to drive its decisionmaking;
it MUST use its own local view of the channel state.
This field only exists as an advisory.
However, in case the acceptor decides to reject the peerswap,
the acceptor SHOULD log the values here to assist in debugging.
The acceptor MAY log the values even if it accepts the peerswap.

`peerswap_init_update_counter` is the value of the pool update
counter *before* the swap party began.


A node:

* MAY send this message only after `swap_party_begin`:
  * If the node is a pool follower, after it has received
    `swap_party_begin`.
  * If the node is the pool leader, after it has arranged to
    send `swap_party_begin` to all followers.
* MUST NOT send this message to a peer that is not a participant
  of the sidepool it wants to use.
* MUST NOT send this message more than once to a peer until the
  swap party of the sidepool has completed, or the sidepool has
  aborted.
  - In case the node is a participant of multiple sidepools, and
    the peer is also a participant of multiple sidepools, the
    node MUST NOT send this message to a peer more than once
    until the swap party that triggered the first send has
    completed or that specific sidepool has aborted.
* MUST NOT send this message if it is not a valid offerrer to
  the peer.
  * A node is a valid offerrer to a peer if, for all the channels
    between this node and its peer, the [SIDEPOOL-10 Balance
    Measurement][] result:
    * The "twice our-side funds" of this node is less than "total
      channel capacity".
* MUST set `peerswap_init_amount_sat` such that:
  * It is greater than or equal to the minimum output amount
    specified in [SIDEPOOL-02 Output States][].
  * It is less than the "imbalance error" computed in [SIDEPOOL-10
    Balance Measurement][].
    Note that the "imbalance error" is in millisatoshis and
    `peerswap_init_amount_sat` is in satoshis.
* MUST NOT send this message to a peer if the number of
  in-Lightning HTLC splits exceeds `peerswap_max_ln_splits` (40),
  as determined below:
  - Determine the maximum HTLC of the channel(s) from the peer to
    itself, such as by getting the smallest `htlc_maximum_msat`
    from the [BOLT #7 `channel_update` Message][] from the peer,
    among all channels with that peer.
  - Compute
    `(peerswap_init_amount_sat * 1000) / htlc_maximum_msat`,
    rounded ***UP***.
  - If the result of the above division, rounded up, is 41 or
    more, then the node MUST NOT send this message.
* If the node cannot achieve all of the above, the node MUST NOT
  send this message.

[SIDEPOOL-10 Balance Measurement]: #balance-measurement
[BOLT #7 `channel_update` Message]: https://github.com/lightning/bolts/blob/8a64c6a1cef979b3f0cecb00ba7a48c2d28b3588/07-routing-gossip.md#the-channel_update-message

The receiver of this message:

* MUST check that it, and the sender, are both participants of
  the sidepool indicated in `peerswap_init_id`.
* MUST check if a swap party was recently started.
  * MUST defer processing of this message for up to 5 minutes if
    it is a pool follower and has not received `swap_party_begin`
    yet, and resume processing of this message if it
    receives `swap_party_begin` before then.
    If it does not receive `swap_party_begin` within 5 minutes,
    it MUST reject the offer.
  * MUST check that the pool update counter specified is correct
    for the current swap party.
* MUST check that the sender has not sent a previous
  `peerswap_init`, unless the previous `peerswap_init` is for a
  swap party that has completed, or a sidepool that has aborted.
* MUST validate that the sender is a valid offerrer:
  * The sender is a vliad offerrer to this node if, for all
    the channels between this node and the sender, the
    [SIDEPOOL-10 Balance Measurement][] result:
    * The "twice our-side funds" of this node is greater than or
      equal to the "total channel capacity".
* MUST check that the `peerswap_init_amount_sat` was set such that:
  * It is greater than or equal to the minimum output amount
    specified in [SIDEPOOL-02 Output States][].
  * It is less than the "imbalance error" computed in [SIDEPOOL-10
    Balance Measurement][].
    Note that the "imbalance error" is in millisatoshis and
    `peerswap_init_amount_sat` is in satoshis.
  * At least one channel with the sender has funds owned by this
    node, such that the amount, minus the reserve, is greater than
    or equal to `peerswap_init_amount_sat`.
* MUST check that the `peerswap_init_timeouts` is valid and
  acceptable to itself, as indicated above.
* MUST check that the `peerswap_init_hops` value is between 0 to 10,
  inclusive.

If the above validation fails, the receiver MUST reject the
peerswap.

The receiver MAY reject the peerswap for other reasons.

The receiver MAY receive a peerswap with the same sidepool ID
and hash from two different offerrers.
The receiver MUST NOT treat this as an error condition, and MUST
NOT abort the sidepool.
The receiver SHOULD accept the second `peerswap_init` even if
it has the same hash; the only requirement this specification
has is that a sender MUST NOT send `peerswap_init` twice (whether
hash is the same or not) to a peer until the swap party has
completed or the sidepool has aborted.

### Rejecting Peerswap

If the receiver of the `peerswap_init` message decides to
reject the peerswap, it MUST send `peerswap_reject`:

1.  `peerswap_reject` (1098)
    - Sent in response to `peerswap_init` if the sender decided to
      reject the peerswap.
2.  TLVs:
    * `peerswap_reject_id` (109800)
      - Length: 16
      - Value: The sidepool ID in the `peerswap_init` message.
      - Required.
    * `peerswap_reject_hash` (109802)
      - Length: 32
      - Value: The hash in the `peerswap_init` message.
      - Required.
    * `peerswap_reject_reason_code` (109804)
      - Length: 2
      - Value: An unsigned big-endian 16-bit number, indicating a
        machine-readable reason for why the peerswap was rejected.
      - Required.
    * `peerswap_reject_reason_text` (109806)
      - Length: N, 1 <= N <= 255
      - Value: A human-readable string encoded in UTF-8.
      - Optional.
    * `peerswap_reject_channel_view_msat` (109808)
      - Length: 24
      - Value: A structure composed of:
        - 8 bytes: `owned_by_offerrer`, an unsigned big-endian
          64-bit number, what the acceptor sees as the amount of
          millisatoshis the offerrer owns unambiguously in all
          channels with the acceptor, including reserve.
        - 8 bytes: `owned_by_acceptor`, an unsigned big-endian
          64-bit number, what the acceptor sees as the amount of
          millisatoshis it owns unambiguously in all channels with
          the offerrer, including reserve.
        - 8 bytes: `total_capacity`, an unsigned big-endian 64-bit
          number, what the acceptor sees as the total amount of
          millisatoshis in all channels with the offerrer,
          including reserve.
      - Required.

`peerswap_reject_reason_code` MAY have the following values:

| `peerswap_reject_reason_code` | Meaning                                        |
|-------------------------------|------------------------------------------------|
| 65535                         | Some policy or rule of this node was violated. |
| 0                             | No `swap_party_begin` received before timeout. |
| 1                             | `peerswap_init` already previously sent        |
| 2                             | Sender is not a valid offerrer.                |
| 3                             | `peerswap_init_amount_sat` too small.          |
| 4                             | `peerswap_init_amount_sat` too large.          |
| 5                             | Timeouts too tight.                            |
| 6                             | Too many splits.                               |
| 7                             | `peerswap_init_hops` out-of-range.             |

`peerswap_reject_reason_code` MAY be set to values not listed in
the above table; if the receiver does not recognize the value, it
SHOULD log the value.

`peerswap_reject_reason_text` is ostensibly a human-readable message
describing why the peerswap was rejected.
The receiver MUST be careful to sanitize the string of control
characters and other characters problematic for its setting before
displaying to human operators, and to make it clear that the message
comes from a remote node that may not be incentive-aligned to the
human operator.

`peerswap_reject_channel_view_msat` is what the rejecting node sees
as the state of the channels between itself and the offerrer.
The receiver SHOULD log this in addition to the
`peerswap_reject_reason_code` and `peerswap_reject_reason_text`,
as well as its local view of the channel state that it sent in the
`peerswap_init` message.

Receivers of this message:

* MUST check that the sidepool ID and hash corresponds to some
  previous `peerswap_init` it sent to the sender of the message
  for the current swap party.

If the above validation fails, the receiver SHOULD log the message
and otherwise ignore it.

Otherwise, if the receiver was the ultimate offerrer of the
peerswap (i.e. it did not forward the peerswap from another node)
then it MUST forget the peerswap and MUST NOT create an HTLC for
it in the sidepool, and MAY continue its participation in the swap
party.

### Forwarding Initiating And Rejecting Peerswap

The receiver of a `peerswap_init` message may choose to forward
the peerswap to another node.

* The forwarder MUST have successfully validated the received
  message as a receiver, as described above, including any policies
  it may have in addition to the required validations described in
  this specification.
* The forwarder MUST only forward the peerswap to another node if:
  - It is able to follow all the rules for a node sending this
    message to that node, as described above.
  - The `peerswap_init_hops` value in the received `peerswap_init`
    is non-0.

When forwarding, the following TLVs are modified.
If it is not possible to modify a TLV to follow one of the rules
indicated below, the node MUST NOT forward the peerswap.

* `peerswap_init_hops`
  - The outgoing value MUST be exactly 1 lower than the incoming
    value.
* `peerswap_init_timeouts`
  - The timelock of the in-sidepool HTLC MUST remain the same.
  - The timelock of the in-Lightning HTLC MUST be increased
    according to the `cltv_delta` of this node.
  - The difference between the in-sidepool and in-Lightning
    timelocks MUST be 2016 or more.
* `peerswap_init_channel_view_msat`
  - The forwarder MUST replace this entirely with its view of the
    channels between itself and the node it intends to forward the
    message to.

All other TLVs MUST remain the same.

If the node decides to forward the peerswap, it MUST store this
fact in persistent storage before forwarding `peerswap_init`.
It MUST store the following information at minimum before
forwarding `peerswap_init` to its chosen next node:

* The sidepool ID.
* The hash.
* The upstream offerrer.
* The downstream acceptor it chose to forward to.
* The offerrer ephemeral and persistent public keys for the
  in-sidepool HTLC.
* The in-sidepool HTLC timelock.
* The in-Lightning HTLC timelock from the upstream offerrer.
* The in-Lightning HTLC timelock it chose to give the downstream
  offerrer.
* The amount of the in-sidepool HTLC.

In case of a sidepool abort, it is possible that a forwarded
in-Lightning HTLC exists.

If the next hop responds with a `peerswap_reject`, the forwarder:

* MAY try to forward to a different node, updating the persistent
  store as necessary.
* Otherwise, SHOULD accept the peerswap at this node.
  * The forwarder MUST delete the fact that it wanted to forward
    the peerswap from persistent storage before accepting it at
    this node.
  * MAY reject the peerswap, but MUST generate its own
    `peerswap_reject` and related information.

> **Rationale** The forwarder must have validated the
> `peerswap_init` from the offerrer before it decided to forward
> the peerswap.
> Thus, the forwarder must already have been prepared to accept
> the peerswap from the offerrer, even if the downstream acceptor
> rejected the peerswap.

Accepting Peerswap
------------------

If the receiver of the `peerswap_init` decides to accept the
peerswap, it is now a peerswap acceptor, and is the ultimate
acceptor of the peerswap.

The acceptor also generates its in-sidepool HTLC ephemeral and
persistent keypairs, as described in [SIDEPOOL-06 HTLC Taproot
Public Key][].

The acceptor then responds with a `peerswap_accept`:

1.  `peerswap_accept` (1002)
    - Sent by an acceptor in response to an accepted
      `peerswap_init`.
2.  TLVs:
    * `peerswap_accept_id` (100200)
      - Length: 16
      - Value: The sidepool identifier in the `peerswap_init`
        message.
      - Required.
    * `peerswap_accept_hash` (100202)
      - Length: 32
      - Value: The 256-bit hash, the hashlock of the HTLCs in the
        peerswap.
      - Required.
    * `peerswap_accept_pubkeys` (100204)
      - Length: 66
      - Value: A structure composed of:
        - 33 bytes: 33-byte DER encoding of the acceptor ephemeral
          public key for the in-sidepool HTLC.
        - 33 bytes: 33-byte DER encoding of the acceptor
          persistent public key for the in-sidepool HTLC.
      - Required.

`peerswap_accept_hash` is the hash indicated in the
`peerswap_init` message.

`peerswap_accept_pubkeys` contains the ephemeral and persistent
public keys of the acceptor, as per [SIDEPOOL-06 HTLC Taproot
Public Key][].

The receiver of this message MUST perform the validations below:

* It previously sent `peerswap_init` to the sender, and the
  sidepool ID and hash matches the one in `peerswap_init`.

If the above validation fails, the receiver of the message SHOULD
log and MUST ignore the message.

If the above validation succeeds, the receiver of the message MUST
perform the validations below:

* `peerswap_accept_min_htlc_msat` is less than or equal to the
  `peerswap_init_min_htlc_msat` it sent.
* The value
  `(peerswap_init_amount_sat * 1000) / peerswap_accept_min_htlc_msat`,
  rounded up, is less than or equal to `peerswap_max_splits` (40).

If the above validation fails, the receiver of the message MUST
abort the sidepool.

### Forwarding Accepting Peerswap

If the sender of `peerswap_init` forwarded it from another peer,
and it receives a `peerswap_accept`:

* It MUST validate the message as described above, as a receiver
  of the `peerswap_accept`.
* It MUST forward the message as-is to the peer it forwarded the
  peerswap from.

Creating Sidepool HTLC
----------------------

On receiving the `peerswap_accept`, the ultimate offerrer of the
peerswap constructs the in-sidepool HTLC address from the public
keys and the HTLC details, as per
[SIDEPOOL-06 HTLC Taproot Public Key][].

The ultimate offerrer then includes the address, as well as the
amount it indicated in the `peerswap_init_amount_sat`, in the
`swap_party_expand_request` message to the pool leader if it is a
pool follower, or equivalent functionality if it is the pool
leader.
See [SIDEPOOL-04 Expansion Phase Requests][].

Once the offerrer, any forwarders, and the acceptor have finished
the expansion phase request, they then wait for [SIDEPOOL-04
Expansion Phase State Validation][].

At this point, the offerrer, forwarders, and acceptor MUST:

* Check if the [SIDEPOOL-06 HTLC Taproot Public Key][] was created
  in the expansion phase, with an amount exactly equal to the
  `peerswap_init_amount_sat`.

If the above validation fails, the offerrer, forwarders, and
acceptor simply forget this peerswap.
The offerrer, forwarders, and acceptor MUST NOT abort the sidepool
simply because the above validation fails (but MAY abort the
sidepool for other reasons).

The offerrer, since it is the pool participant that requests for
the creation of the HTLC output, MUST follow the validation rules
indicated in [SIDEPOOL-04 Expansion Phase State Validation][] if
it is a pool follower and the in-sidepool HTLC output does not
exist.

The forwarders and acceptor do not have any additional rules or
specified behavior if the in-sidepool HTLC output does not exist.

After the offerrer has completed the expansion phase state
validation and provided its signature for the expansion phase
state, the offerrer proceeds to the in-Lightning HTLC
propagation step.

In-Lightning HTLC Propagation
-----------------------------

In-Lightning HTLC propagation consists of two stages:

* Propagate `peerswap_lightning` messages from the ultimate
  offerrer to the ultimate acceptor.
* Propagate in-Lightning HTLCs (via BOLT #2 `update_add_htlc`
  messages) from the ultimate acceptor to the ultimate offerrer.

The `peerswap_lightning` message contains information that the
local acceptor will use to pay to the sender.

1.  `peerswap_lightning` (1004)
    - Sent by an offerrer to inform its acceptor about the
      payment onion details.
2.  TLVs:
    * `peerswap_lightning_id` (100400)
      - Length: 16
      - Value: The sidepool identifier in the `peerswap_init`
        message.
      - Required.
    * `peerswap_lightning_hash` (100402)
      - Length: 32
      - Value: The 256-bit hash, the hashlock of the HTLCs in the
        peerswap.
      - Required.
    * `peerswap_lightning_payment_secret` (100404)
      - Length: 32
      - Value: A 32-byte secret that the offerrer will use to
        identify this payment as being part of a
        peerswap-in-sidepool.
        The local acceptor will encode this as the
        `payment_secret` of the `payment_data` field inside the
        onion to the sender.
      - Optional.
    * `peerswap_lightning_payment_metadata` (100406)
      - Length: 1 <= N <= 16384
      - Value: A variable-length arbitrary field that the offerrer
        will use to identify this payment as being part of a
        peerswap-in-sidepool.
        The local acceptor will encode this as the
        `payment_metadata` field inside the onion to the sender.
      - Optional.

`peerswap_lightning_payment_secret` and
`peerswap_lightning_payment_metadata` are optional fields that the
sender may include.
They will be the `payment_secret` and `payment_metadata` fields
that the local acceptor will use in the payment onion to the local
offerrer that sends the `peerswap_lightning` message.

The receiver of `peerswap_lightning` MUST perform the validations
below:

* It previously sent `peerswap_accept` to the sender, and the
  sidepool ID and hash matches the one in `peerswap_accept`.

If the above validation fails, the receiver of the message SHOULD
log and MUST ignore the message.

On receiving this message, the local acceptor MUST arrange to
get one or more in-Lightning HTLCs to the sender:

* If the local acceptor is a forwarder, it MUST forward a
  `peerswap_lightning` to its own local acceptor, wait for
  incoming HTLCs from that acceptor, and *then* send a number of
  HTLCs to the local offerrer.
* Otherwise, the local acceptor is the ultimate acceptor, and it
  MUST send a number of HTLCs.

### Agreed-upon Amount of In-Lightning HTLCs

The "*agreed upon amount*" of the in-Lightning HTLCs is the
"post-tax amount" of the in-sidepool HTLC of this peerswap, as
defined in [SIDEPOOL-02 Onchain Fee Charge Distribution][].

[SIDEPOOL-02 Onchain Fee Charge Distribution]: ./02-transactions.md#onchain-fee-charge-distribution

The total amount to be sent over each hop of the forwarded
peerswap on the Lightning Network is equal to the "post-tax
amount" of the in-sidepool HTLC.

### Forwarding `peerswap_lightning`

A forwarder MUST NOT copy the `peerswap_lightning` message
verbatim.

On receiving `peerswap_lightning`, a forwarder MUST persistently
record `peerswap_lightning_payment_secret` and
`peerswap_lightning_payment_metadata`, if any.

Then, the forwarder MUST construct its own `peerswap_lightning`
message.
The forwarder MAY include its own generated
`peerswap_lightning_payment_secret`, its own generated
`peerswap_lightning_payment_metadata`, both, or neither, in the
`peerswap_lightning` message it sends to its own local acceptor.

### Offerring In-Lightning HTLCs

A peerswap acceptor will offer in-Lightning HTLCs (i.e. the
Lightning HTLCs are in the reverse direction, to compensate the
initial offerrer).

The ultimate acceptor, once it has received `peerswap_lightning`
from the local offerrer, MUST immediately offer the in-Lightning
HTLCs.

A forwarder MUST instead wait for its own local acceptor to offer
*all* in-Lightning HTLCs before itself offerring *any* HTLCs to
the local offerrer.

An acceptor (whether the ultimate acceptor, or a forwarder), once
it is time to offer the in-Lightning HTLCs, first calculates the
"agreed-upon amount" of the in-Lightning HTLCs, as above.

Then, it makes a number of payments to the local offerrer.
Each payment MUST NOT exceed any limit on the maximum HTLC to the
local offerrer, such as any `htlc_maximum_msat` sent by the
acceptor to the channel.

The acceptor MAY use any valid direct channel with its local
offerrer for any part of the overall payment.
Each payment MUST be a single hop, from the acceptor to its local
offerrer.

The acceptor MAY pay using one payment if the maximum HTLC size to
the local offerrer is greater than or equal to the "agreed-upon
amount".
Otherwise, the acceptor MUST make multiple payments, each smaller
than the maximum HTLC, all totalling to the "agreed-upon amount".

For each payment sent, the following MUST be true:

* The `update_add_htlc`:
  * The `payment_hash` MUST equal `peerswap_init_hash`,
    `peerswap_accept_hash`, and `peerswap_lightning_hash` (which
    are all equal, so it is sufficient to just compare against one
    of them).
  * The `amount_msat` MUST be less than the maximum HTLC size.
  * The `cltv_expiry` MUST equal the timelock of the in-Lightning
    HTLC that was sent by the offerrer in `peerswap_init_timeouts`.
  * The `onion_routing_packet` MUST be a one-hop payment onion
    terminating at the direct peer (the local offerrer):
    * The `amt_to_forward` and `total_msat` MUST equal the
      `amount_msat` above.
    * The `outgoing_cltv_value` MUST equal `cltv_expiry` above.
    * If `peerswap_lightning_payment_secret` existed in the
      incoming `peerswap_lightning`, then the `payment_secret`
      field in the payment onion MUST exist and equal that value.
      Otherwise, it MUST NOT exist.
    * If `peerswap_lightning_payment_metadata` existed in the
      incoming `peerswap_lightning`, then the `payment_metadata`
      field in the payment onion MUST exist and equal that value.
      Otherwise, it MUST NOT exist.
    * The `short_channel_id` MUST NOT exist (i.e. this is the
      final hop).

If an acceptor is unable to send out all the payments (for
example, due to a lack of capacity) then the acceptor MUST stop
offerring payments, and MUST wait for *all* the
previously-offerred in-Lightning HTLCs to be failed by the local
offerrer.

### Accepting In-Lightning HTLCs

An offerrer waits for in-Lightning HTLCs from the local acceptor.

An offerrer waits until the total in-Lightning HTLCs that have
been identified as part of the peerswap-in-sidepool (by recognizing
the hash, timelock, and any `payment_secret` and/or
`payment_metadata`), which have been irrevocably committed, have
totalled to equal or greater than the "agreed-upon amount" of the
in-Lightning HTLCs.

Until the amount reaches the agreed-upon amount, the offerrer MUST
hold the incoming in-Lightning HTLCs.
The offerrer MUST hold for up to 60 seconds, starting from when
the first incoming in-Lightning HTLC has been irrevocably committed.
Once this timer times out, the offerrer MUST fail (via
`update_fail_htlc`) all incoming in-Lightning HTLCs for this
peerswap-in-sidepool.
The offerrer MAY use any payment failure code.

Once the total in-Lightning HTLCs of the peerswap-in-sidepool
have been irrevocably committed, and the total amount equals or
exceeds the "agreed-upon amount":

* If the offerrer is the ultimate offerrer, it proceeds to
  peerswap resolution below, in the success path.
* Otherwise, the offerrer is a forwarder.
  It then offers in-Lightning HTLCs to its own local offerrer,
  as above.

> **Rationale** The forwarding of in-Lightning HTLCs in a
> forwardable peerswap all occur along a single path.
> Thus, there is no need for multipart payments where individual
> parts may take different paths to the same destination.
> Instead, each forwarder waits for the total "agreed-upon amount"
> to be offerred and irrevocably committed by its local acceptor
> before proceeding to forward.
> This allows each hop to handle its own `htlc_maximum_msat`
> setting without requiring that the entire path agree on a single
> maximum HTLC; the effective maximum is the current capacity of
> all active channels between any two participants in the
> forwardable peerswap.

### Failing In-Lightning HTLCs

If the 60-second hold timeout is reached, as noted above, the
local offerrer (which accepts in-Lightning HTLCs) MUST fail *all*
HTLCs of the peerswap-in-sidepool, including those that arrive
after the timeout.
This holds for both forwarders and the ultimate offerrer.

For a forwarder:

* If *any* in-Lightning HTLC it offerred to the local offerrer is
  fulfilled, then the forwarder MUST stop the above 60-second hold
  timeout, then fulfill *all* in-Lightning HTLCs it accepted from
  the local acceptor.
* If *all* in-Lightning HTLCs it offerred to the local offerrer
  are failed, *AND* all of them are irrevocably committed to be
  removed from their channels, then the forwarder MUST fail *all*
  in-Lightning HTLCs it accepted from the local acceptor.

For the ultimate acceptor:

* If *all* in-Lightning HTLCs it offerred to the local offerrer
  are failed, *AND* all of them are irrevocably committed to be
  removed from their channels, then the ultimate acceptor proceeds
  to the peerswap resolution below, in the fail path.

Peerswap Resolution
-------------------

Peerswap resolution is performed in one of two paths:

* The success path, which the ultimate offerrer initiates once it
  has accepted in-Lightning HTLCs totalling or exceeding the
  "agreed-upon amount", and all those HTLCs have been irrevocably
  committed.
  * The ultimate offerrer sends out `peerswap_success`, which
    forwarders then forward to the ultimate acceptor.
* The fail path, which the ultimate acceptor initiates once *all*
  in-Lightning HTLCs it offerred have been failed, and all those
  HTLCs have been irrevocably committed to be removed from their
  channels.
  * The ultimate acceptor sends out `peerswap_fail`, which
    forwarders then forward to the ultimate offerrer.

In both cases, [SIDEPOOL-06 Private Key Handover][] is performed:

* In the success path, the ultimate offerrer hands over its
  ephemeral private key to the ultimate acceptor.
* In the fail path, the ultimate acceptor hands over its ephemeral
  private key to the ultimate offerrer.

[SIDEPOOL-06 Private Key Handover]: ./06-htlcs.md#secure-private-key-handover

### Success Path

In the success path (the ultimate offerrer receives total
in-Lightning HTLCs equal or exceeding the agreed-upon amount), the
ultimate offerrer MUST send `peerswap_success`.

The ultimate offerrer MUST also fulfill each of the incoming
in-Lightning HTLCs.

The ultimate offerrer MAY fulfill the in-Lightning HTLCs in any
order, and MAY send `peerswap_success` before or after fulfilling
any in-Lightning HTLCs, or interspersed with fulfilling the
in-Lightning HTLCs.
i.e. there is no ordering requirement to `peerswap_success` with
respect to the in-Lightning HTLCs being fulfilled.

1.  `peerswap_success` (1010)
    - Sent by an offerrer to its local acceptor to indicate that
      the ultimate offerrer has received the corresponding amount.
2.  TLVs:
    * `peerswap_success_id` (101000)
      - Length: 16
      - Value: The sidepool identifier of the pool on which the
        offerrer signals success of the peerswap.
      - Required.
    * `peerswap_success_hash` (101002)
      - Length: 32
      - Value: The 256-bit hash, the hashlock of the HTLCs in the
        peerswap.
      - Required.
    * `peerswap_success_ephemeral_privkey` (101004)
      - Length: 65
      - Value: The encrypted offerrer ephemeral private key, as
        described in [SIDEPOOL-06 Secure Private Key Handover][].
        This is the concatenation of:
        * 32 bytes: the encrypted offerrer ephemeral private key.
        * 33 bytes: the compressed SEC encoding of the encryption
          public key.
      - Required.

TODO

### Fail Path

TODO

In-Sidepool HTLC Claiming
-------------------------

TODO
