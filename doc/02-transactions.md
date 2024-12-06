SIDEPOOL-02 Transactions

Overview
========

Sidepool version 1 pools use the [Decker-Wattenhofer][]
decrementing-`nSequence` mechanism.

[Decker-Wattenhofer]: https://tik-db.ee.ethz.ch/file/716b955c130e6c703fac336ea17b1670/

A pool is a fund, which backs a current set of transaction
outputs.
This set of transaction outputs are not published onchain
immediately, but are kept offchain.
The set of transaction outputs inside the pool can be changed
dynamically, according to particular rules, without any data
needing to be published onchain.

The most recent set of transaction outputs is called the
"*current state*" of the pool.

The current state can be updated twice, in succession, during
sessions known as "*swap parties*".
For standard deployments, swap parties are scheduled by the pool
leader to occur once per day, and start between 0000h to 0030h
UTC; both updates of a swap party occur within 20 minutes of each
other.

(The *pool leader* is the Lightning Network node that
initiated the creation of a sidepool instance, and is
the one that emulates broadcast messages and handles
requests.
The *pool followers* are other Lightning Network nodes
that were invited by the pool leader to the sidepool
instance, and accepted the invitation.)

While operating, version 1 pools have the following transaction
outputs and transactions:

* A *funding transaction*, one of whose outputs is the *funding
  transaction output* for the pool.
  * The funding transaction must be confirmed onchain to a
    certain minimum depth before the sidepool is considered
    operational.
    All other transactions are offchain.
  * The funding transaction output has an aggregate key of all
    participants.
* A *kickoff transaction*, which spends the above funding
  transaction output, and two outputs, (1) an aggregate key
  of all participants with the same amount as its sole input,
  and (2) a P2A output of 240 satoshis (which is always
  the last output).
  * The kickoff transaction has `nVersion=3`, which indicates
    Topologically Restricted Until Confirmation, and implies
    replacability.
* Multiple *update transactions*.
  * They all have a single input:
    * The First update transaction spends from the aggregate-key
      output of the kickoff transaction.
    * All but the first update transaction spends from the
      aggregate-key output of the previous update transaction.
  * For outputs:
    * All but the last update transaction has a two outputs,
      (1) an aggregate key of all participants and (2) a P2A
      output of 240 satoshis (which is always the last
      output).
    * The last update transaction has as outputs all of the
      current set of transaction outputs inside the pool,
      plus a P2A output of 240 satoshis (which is
      always the last output).
  * Update transactions have an `nSequence` encoding a relative
    lock time, from 0 blocks, to
    `blocks_per_step * (steps_per_stage - 1)`.
    Only multiples of `blocks_per_step` are allowed for their
    `nSequence`.
  * Update transactions have `nVersion=3`, which indicates
    Topologically Restricted Until Confirmation, and implies
    replacability.
  * The number of update transactions is `number_of_stages`.

For version 1 pools:

* `steps_per_stage` is 4, except for the last update transaction,
  where it is 2.
* `blocks_per_step` is 144.
* `number_of_stages` is 6.

Output States
-------------

An output state is a bag of tuples of Taproot X-only public key
(i.e. a Taproot SegWitv1 address) and amount.
Each tuple represents an output that is currently stored inside
the pool.

As a bag, it is possible for multiple outputs to have the same
address and amount.
It is expected that protocols built on top of sidepools will
normally generate Taproot addresses that are unique with high
probability if the participants are honest, but at the sidepool
level duplicates are allowed.

> **Rationale** Participants within the same sidepool may attempt
> to disrupt the entire sidepool by duplicating an output.
> If this specification did not indicate what happens in that
> case, differring implementations would handle the duplicated
> output in different ways, including in ways that could cause one
> or the other implementation to lose funds.
>
> If two different participants want to create the exact same
> output at the same time, there is no easy way to determine which
> one honestly created their output and which one copied it.
> New outputs are expected to be created with shared control
> (e.g. the new output being created is an HTLC), and if one
> participant funds that output, the other participant with
> shared control can attempt to disrupt their protocol in ways
> that trigger bugs in the other participant by duplicating the
> same output.
> Due to the shared control, there is no way to prove which
> participant was honest and which one was copying.
>
> Thus, this specification allows duplicates, and implementations
> need to be robust against duplicates of the outputs.

Implementations of higher-level protocols that need an output to
be instantiated in the sidepool MUST ignore duplicates of the
output they are expecting, and treat the output as having been
created if at least one copy of it exists in the output state that
is created in the sidepool.

