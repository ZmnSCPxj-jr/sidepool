SIDEPOOL-05 Reseat And Splicing

Overview
========

Sidepool version 1 pools have a limited number of updates, as
specified in [SIDEPOOL-02 Pool Update Counter][].
Once a pool has reached the maximum number of updates, it can
only be closed.

[SIDEPOOL-02 Pool Update Counter]: ./02-transactions.md#pool-update-counter

Rather than always closing the pool when the maximum number of
updates has been reached, sidepool version 1 pools instead
performs a "*reseat*".
A *reseat* closes the pool and cuts-through the closing
transaction with the opening of a new pool whose initial state is
based on the final state of the pool, thus effectively continuing
the lifetime of the pool with a new funding transaction output.

A reseat is very similar to what is called a *splice* in the
Lightning Network, wherein a channel is closed but is immediately
re-opened, with a change in capacity of the channel (i.e. funds
can be added by a participant via a splice-in, or removed by a
participant via a splice-out).
Splice-ins and splice-outs would be as useful to sidepools as they
would be in a Lightning Network channel, thus, we also want to
define a specification for splicing.
A reseat can be considered as a splice operation where funds are
not added or removed from the sidepool, i.e. a "no-op" splice.

Finally, or splice is also an opportunity for a pool follower to
leave the sidepool entirely, or for a new pool follower to join.
Thus, all of these operations are described in this single
specification.

Onchain Fee Negotiation
=======================

Onchain feerate disagreements have classically been a problem on
the Lightning Network.
During normal operation, the channel initiator dictates a
feerate, then the channel acceptor either accepts it, or rejects
it by forcing an expensive unilateral close.
In effect, the participants are continuously playing "ultimatum"
games with each other.
It is a common case where one participant has a particularly
different view of the mempool (possibly exacerbated by a sudden
influx of new unconfirmed transactions, which have to start
*somewhere* and might start nearer to one node than to another),
leading to disagreement on what is a "reasonable" fee.

The Lightning Network also has a less "ultimatum"-type fee
negotiation during a negotiated mutual close.
During mutual closes, both sides name a fee, then if they are
different, they advance the mutual close by naming a new fee
that is at least one satoshi nearer to the other fee.
While many implementations just "jump" their fee by a large
fraction of the difference, in theory an implementation could
"play hardball" by only moving the fee by the requisite minimum of
1 satoshi
(CLN has an option where it does this, for example).
If both participants "play hardball" this way, then it requires a
number of communication rounds equal to half the difference of the
initial fees they named.
This fee negotiation is also only possible if there are exactly 2
participants, and there is no easy way to extend it to 3 or more
participants, as would be the case with a sidepool.

Thus, this SIDEPOOL specification defines a novel method of
negotiating fees.
Briefly:

* All participants first present a precommitment to a particular
  feerate they believe is reasonable.
  * They use a salted hash as commitment, and also sign the hash
    to prove that they, as a participant, contributed that
    particular feerate estimate.
* The pool leader gathers all commitments and signatures, and
  broadcasts the full set to all pool followers.
* All participants validate the signatures, and that all the
  participants contributed their estimates, then reveal their
  feerate and salt.
* The pool leader gathers all commitment revelations and
  broadcasts all revelations to all pool followers.
* All participants then validate that the revelations are the
  ones committed to, and sort the feerates.
  All participants then agree to get the median feerate as the
  negotiated feerate.

The above method has the following advantages:

* It supports any number of participants.
* It requires only two communication rounds:
  * Provide commitment.
  * Open commitment.
* Precommitments prevent late responders from having a strategic
  advantage (i.e. by selecting a fee based on what faster
  responders have selected).
  * For example, the Lightning Network mutual close ritual can
    be gamed by the second responder.
    The second responder can have a target fee, and after the
    first responder names a fee, the second responder can
    choose a fee such that the average of the first responder
    named fee and the fee the second responder selects is the
    target fee, which is now likely to be settled on in a
    normal run of the negotiation.
* Outliers that have unusual or misconfigured mempools are
  unlikely to negatively affect the resulting feerate.

Nevertheless, the method described above still has a flaw:

* Sockpuppets of a single entity, if they are running on the
  same sidepool, have a much higher effect on the estimate.
  If at least half of the participants are sockpuppets, then
  the sockpuppets can set any value as median.

