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
> implementation exposes enough functionality to implement this
> (such as CLN `htlc_accepted` hook, which are re-issued on
> restart and thus on restart allow the incoming HTLC to remain
> in-flight, but not LDK interception SCIDs, which fail the
> incoming HTLCs on restart if forwarding is not delegated to
> LDK; in particular, LDK forwarding requires a fee equal to or
> greater than the normal channel fee to be charged, but this
> specification mandates that peerswap forwarders do not charge a
> fee in the Lightning part of the forwarded peerswap, CLN
> allows you to override this if you implement the persistent
> tracking of HTLC forwardings yourself).
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
    * `peerswap_init_offerrer` (100010)
      - Length: 33
      - Value: A DER-encoded 33-byte node ID.
      - Required.
    * `peerswap_init_cltv_final` (100012)
      - Length: 4
      - Value: unsigned big-endian 32-bit number, a number of
        blocks to use as the `min_final_cltv_expiry_delta`.
      - Required.
    * `peerswap_init_routehints` (100014)
      - Length: N * (33 + 8 + 2), 0 <= N <= 5
      - Value: An array of structures:
        - 8 bytes: short channel ID to the above hop.
        - 2 bytes: unsigned big-endian 16-bit number, the
          `cltv_expiry_delta` for this hop.
        - 33 bytes: DER-encoded 33-byte node ID.
      - Required.
    * `peerswap_init_amount_sat` (100016)
      - Length: 8
      - Value: unsigned big-endian 64-bit number, the number of
        satoshis to be offered in-sidepool.
      - Required.
    * `peerswap_init_channel_view_msat` (100018)
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
    * `peerswap_init_update_counter` (100020)
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

`peerswap_init_offerrer` is the ultimate offerrer of the peerswap.
If the message sender is the actual ultimate offerrer, this MUST
be its own node ID.
If the message sender is a forwarder, then this MUST NOT be its
own node ID, but be copied verbatim from the incoming
`peerswap_init`.

`peerswap_init_cltv_final` is the final CLTV-delta for the
offerrer.

`peerswap_init_routehints` is the sequence of routehints that it
took from the ultimate offerrer to the current offerrer.
If the message sender is the actual ultimate offerrer, this MUST
be empty.
If the message sender is a forwarder, then this MUST be non-empty,
and would contain at least the hop from the previous offerrer to
the forwarder.
The short channel ID MAY be any value, including an impossible or
nonexistent channel;
the receiver MUST ignore the value and simply copy it verbatim.

`peerswap_init_amount_sat` is the amount, in satoshis, of the
in-sidepool HTLC that the offerrer proposes to the acceptor.

`peerswap_init_channel_view_msat` is what the offerrer believes
is the current state of all channels between the offerrer and
the acceptor.
This view may be stale, as HTLCs can continue to be offerred,
fulfilled, and failed asynchronously with the peerswap protocol.
The acceptor MUST NOT use this data to drive its decisionmaking;
it MUST use its own local view of the channel state.
However, in case the acceptor decides to reject the peerswap,
the acceptor SHOULD log the values here to assist in debugging.
The acceptor MAY log the values even if it accepts the peerswap.

`peerswap_init_update_counter` is the value of the pool update
counter before the swap party begins.

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
  * At least one channel with the peer has funds owned by that
    peer, such that the amount, minus the reserve, is greater than
    or equal to `peerswap_init_amount_sat`.
  * If the node cannot achieve all of the above, the node MUST NOT
    send this message.

[SIDEPOOL-10 Balance Measurement]: #balance-measurement

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
  previous `peerswap_init` it sent for the current swap party.

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
* The forwarder MUST only forward the peerswap to another node if
  it is able to follow all the rules for a node sending this
  message to that node, as described above.

When forwarding, the following TLVs are modified.
If it is not possible to modify a TLV to follow one of the rules
indicated below, the node MUST NOT forward the peerswap.

* `peerswap_init_timeouts`
  - The timelock of the in-sidepool HTLC MUST remain the same.
  - The timelock of the in-Lightning HTLC MUST be increased
    according to the `cltv_delta` of this node.
  - The difference between the in-sidepool and in-Lightning
    timelocks MUST be 2016 or more.
* `peerswap_init_routehints`
  - The forwarder MUST select any short channel ID, with any
    value (existing or not, plausible or not).
  - The forwarder appends the selected short channel ID and its
    own node ID, as well as its typical `cltv_expiry_delta`, as a
    new entry at the end of this array.
  - The resulting array MUST be of length 5 or shorter.
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
peerswap, it is now a peerswap acceptor.