The Last update transaction has a set of outputs that represents
the current output state.

The outputs are ordered according to the lexical ordering of
the 32-byte representation of the Taproot X-only public key.
In case of duplicate addresses, ties are broken by ordering the
output with a smaller amount first.
In case of duplicate address and amount, all of the transaction
outputs are instantiated in sequence.

Output amounts MUST be greater than or equal to 10,000 satoshis.
This is 0.0001 BTC, 0.1mBTC, 100 uBTC, 100 bits, and
10,000,000 millisatoshis;
it is called `sidepool_version_1_min_amount`.

There must always be at least one output.
The pool leader must always have an output, with nominal amount
greater than or equal to the above limit, that it unilaterally
controls.
In the worst case (such as just after setup of a new sidepool
where the pool followers end up not contributing any funds), only
the pool leader has a single transaction output it controls in
the pool.

At the sidepool level, only Schnorr signatures to spend
transaction outputs of the current output state are accepted.
Protocols built on top of sidepools are expected to allow a
single Schnorr signature to be generated, such as
(**non-normative**) by completing a MuSig2 session within the
higher-level protocol, or by handing over actual private keys
that were generated ephemerally for a single run of the
higher-level protocol.
See [SIDEPOOL-06 HTLCs In Sidepools][] for an example.

[SIDEPOOL-06 HTLCs In Sidepools]: ./06-htlcs.md

> **Rationale** For protocols built on top of sidepools, a
> protocol timeout would imply that the sidepool itself should
> abort --- the expectation is that a dishonest or buggy
> participant would not correctly handle the new state in the
> sidepool anyway, thus, abort paths would need to be enforced
> onchain.
>
> Taproot allows such abort paths to be added as separate
> tapleaves, and various other ways of backing out of the
> protocol may also be done, such as by pre-signing timelocked
> transactions (with the timelock effectively being a timeout
> that returns funds in a protocol abort).
> The blockchain can thus implement the full consensus code
> including SCRIPT interpreter, while sidepool code only needs
> to care about Schnorr signatures for the happy case.

Output states MUST:

* Have a minimum of 1 output.
* Have a maximum of 400 outputs
  (`sidepool_version_1_max_outputs = 400`).
* Have at least one output that is unilaterally controlled by the
  pool leader.

Output states, when serialized, are sorted in a canonical
ordering:

* The Taproot X-only Pubkeys are serialized in their 32-byte
  format as per [BIP-340 Specification][].
* The amounts are serialized as big-endian 64-bit unsigned
  numbers, in satoshi, concatenated after the serialization of
  the Taproot X-only Pubkeys.
  The serialization of a single output is thus 40 bytes.
* The serialized outputs are then sorted lexicographically.
  * Equivalently, outputs are sorted by the lexicographic
    comparison of their Taproot X-only Pubkeys, and in case of
    duplicate public keys, are sorted with the smaller amounts
    first.

As noted, the output state is a bag, not a set, and outputs
(which are tuples of Taproot X-only Pubkeys and satoshi amounts)
MAY be duplicated.

[BIP-340 Specification]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Specification

Participants
------------

Sidepool version 1 pools must have a number of participants:

* At least 3 participants
  (`sidepool_version_1_min_participants`).
* At most 28 participants
  (`sidepool_version_1_max_participants`).

> **Rationale** `sidepool_version_1_max_outputs` limits the
> number of channels that can be updated in this sidepool, as
> each updated channel needs to have an HTLC temporarily hosted
> in the sidepool in order to change the balance of that channel.
> For 28 participants, with every participant having a channel
> with every other participant, that is 406 channels, which
> already exceeds the specified `sidepool_version_1_max_outputs`.
>
> A sidepool with only 2 participants has no advantage over a
> 2-participant Lightning Network channel, while being slower
> and more blockchain-inefficient in a unilateral close case,
> thus sidepools must have at least 3 participants for minimum
> utility on top of Lightning Nework channels.

Swap Parties
------------

Swap parties are sessions during which pool participants
may offer an HTLC to other participants, then fulfill,
fail, or retain the HTLC for the next swap party.

There are two kinds of swap parties:

* *Regular swap parties*
  - These are scheduled by the pool leader to occur
    once a day.
  - The pool leader may miss a regular swap party for
    the day.
    - This can occur if it tries to raise all followers
      by connecting to them and trying to send a `ping`
      `pong`, but fails to raise one or more followers.
    - This can also occur if the pool leader is offline
      during the regular period.
  - When a sidepool is initially created, the pool
    leader indicates a time-of-day at which the
    regular swap party is expected to be performed.
    This time-of-day is always relative to the UTC
    timezone.
    Pool members check that regular swap parties can
    only be performed within one hour of this
    promised time.