To protect against the above, we instead use a *weighted median*.
In a weighted median, each sample has a weight that is associated
with them.
We take the sum total weight of all samples, and after sorting
them, we iterate starting from the first (lowest) sample.
We accumulate weights, and once an entry causes the accumulated
weight to equal or exceed half the total weight, that entry is
selected as the weighted median.
If all samples had the same non-zero weight, then the weighted
median is the same as the median.

The weight used is the size of the funds that a participant has
direct control of in the sidepool, effectively weighting the
feerates according to their stake in the sidepool.

By using a weighted median, sockpuppets need to be funded, which
increases the barrier against the above attack.

Fee Computation For Splice
--------------------------

The Reseat transaction is a single transaction that includes
splice-ins and splice-outs.
As such, the Reseat transaction has a single onchain transaction
fee ratek065l.

We separate the onchain fees paid by in-sidepool funds, from the
fees paid by participants that splice-in and splice-out.

In-sidepool funds pay for:

* Common parts of the transaction.
* The transaction input that spends the current funding
  transaction output.
* The new funding transaction output.

The fees for the above parts are divided among all the outputs
in the output state at the end of the preceding Contraction
Phase.
This division may result in a remainder, which is an overpayment
of the fees.

In contrast:

* For a splice-in, the fee cost of that input (`prevout`,
  `scriptSig`, and `witness`) is deducted from the input amount
  before it is instantiated as a new output in the output state.
* For a splice-out, the fee cost of that output (`scriptPubKey`
  and `amount`) is deducted from the nominal amount before it is
  instantiated as an output amount of the splice-out.

Splice-in Holding Address
=========================

In sidepool version 1 pools, funds for splicing in are first
placed into a holding UTXO.
The address for that holding UTXO is a Taproot address:

* The internal public key is the aggregate pool public key as
  described in [SIDEPOOL-02 Intermediate Output Addresses][].
* A single Tapleaf:
  * Refund branch:
    Version 0xC0 SCRIPT
    `<relative 1008 blocks> OP_CHECKSEQUENCEVERIFY OP_DROP <X-only pubkey> OP_CHECKSIG`

[SIDEPOOL-02 Intermediate Output Addresses]: ./02-transactions.md#intermediate-output-addresses

The fund can then be spliced-in, if:

* The owner of the fund is a participant of the pool, and
  provides the `<X-only pubkey>` above to the pool leader
  (which broadcasts it to the pool followers on the next swap
  party on announcement of a Rseeat) via standard SIDEPOOL
  messages.
* The fund is still unspent.
* The confirmation depth of the transaction holding the fund
  is strictly `6 <= depth <= 576`.
* The current aggregate pool public key of the sidepool matches
  the one in the splice-in holding address.
  * There is an edge case where the fund reaches the minimum 6
    depth confirmations just *after* a Reseat has occurred.
    As it has not reached depth 6 yet the fund cannot be added to
    that Reseat, and the Reseat will rotate the aggregate public
    key, thus failing this condition on the next swap party.

(TODO)

Random Transaction Input And Output Ordering
============================================

Splice transactions have their inputs and outputs
deterministically shuffled in a multiparty computation.

> **Rationale** Ideally, all wallets should implement
> [BIP-0069][], deterministic lexicographical indexing of
> transaction inputs and outputs.
>
> [BIP-0069]: https://github.com/bitcoin/bips/blob/737879123512f47506de996625615da533d390fb/bip-0069.mediawiki
>
> Sadly, few wallets support [BIP-0069][] and most wallets instead
> shuffle the order of transaction inputs and outputs, as that is
> actually more private (has a larger anonymity set) in practice
> than BIP-0069.
>
> Thus, multiparticipant protocols that need to build
> multiple-input and multiple-output transactions must, by
> necessity, include a complicated multiparty transaction input
> and output shuffling sub-protocol.
> Sidepools are no exception to this.
>
> As sidepools are not intended to be used publicly, and thus have
> no advantage in being known by the rest of the network, it is
> best to reduce the amount of knowledge that
> non-sidepool-participants learn about the sidepool, thus privacy
> is a design goal as well; non-sidepool-participants should find
> it difficult to determine if some transaction is a sidepool
> splice transaction or not.