The acceptor first generates the hop from itself to the
offerrer, by selecting any short channel ID (existing or not,
plausible or not).
It then provides its `cltv_expiry_delta` and the node from
which it received the `peerswap_init`, and adds it to the
routehint it received, then feeds that routehing back to
the offerrer.

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
    * `peerswap_accept_routehints` (100206)
      - Length: N * (33 + 8 + 2), 1 <= N <= 6
      - Value: An array of structures:
        - 8 bytes: short channel ID to the above hop.
        - 2 bytes: unsigned big-endian 16-bit number, the
          `cltv_expiry_delta` for this hop.
        - 33 bytes: DER-encoded 33-byte node ID.
      - Required.

`peerswap_accept_hash` is the hash indicated in the
`peerswap_init` message.

`peerswap_accept_pubkeys` contains the ephemeral and persistent
public keys of the acceptor, as per [SIDEPOOL-06 HTLC Taproot
Public Key][].

`peerswap_accept_routehints` contains the route to be used from
the ultimate offerrer (which will receive the corresponding
in-Lightning payment) to the ultimate acceptor (which will send the
in-Lightning payment).
The ultimate acceptor MUST take the routehints it got from
`peerswap_init` and append exactly one entry to it, containing the
the short channel ID it chose, its `cltv_expiry_delta`, and its own
node ID.

The receiver of this message MUST perform the validations below:

* It previously sent `peerswap_init` to the sender, and the
  sidepool ID and hash matches the one in `peerswap_init`.

If the above validation fails, the receiver of the message SHOULD
log and ignore the message.

If th above validation succeeds, the receiver of the message MUST
perform the validations below:

* The routehints includes the routehints it sent in the
  `peerswap_init` as a prefix, and is at least one entry longer.

If the above validation fails, the receiver of the message MUST
abort the sidepool.

### Forwarding Accepting Peerswap

If the sender of `peerswap_init` forwarded it from another peer,
and it receives a `peerswap_accept`:

* It MUST validate the message as described above.
* It MUST forward the message as-is to the peer it forwarded the
  peerswap from.

Creating Sidepool HTLC And BOLT11 Invoice
-----------------------------------------

On receiving the `peerswap_accept`, the ultimate offerrer of the
peerswap constructs the in-sidepool HTLC address as per
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

The offerrer, since it is the pool participant that requests for
the creation of the HTLC output, MUST follow the validation rules
indicated in [SIDEPOOL-04 Expansion Phase State Validation][] if
it is a pool follower and the in-sidepool HTLC output does not
exist.

The forwarders and acceptor do not have any additional rules or
specified behavior if the in-sidepool HTLC output does not exist.

After the offerrer has completed the expansion phase state
validation and provided its signature for the expansion phase
state, the offerrer generates the BOLT11 invoice.

> **Rationale** A BOLT11 invoice is the least common denominator
> that allows implementation of a forwardable peerswap acceptor
> across all extant Lightning Network node implementations.
>
> The routehint built by the `peerswap_init`-`peerswap_accept`
> protocol is necessary as LDK HTLC interception requires that
> an SCID be acquired from LDK, which will then be used to
> trigger interception of the HTLC, in order to override the
> normal fee requirements of LDK forwarding.
> This information needs to be transferred from all forwarders
> to the acceptor.
> This is the reason why the short channel ID is allowed to be
> any value, including impossible or invalid values.
>
> Other Lightning Network node implementations allow for more
> flexible interception, in order to implement 0-fee forwarding,
> and thus may use any short channel ID, including a constant
> invalid short channel ID like 0x0x0.
> Such implementations can trigger interception and 0-fee
> forwarding by inspecting the payment hash instead of the short
> channel ID.
>
> In theory, the route to the ultimate offerrer could have been
> built as a BOLT12 blinded route instead.
> However, blinded route support is not as widely-deployed at the
> time of this routing as BOLT11 routehint support; in particular,
> sending to a blinded route may not have easy-to-use interfaces
> on all implementations, but paying to a BOLT11 invoices is
> readily available on all implementations.

The BOLT11 invoice is sent by the offerrer to the acceptor via the
`peerswap_invoice` message:

1.  `peerswap_invoice` (1004)
    - Sent by the offerrer to the acceptor to indicate the BOLT11
      invoice.
2.  TLVs:
    * `peerswap_invoice_id` (100400)
      - Length: 16
      - Value: The sidepool ID that the peerswap is done inside
        of.
      - Required.
    * `peerswap_invoice_bolt11` (100402)
      - Length: N
      - Value: The ASCII encoding of the BOLT11 invoice.
      - Required.

