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