All sidepool participants need to know the entire shuffled deck,
thus, many of the problems with multiparty shuffling (which
requires that only those issued with a particular card in the
shuffled deck learn the card and can later cryptographically prove
that a particular card was issued to them) are not present.
This multiparty shuffling protocol is therefore not appropriate
for card games between cryptographers, as all participants learn
the ordering of the deck, and MUST NOT be used as such.

The PRNG seed flow is as follows:

* Each participant picks a 256-bit number uniformly at random.
  Call this "*per-participant shuffling seed*"
* Each participant hashes their per-participant shuffling seed,
  then all the pool followers send it to the pool leader, who
  broadcasts all the per-participant shuffling seed hashes to all
  pool followers.
  Each pool follower validates that their hash exists in the
  broadcast.
* Once each pool participant has received all the hashes, pool
  followers send the per-participant shuffling seed (i.e. the
  preimage to the hash) to the pool leader, which validates the
  seeds.
  The pool leader then broadcasts all the per-participant
  shuffling seeds to all pool followers.
* All the pool followers validate that each per-participant
  shuffling seed matches (i.e. the preimage to the hash).
* All pool participants XOR each of the per-participant shuffling
  seeds together to form the "*total shuffling seed*".

The actual shuffling is performed using Fisher-Yates shuffle with
the total shuffling seed used as the seed to a repeated tagged
hash as a CSPRNG.
Inputs are shuffled first, then outputs.

* Sort the transaction inputs and outputs using [BIP-0069][].
* Set the current CSPRNG state (a 256-bit number) to the
  total shuffling seed.
* For `i` = `num_inputs - 1` to `1`:
  * Get the tagged 256-bit SHA-2 hash of the current CSPRNG state,
    with tag `"sidepool version 1 shuffle"` not including the
    double quotation marks and not including any terminating NUL
    characters, and replace the state with the hash.
  * Treat the first 8 bytes of the state as a *little-endian*
    64-bit unsigned integer.
  * Let `j` be the above number modulo `i + 1`.
  * If `i` and `j` are different, swap the transaction inputs
    numbered `i` and `j` (0-indexed).
* For `i` = `num_outputs - 1` to `1`:
  * Get the tagged 256-bit SHA-2 hash of the current CSPRNG state,
    with tag `"sidepool version 1 shuffle"` not including the
    double quotation marks and not including any terminating NUL
    characters, and replace the state with the hash.
  * Treat the first 8 bytes of the state as a *little-endian*
    64-bit unsigned integer.
  * Let `j` be the above number modulo `i + 1`.
  * If `i` and `j` are different, swap the transaction outputs
    numbered `i` and `j` (0-indexed).

> **Rationale** While the rest of the protocol uses big-endian
> numbers, this is because the standard "network ordering" is
> big-endian.
> Modern CPUs are predominantly little-endian, thus the 64-bit
> random number is little-endian.
>
> The above includes a small bias due to the naive use of
> modulo, i.e. modulo bias.
> However, it is expected that using a 64-bit number should, in
> practice, have a negligible modulo bias for most practical
> numbers of inputs and outputs, and 64-bit modulo is widely
> implemented in most languages and CPUs.
> Debiasing modulo may gretly increase the complexity and
> practical runtime of the shuffling algorithm, thus this is
> considered an acceptable trade-off.
>
> The use of a repeated hash function instead of a true
> deterministic cryptographically-secure random number generator
> is generally considered bad design.
> However, this only holds if an adversary can meaningfully
> control the seed, and the use of precommitment hashes for each
> per-participant shuffling seed prevents that.
> As sidepool participants are expected to use HTLCs, and HTLCs
> in Bitcoin predominantly use SHA-256, sidepool implementations
> are already expected to have a SHA-256 implementation, from
> which a tagged hash implementation can trivially be built.
> This reduces the effort of implementing sidepools, compared to
> requiring a particular CSPRNG algorithm.
>
> The shuffling algorithm must be described in detail here (and
>  reimplemented in each implementation of the sidepool protocols)
> as various standard libraries across various language
> implementations, even if they implement a Fisher-Yates or
> equivalent-quality shuffle with low or no modulo bias, are
> not assured to be identical to each other and are frequently
> not assured of keeping the same deterministic algorithm forever.
> Yes, sometimes NIH is necessary.