* *Irregular swap parties*
  - Any pool member can initiate a swap party at any
    time.
  - The pool member (whether leader or follower) must
    have unilaterally-controlled funds in the sidepool,
    and pays a nominal fee from those funds, in order
    to authorize the swap party.

Every swap party is composed of two phases, and an
optional third phase.

### Swap Party Phases

HTLCs, to be resolved, need to have two updates:

* One update to actually offer the HTLC.
* Another update to fulfill or fail the HTLC.

Thus, as mentioned, swap parties have two updates, or "*swap
party phases*":

* *Expansion Phase* - participants specify that an existing
  transaction output in the current output state is split into
  one or more new transaction outputs.
  * For example, a participant offers an HTLC to another
    participant by splitting its in-pool funds between a "change"
    output containing its remaining funds, and a new output that
    has a Taproot output representing an HTLC.
* *Contraction Phase* - participants specify that one or more
  transaction outputs in the current state will be merged into a
  new transaction output.
  * For example, a participant accepts an HTLC by arranging to
    get the offerer ephemeral private key in that HTLC (i.e.
    private key handover), then signing off on the merge from the
    HTLC and its other funds (possibly including other HTLCs).

As described in [SIDEPOOL-04 Swap Party Protocol][], the
participant would send the pool leader a signature in the
Expansion Phase that signs a message describing all the outputs it
wants to create, and in the Contraction Phase would create a
signature for each output it wants to destroy, signing a message
describing the new merged output.

[SIDEPOOL-04 Swap Party Protocol]: ./04-swap-party.md

There is also an optional third phase, the *Reseat Phase*.
The Reseat Phase does not represent an update of the
[Decker-Wattenhofer][] mechanism, but instead represents an
onchain operation, which causes the mechanism to reset.
The full details of the Reseat Phase are described in
[SIDEPOOL-05 Reseat And Splicing][].

[SIDEPOOL-05 Reseat And Splicing]: ./05-reseat-splicing.md

Pool Update Counter
-------------------

The pool has an 11-bit counter.

Each update increments this counter.
For a new pool, or a reseated pool, the pool update counter
starts at binary 00000000001, or decimal 1.

Each update transaction has a relative lock time encoded in its
`nSequence` field.
Each update transaction, except the last update transaction,
takes 2 bits from the counter.
The First update transaction takes the top 2 bits, while the Last
update transaction takes the lowest 1 bit.

The counter is thus divided into 6 bit fields, from highest bit
to lowest bit, with the first 5 update transactions taking 2 bits
and the last update transaction taking the last 1 bit.

| Counter Bits        | 10 9  |  8 7   | 6 5   | 4 3    | 2 1   | 0    |
|---------------------|-------|--------|-------|--------|-------|------|
| Update Transaction  | First | Second | Third | Fourth | Fifth | Last |

The `nSequence` of an update transaction depends on the value
of the bits allocated to it.
For all except the last update transaction:

| Bit Values | Relative Lock Time |
|------------|--------------------|
| 00         | 432 (= 3 * 144)    |
| 01         | 288 (= 2 * 144)    |
| 10         | 144                |
| 11         | 0                  |

For the last update transaction, because it has only the last
bit, the table is:

| Bit Values | Relative Lock Time | Comment                     |
|------------|--------------------|-----------------------------|
| 0          | 144                | Result of Expansion Phase   |
| 1          | 0                  | Result of Contraction Phase |

A swap party involves two updates.
Because the pool update counter starts at 1:

* The first update (Expansion Phase) in the swap party is always
  from an odd counter value (lowest bit 1) to an even counter
  value (lowest bit 0).
  * More than one update transaction will need to be re-created,
    depending on any carry bits from lower bit fields.
    At least the Fifth and Last update transactions need to be
    re-created.
* The second update (Contraction Phase) is always from an even
  counter value (lowest bit 0) to an odd counter value (lowest
  bit 1).
  * Only the last update transaction is re-created, as it is
    impossible to have a carry of 1 when incrementing the lowest
    bit from 0 to 1, and thus only the last update transaction
    is affected in this phase.

As there are two updates per swap party, the 11-bit counter
(maximum 2047) implies a maximum of 1023 swap parties before the
pool needs to perform some onchain operation to reset the pool
update counter back to 1 (i.e. a Reseat Phase becomes mandatory).