The receiver of the `peerswap_invoice` message MUST perform the
validation below:

* It recognizes the sidepool ID, and that the sender and the
  receiver are both participants of the indicated sidepool.
* It parses the invoice and extracts the `payment_hash` / `p`,
  which must match the `peerswap_init_hash` of some peerswap it
  accepted before, whether as a forwarder or as an ultimate
  acceptor, from the sender.

If any of the above checks fail, the receiver MUST ignore the
message and SHOULD log it is an unusual event.

Then, the forwarder or acceptor MUST validate the BOLT11 invoice
in detail as specified in the next section.

### BOLT11 Invoice Generation And Validation

The offerrer, any forwarders, and the acceptor, MUST determine
the following:

* The "post-tax amount" of the in-sidepool HTLC output, as per
  [SIDEPOOL-02 Onchain Fee Charge Distribution][], for the
  expansion phase state.

[SIDEPOOL-02 Onchain Fee Charge Distribution]: ./02-transactions.md#onchain-fee-charge-distribution

The offerrer MUST set the following bits in the BOLT11 feature
bits array `9`:

* Bit 8: require `var_onion_optin`.
* Bit 14: require `payment_secret`.

The offerrer MUST NOT set any other bits, even if it would
normally set them for "normal" invoices.

The offerrer MUST set the human-readable part as per
[BOLT #11 Human-Readable Part][]:
The forwarders and the acceptor MUST check these as well.

* The `prefix` is correct for the network the offerrer and the
  sidepool is operating in.
* The `amount` is the **"post-tax amount"** of the in-sidepool
  HTLC.
  * This will usually be less than the `peerswap_init_amount_sat`,
    but may be larger if the pool leader selected the in-sidepool
    HTLC as the onchain fee tax overpayment beneficiary.

[BOLT #11 Human-Readable Part]: https://github.com/lightning/bolts/blob/master/11-payment-encoding.md#human-readable-part

The offerrer then sets the data part as per [BOLT #11 Data Part][].
The forwarders and the acceptor also validate these as well.
All tagged fields MUST be indicated at most once, and not repeated.

* The `timestamp` MUST be set to offerrer view of the current
  time.
  The acceptor MUST accept a `timestamp` of up to one hour before
  to one hour after its view of the current time, while any
  forwarders MUST ignore the timestamp.
* The `payment_hash` / `p` MUST be set to the value indicated in
  `peerswap_init_hash`.
* The `payment_secret` / `s` MUST be set.
  The offerrer MAY set it to any value, and SHOULD set it to a
  256-bit number indistinguishable from random.
* The `description` / `d` MUST be set to the ASCII encoding of
  the exact string `"forwardable-peerswap"` *without* the double
  quotation marks and *without* a trailing NUL character.
* The `metadata` / `m` MUST NOT be set.
* The `destination` / `n` MUST be set to the offerrer indicated
  in `peerswap_init_offerrer`.
* The `description_hash` / `h` MUST NOT be set.
* The `expiry` / `x` MUST be set to 24 * 60 * 60 = 86400 (1 day).
* The `min_final_cltv_expiry_delta` / `c` MUST be set to the
  `peerswap_init_cltv_final`.
* The `fallback_address` / `f` MUST NOT be set.
* The `routehint` / `r` MUST be set, and contains the routehints
  indicated in the `peerswap_accept_routehints`.
  The `fee_base_msat` and `fee_proportional_millionths` fields of
  each entry MUST be set to 0.
* The `featurebits` / `9` MUST be set, to the feature bits
  specified above.
* Tagged fields other than the above MUST NOT be set.

[BOLT #11 Data Part]: https://github.com/lightning/bolts/blob/master/11-payment-encoding.md#data-part

The invoice MUST be signed.

If any of the above validations fail for a forwarder or for the
acceptor, the forwarder or acceptor MUST abort the sidepool.

Any forwarders MUST forward the `peerswap_invoice` message
verbatim.

The acceptor performs the following additional validation:

* The current blockheight, plus the `min_final_cltv_expiry_delta`
  / `c`, plus all of the `cltv_expiry_delta`s in the `routehints`
  / `r`, must be less than or equal to the in-sidepool timeout
  indicated in `peerswap_init_timeouts`.

If the above validation fails, the acceptor MUST refund the
peerswap, as per [SIDEPOOL-10 Refunding Peerswap][] instead of
attempting to pay the invoice.

(TODO)
