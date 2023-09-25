SIDEPOOL-05 Reseat And Splicing

Overview
========

Sidepool version 1 pools have a limited number of updates, as
specified in [SIDEPOOL-02 Pool Update Counter][].
Once a pool has reached the maximum number of updates, it can
only be closed.

[SIDEPOOL-02 Pool Update Counter]: https://github.com/ZmnSCPxj-jr/sidepool/blob/master/doc/02-transactions.md#pool-update-counter

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
We accumulate weights, and once an entry causes the accumulated to
equal or exceed half the total weight, that entry is selected as
the weighted median.
If all samples had the same weight, then the weighted median is
the same as the median.

The weight used is the size of the funds that a participant has
direct control of in the sidepool, effectively weighting the
feerates according to their stake in the sidepool.

By using a weighted median, sockpuppets need to be funded, which
increases the barrier against the above attack.

The fee negotiation scheme above is done for the following
operations:

* Cooperative close - All participants partake in the fee
  negotiation, and their fee rate samples are weighted according
  to the funds inside the sidepool that they unilaterally control.
  Fees are deducted from each output in the output state.
* Reseat - All participants partake in the fee negotiation, and
  their fee rate samples are weighted according to the funds
  inside the sidepool that they unilaterally control.
  Fees are deducted from each output in the output state.
* Splice - Only participants that splice-in or splice-out
  participate in the fee negotiation, with the fee rate samples
  weighted according to the amount they splice in or splice out.
  Fees are deducted from the respective splice-in and splice-out
  amounts; outputs in the current output state, which are not
  splice out, are not changed.

(TODO)