Onchain Feerates
----------------

All transactions (the kickoff and each update transaction)
must have `nVersion=3`, meaning they are Topologically Restricted
Until Confirmation.
This also implies that the transactions are replacable and allowed
to have 0 fee.

All transactions must have a P2A output as their last output.
All transactions, except the last update transaction, must
pay 0 fee (i.e. the sum of all output amounts is equal to
the sume of all input amounts).
The last update transaction may or may not pay 0 fee,
as described in a later section.

Every P2A output must have a 240 sat amount, and is always the
last output.
On unilateral closure, at least one participant has to use an
exogenous UTXO to pay for the confirmation of transactions,
hooking into the P2A output to pay fees for it.

The feerate of the funding transaction is negotiated by the pool
participants during the creation of the pool, or during a
Reseat Phase where a new funding transaction for the pool is
created from the current funding transaction output.

### Who Pays For What?

These things must be paid for onchain by the sidepool
mechanism:

* The P2A outputs
* The impact of irregular swap parties
* The impact of regular swap parties

#### Who Pays For P2A Outputs?

As each P2A output is required to be 240 satoshis,
each transaction has its non-P2A outputs sum up to
240 less than what its input amount is.
There are `1 + number_of_stages` transactions, thus,
the total amount from the funding transaction output
to all P2A outputs is `(1 + number_of_stages) * 240`
satoshis.

The total P2A output amount is a contribution of the
pool leader, and is the reason why the pool leader
must always own an output in the current output state
of the pool.

Whenever the sidepool state updates (i.e. in all of
Expansion, Contraction, and Reseat phases), the pool
leader shows a proof that one of the outputs in the
current output state is owned by itself.
The amount `(1 + number_of_stages) * 240` is deducted
from one instance of that output in the output state
bag, before it is written into the Last update
transaction.

Thus, P2A output amounts are paid for by the pool
leader, and that amount is effectively a contribution
by the pool leader to allow feerate decisions to be
deferred to unilateral exit time.

#### Who Pays for Irregular Swap Parties?

Any pool member --- whether pool leader or pool
follower --- may request an irregular swap party at
any time.
For example, if a pool member would have failed a
forwarding request, but it has funds in a sidepool
which it can swap to the failing channel, it can
request an irregular swap party, then successfully
forward the payment.

A pool member (leader or follower) authorizes an
irregular swap party by showing a signature using
an output inside the sidepool that it unilaterally
controls.

Every swap party increments the update counter.
This counter is limited to a maximum of 1023 swap
parties.
Thus, an irregular swap party is a cost on the
sidepool, as it shortens the effective lifespan
of the sidepool.

A pool member that authorizes an irregular swap
party pays a fixed fee, the
`irregular_swap_party_fee_base`.
This is removed during the Expansion phase of the
irregular swap party.

For version 1 pools, the
`irregular_swap_party_fee_base` is 10 satoshis.

The contributed amount is ***not*** paid to any other
pool member.
Instead, all irregular swap party contributions are
put into the Pay-for-Reseat Fund.

If a swap party includes a Reseat phase, after the
pool members have agreed on a feerate for the Reseat
operation, the Pay-for-Reseat Fund pays for the
onchain fee before any funds are deducted from the
outputs in the output state.
If the Pay-for-Reseat Fund can cover the entire onchain
fee for the reseat (specifically, the common
transaction fields, plus the input consuming the
current funding outpoint and the output creating the
new funding outpoint), the reseat will not reduce any
in-sidepool funds.
However, if the Pay-for-Reseat Fund is insufficient,
the onchain fee for the reseat cost is paid by a
shared deduction from all outputs in the current
output state.

In addition, the Pay-for-Reseat Fund is the onchain
fee paid by the Last update transaction, as it is
exactly equal to the difference between the output
state and the input to the Last update transaction.

When a sidepool is first opened, the Pay-for-Reseat
Fund starts at 0 satoshis.

Update Transaction Recreation
-----------------------------

During the Expansion Phase, at least two of the update
transactions need to be recreated, and in the worst case, all
of them are recreated.

To determine which update transactions need to be recreated, we
need to check the current pool update counter (i.e. the counter
value *before* the Expansion Phase start).
Recall that the pool update counter at the start of the Expansion
Phase MUST be odd.

Recall that each update transaction is assigned bit fields of
the pool, as follows:

| Counter Bits        | 10 9  |  8 7   | 6 5   | 4 3    | 2 1   | 0    |
|---------------------|-------|--------|-------|--------|-------|------|
| Update Transaction  | First | Second | Third | Fourth | Fifth | Last |

To know if a particular update transaction (First, Second, etc.)
needs to be recreated, then we should check if all the bits to the
*right* of the corresponding bit field (but not including the bit
field itself) are all 1s.

For example, to determine if the Third update transaction must be
recreated, we should check the current pool update counter if
bits 4, 3, 2, 1, 0 are all 1s, as those bits are to the right of
the bit field of the Third update transaction (bits 5 and 6).
This is because once we add 1 to the counter, a carry of 1 will
propagate all the way to bit 5, causing the Third update
transaction to change.

Since the current pool update counter at the start of the
Expansion Phase must be odd, the lowest bit, bit 0, is always 1,
and the Fifth update transaction is always recreated in the
Expansion Phase.

Here are a few examples:

* `pool_update_counter = 21`:
  * Binary: `00000010101`.
  * Split into bitfields: `00 00 00 10 10 1`.
  * Only the Fifth and Last update transactions need to be
    recreated.
* `pool_update_counter = 39`:
  * Binary: `00000100111`.
  * Split into bitfields: `00 00 01 00 11 1`.
  * The Fourth, Fifth, and Last update transactions need to be
    recreated.
* `pool_update_counter = 1279`:
  * Binary: `10011111111`.
  * Split into bitfields: `10 01 11 11 11 1`.
  * The Second, Third, Fourth, Fifth, and Last update
    transactions need to be recreated.

During the Contraction Phase, only the Last update transaction
needs to be recreated.
Since the previous Expansion Phase starts with an odd counter
value, and it increments the pool update counter on completion,
the Contraction Phase starts with a 0 in the lowest bit position,
and the First through Fifth update transactions do not need to be
recreated.

When recreating the update transactions, the relative time locks
of the recreated update transactions should use the
*next* pool update counter value, i.e. the one that is after
incrementing.

Transaction Structure
=====================

Intermediate Output Addresses
-----------------------------

We skip over the P2A output that exists for the kickoff
and all update transactions; P2A outputs have a fixed
address.

The funding transaction output, the kickoff transaction output,
and the non-Last update transaction outputs are all controlled by
an N-of-N of the participants (i.e. the sidepool leader and all
the sidepool followers).

At pool setup time, the following is negotiated:

* Each participant nominates a "*pool participant public key*"
  for the pool instance.
  This is a persistent public key (preferably derived from some
  root secret that is backed up by the participant) that is used
  in the intermediate output addresses.
* Each participant contributes a random number that is then
  combined to form an identifier called the "*sidepool
  identifier*".

The "*aggregate pool public key*" is then derived by using the
[BIP-327 Key Aggregation][] algorithm on all the pool participant
public keys.

[BIP-327 Key Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Key_Aggregation

The intermediate output addresses are then Taproot X-only
public keys.
Each address has:

* An internal public key equal to the aggregate pool public key.
* A single Tapleaf, containing a version 0xC0 SCRIPT: `OP_RETURN
  <shared secret>`.

The `<shared secret>` above is based on the sidepool identifier.

* The funding transaction output uses the SHA256 of the sidepool
  identifier.
* The kickoff transaction output uses the SHA256 of the above
  (i.e. the SHA256 of the SHA256 of the sidepool identifier).
* The First update transaction output uses the SHA256 of the
  above.
* Each non-First, non-Last update transaction output uses the
  SHA256 of the shared secret of the previous update transaction.

Note that the transaction output `scriptPubKey`s do
**not** change based on the current pool update counter.

The only point of the `OP_RETURN` script is to blind
non-participants who observe the blockchain.
Even if all participants use their node ID as the pool
participant public key (not recommended, but allowed), observers
cannot know the tweak without cooperation of at least one
participant.
The `OP_RETURN` Tapleaf is provably unspendable and has otherwise
no effect.

On a Reseat, pool participants need to contribute fresh pool
participant public keys, but the sidepool identifier does not
change.

Transaction Witness
-------------------

For the kickoff transaction and all update transactions, the
witness always uses the keyspend path, and is simply the aggregate
signature of all participants in the pool (with a tweak
as described above).

When a sidepool mechanism is dropped onchain unilaterally by any
participant (by publishing the kickoff transaction), the outputs
of the Last update transaction will be spendable onchain using
the full blockchain rules.
What happens in that case must be defined by protocols built on
top of the sidepool protocol, and is not defined in this
document.

Last Update Transaction Outputs
-------------------------------

The pool leader needs to provably reveal its output,
as the P2A contribution is deducted from that output.
In addition, the amount in the Pay-for-Reseat Fund
must also be known.

Outputs of the Last update transaction have the same ordering as
the canonical ordering of the output state.
In addition, an extra P2A output of 240 satoshis is the
last output of the Last update transaction.

The first instance of the leader-owned output in the
output state bag has `(1 + number_of_stages) * 240`
satoshis deducted from it before being instantiated
on the Last update transaction.
These deducted funds are diverted to the P2A outputs.

The fee for the Last update transaction is equal to
the current Pay-for-Reseat Fund.

Addendum: Why Decker-Wattenhofer
================================

> **Rationale** This entire section is a rationale for why we
> use the Decker-Wattenhofer decrementing-`nSequence` mechanism.

Sidepol version 1 uses [Decker-Wattenhofer][]
decrementing-`nSequence` mechanisms.

Just to be clear, the linked paper introduces Duplex Micropayment
Channels, which are 2-participant offchain mechanisms.
The duplex mechanism is actually several mechanisms:

* A sequence of decrementing-`nSequence` mechanisms...
* ...terminated by two relative-locktime Spilman channels.

The name "duplex" comes from the fact that the Spilman channels
are unidirectional, and we have two such channels going in
opposite directions, hence duplex.
Sidepool version 1 does *not* use duplex, it uses the
decrementing-`nSequence` mechanisms only.

The decrementing-`nSequence` mechanism has two pros and a major
con:

* Decrementing-`nSequence` Pro:
  * Can be implemented with existing 2025 Bitcoin.
  * Can be used by more than two participants.
* Decrementing-`nSequence` Con:
  * Pushes a lot of data onchain in case of a unilateral close.

Decker-Russell-Osuntokun ("eltoo") reduces the unilateral close
to only two transactions in the economically-rational case.
It can be emulated with 2025 Bitcoin, but would require
an onerous number of parallel signing sessions once the
mechanism has had enough updates.

Law also presents two other mechanisms in [Efficient Factories For
Lightning Channels][].
These mechanisms have the same two pros above (works today,
N > 2), but do *not* have the con of pushing a lot of data
onchain in a unilateral close --- they only require 2 transactions.

[Efficient Factories For Lightning Channels]: https://github.com/JohnLaw2/ln-efficient-factories/blob/main/efficientfactories10.pdf

They have a major con, however:

* TPF / SC Con:
  * Every participant needs to put up a bond, which is slashed
    if they cheat and publish an old state, and which is a
    *separate* onchain fund from the funds inside the mechanism.

The above con makes the mechanism undesirable for sidepools.
One of the advantages of sidepools is that it allows the limited
funds of a Lightning Network participant to be allocated, not to a
single channel to a single peer, but to multiple peers in the same
sidepool.
Thus, the extra bond funds are a capital inefficiency that would
be unnecessary under decrementing-`nSequence` mechanism.

In the worst case, there exists a state where all the sidepool
funds are owned by only one participant.
Then if the most recent valid state has that participant with 0
funds (or close to it relative to the fund size) in the sidepool,
that participant can attempt to publish the old state where they
owned all the sidepool funds.
Thus, in order to make that unpalatable, the bond has to be the
same size as the entire funding of the whole mechanism, and
worse, *each* participant has to put that bond up.

The alternative is to force participants to lock up funds inside
the mechanism, which they can then not use in transactions within
the mechanism, so that the worst-case difference between the
most recent state and some old state cannot exceed some
threshold, and only that worst-case difference is put in the
external bond.
But that is equivalent to putting up a bond of similar size
outside the mechanism that cannot be spent during the lifetime
of the mechanism, and that will end up having the same capital
inefficiency.

The extra bond cannot be used for other purposes.
As such, the existence of this bond represents an opportunity
cost --- the owner could have instead put up the bonds in
Lightning Network channels or JoinMarket maker bots and earned.
As the lifetime of the mechanism increases, the cost of
maintaining the bond increases as well.
Ultimately, the cost of putting additional data onchain is a
satoshi cost that can be compared to the opportunity cost of
maintaining a bond (illiquidity) that cannot earn funds from
Lightning or JoinMarket.
Thus, for a long-lived mechanism that locks up significant funds,
the opportunity cost of the bonds required in TPF and SC
mechanisms will exceed the cost of the additional transactions in
decrementing-`nSequence`.
