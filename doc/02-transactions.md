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
"current state" of the pool.

The current state can be updated twice, in succession, during
sessions known as "*swap parties*".
For standard deployments, swap parties are scheduled by the pool
leader to occur once per day, and start between 0000h to 0030h
UTC; both updates of a swap party occur within 20 minutes of each
other.

While operating, version 1 pools have the following transaction
outputs and transactions:

* A *funding transaction*, one of whose outputs is the *funding
  transaction output*.
  * The funding transaction must be confirmed onchain to a
    certain minimum depth before the sidepool is considered
    operational.
    All other transactions are offchain.
  * The funding transaction output has an aggregate key of all
    participants.
* A *kickoff transaction*, which spends the above funding
  transaction output, and has a single output, an aggregate key
  of all participants.
* Multiple *update transactions*.
  * They all have a single input:
    * The first update transaction spends from the output of the
      kickoff transaction.
    * All but the first update transaction spends from the
      output of the previous update transaction.
  * For outputs:
    * All but the last update transaction has a single output,
      an aggregate key of all participants.
    * The last update transaction has as outputs all of the
      current set of transaction outputs inside the pool.
  * Update transactions may have an `nSequence` encoding a 
    relative lock time, from 0 blocks, to
    `blocks_per_step * (steps_per_stage - 1)`.
    Only multiples of `blocks_per_step` are allowed.
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
See the document on HTLCs inside sidepools for an example
(TODO).

> **Rationale** For protocols built on top of sidepools, a
> protocol abort would imply that the sidepool itself should
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

Output states, when serialized, are sorted in a canonical
ordering:

* The Taproot X-only Pubkeys are serialized in their 32-byte
  format as per [BIP-340 Specification][].
* The amounts are serialized as big-endian 64-bit unsigned
  numbers, in satoshi, concatenated after the serialization of
  the Taproot X-only Pubkeys.
  The serialization of a single output is thus 40 bytes.
* Each serialized output is then sorted lexicographically.
  * Equivalently, outputs are sorted by the lexicographic
    comparison of their Taproot X-only Pubkeys, and in case of
    duplicate public keys, are sorted with the smaller amounts
    first.

As noted, the output state is a bag, not a set, and outputs
(which are tuples of Taproot X-only Pubkeys and satoshi amounts)
MAY be duplicated.

[BIP-340 Specification]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Specification

Swap Party Phases
-----------------

Swap parties are sessions during which pool participants may
offer an HTLC to other participants, then fulfill, fail, or
retain the HTLC for the next swap party.

HTLCs, to be resolved, need to have two updates:

* One update to actually offer the HTLC.
* Another update to fulfill or fail the HTLC.

Thus, as mentioned, swap parties have two updates, or "*swap
party phases*":

* *Expansion Phase* - participants specify that an existing
  transaction output in the state is split into one or more new
  transaction outputs.
  * For example, a participant offers an HTLC to another
    participant by splitting its in-pool funds between a "change"
    output containing its remaining funds, and a new output that
    has a Taproot output representing an HTLC.
* *Contraction Phase* - participants specify that one or more
  transaction outputs in the current state will be merged into a
  new transaction output.
  * For example, a participants accepts an HTLC by arranging to
    get the offerer ephemeral private key in that HTLC (i.e.
    private key handover), then signing off on the merge from the
    HTLC and its other funds (possibly including other HTLCs).

As described in another document (TODO), the participant would
send the pool leader a signature in the Expansion Phase that
signs a message describing all the outputs it wants to create,
and in the Contraction Phase would create a signature for each
output it wants to destroy, signing a message describing the
new merged output.

There is also an optional third phase, the *Reseat Phase*.
The Reseat Phase does not represent an update of the
[Decker-Wattenhofer][] mechanism, but instead represents an
onchain operation, which causes the mechanism to reset.
The full details of the Reseat Phase are described in another
document (TODO).

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

All transactions must be marked with the RBF-enabled flag.
Relative lock times encoded in `nSequence` automatically enable
the RBF flag.
This also means that a relative lock time of 0 must still be
encoded in `nSequence` in order to force the RBF-enabled flag.

The feerate of the funding transaction is negotiated by the pool
participants during the creation of the pool, or during a
Reseat Phase where a new funding transaction for the pool is
created from the current funding transaction output.

The feerate of the kickoff transaction is fixed to the
`minimum_fee_rate + minimum_fee_rate_step`.

The feerate of update transactions, except the Last update
transaction, is a steady increase in feerate.
The lowest (`00`) has `minimum_fee_rate`, and each sucessive
update is higher by `minimum_fee_rate_step`.

    minimum_fee_rate = 280 satoshis per 1000 weight units
    minimum_fee_rate_step = 253 satoshis per 1000 weight units

> **Rationale** The `minimum_fee_rate_step` is from [BIP-125
> Implementation Details][] Rule 4, which indicates that the
> feerate must increase by at least the minimum relay fee.
> The default Bitcoin Core relay fee is 1 satoshi per vbyte,
> equal to 253 satoshis per 1000 weight units.
>
> The `minimum_fee_rate` is slightly higher than the minimum
> relay fee; generally, a 1.1 satoshi per vbyte is often
> enough to get a transaction confirmed onchain, and 280
> satoshis per 1000 weight units is slightly higher than that.

[BIP-125 Implementation Details]: https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki

In a table, the non-Last update transactions thus have the
onchain feerates below:

| Bit Values | Onchain Feerate (satoshis per 1000 WU) |
|------------|----------------------------------------|
| 00         | 280                                    |
| 01         | 533 (= 280 + 253)                      |
| 10         | 786 (= 280 + 253 + 253)                |
| 11         | 1039 (= 280 + 253 + 253 + 253)         |

> **Rationale** Increasing the feerates as the bit values
> increase implies that in a default configuration Bitcoin Core
> mempool, the update transaction stage with the later (higher)
> bit value will replace those with the earlier (lower) bit
> value.
> This reduces, though does not eliminate, the probability of
> older, defunct states to be confirmed; the probability is
> very good that a later state will replace an earlier state,
> and an earlier state will be unable to replace a later state.
>
> An alternative is to use anchor outputs so that all offchain
> transactions are at the minimum relay feerate, and then
> participants must then "bring their own fees" by using CPFP-RBF
> from the anchor outputs.
> However, existing implemented rules for anchor outputs require
> that each participant has its own dedicated anchor output,
> which increases the transaction size drastically if there are
> even a dozen pool participants, further multiplied by the number
> of update transaction stages.
> As CPFP-RBF still relies on RBF anyway, we instead rely
> directly on RBF to avoid the drastic transaction size increase.
>
> Onchain feerates are described in this section in order to
> require that all sidepool implementations follow the same
> policy regarding onchain feerates, as experience with the
> Lightning Network has shown that onchain feerate disagreements
> are a frequent cause of unilateral channel closures.
> In fact, onchain feerate disagreements often arise due to
> sudden changes in onchain feerate (mempools are only weakly
> synchronized), and once channels start dropping due to onchain
> fee disagreement, the onchain feerate estimates start moving
> even faster, exacerbating the problem and causing even more
> channels to close.

> **Non-normative** As noted, this scheme reduces, but ***does
> not eliminate***, the probability that a defunct state is
> confirmed.
> If a unilateral close is triggered and the mempool is
> congested such that the latest state is unable to be
> confirmed within 72 blocks of the previous transaction stage
> getting confirmed, actual implementations MUST sound some
> alarm at the operator, informing them of the stuck transaction
> and imploring them to use out-of-band communications with as
> many miners as possible to pay to accelerate the stuck
> transaction.
> A similar alarm should be triggered, if a reseating transaction
> is unable to confirm within 72 blocks of being fully signed.

For the Last update transaction, if the bit value is 0 (i.e.
even) then the feerate is exactly `minimum_fee_rate`.

However, if the bit value is 1, then the fee (*not the fee
**rate**!*) must be equal to the previous fee at bit value = 0,
plus the current size of the Last update transaction times
the `minimum_fee_rate_step`.
This respects [BIP-125 Implementation Details][] Rule 4.

### Onchain Fee Charge Distribution

In order to pay for the onchain fees, every transaction output
in the current state has to have a small amount removed, the
*onchain fee contribution tax*.

The total amount removed is equal to the total fees of all
offchain transactions, from the kickoff, non-Last update
transactions, and the Last update transaction.

This total is then divided by the number of transaction outputs,
rounded up.
The resulting rounded-up quotient is the onchain fee contribution
tax for that state.
The onchain fee contribution tax is then deducted from each
transaction output.

As the total is unlikely to be divisible by the number of
transaction outputs, there will be an overpayment.
This overpayment MUST NOT be put in the transaction fee of
any of the transactions.
Instead, the pool leader MUST maintain a non-zero amount of
funds it unilaterally controls, and put this overpayment into
the transaction output representing that fund.
Thus, the pool leader may end up having a net positive amount on
its unilaterally-controlled funds.

> **Rationale** This distribution of the onchain fees
> incentivizes participants to only have one transaction output
> for all its unilaterally-controlled funds.
> If a participant splits its unilaterally-controlled funds among
> multiple outputs, it ends up paying more of the total onchain
> fee.
>
> While this reduces privacy, we should note that due to the
> server-client relationship of the pool leader and the pool
> followers, the pool leader already knows which funds are owned
> by which Lightning Network nodes, as messages relating to the
> expansion and contraction of those funds would arise from that
> node anyway.

Thus, a transaction output in the current state has two values:

* The in-current-state "*nominal amount*", which is used in all
  sidepool operations.
  * If an output is not consumed in an Expansion Phase or merged
    in a Contraction Phase, the nominal amount does not change.
* The in-last-transaction "*post-tax amount*", which is the above
  nominal amount minus the onchain fee contribution tax, and for
  the pool leader, plus the onchain fee contribution tax
  overpayment.
  The post-tax amount is the amount that is used in the Last
  update transaction.
  * The post-tax amount may change even if the output is not
    consumed in an Expansion Phase or merged in a Contraction
    Phase.
    This variation depends on how many transaction outputs
    actually would exist after a phase completes.

All serializations of output states, and individual outputs, use
nominal amounts.
The post-tax amount is only used when creating the outputs of the
Last update transaction.

Update Transaction Recreation
-----------------------------

During the Expansion Phase, at least two of the update
transactions need to be recreated, and in the worst case, all
of them are recreated.

To determine which update transactions need to be recreated, we
need to check the current pool update counter (i.e. the counter
value *before* the Expansion Phase completes).
Recall that the pool update counter at the start of the Expansion
Phase MUST be odd.

Recall that each update transaction is assigned bit fields of
the pool, as follows:

| Counter Bits        | 10 9  |  8 7   | 6 5   | 4 3    | 2 1   | 0    |
| Update Transaction  | First | Second | Third | Fourth | Fifth | Last |

To know if a particular update transaction (First, Second, etc.)
needs to be recreated, then we should check if all the bits to the
*right* of the corresponding bit field (but not including the bit
field itself) are all 1s.

For example, to determine if the Third update transaction must be
recreated, we should check if bits 4, 3, 2, 1, 0 are all 1s.
This is because once we add 1 to the counter, a carry of 1 will
propagate all the way to bit 5.

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
and the fee rates of the recreated update transactions should use
the *next* pool update counter value, i.e. the one that is after
incrementing.

Transaction Structure
=====================

Intermediate Output Addresses
-----------------------------

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

Note that the transaction outputs do **not** change based on the
current pool update counter.

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
signature of all participants in the pool.

When a sidepool mechanism is dropped onchain unilaterally by any
participant (by publishing the kickoff transaction), the outputs
of the Last update transaction will be spendable onchain using
the full blockchain rules.
What happens in that case must be defined by protocols built on
top of the sidepool protocol, and is not defined in this
document.

Last Update Transaction Outputs
-------------------------------

The onchain fee contribution tax and the onchain fee overpayment
must be known, and the pool leader needs to also reveal which
output should get the overpayment.

Outputs of the Last update transaction have the same ordering as
the canonical ordering of the output state.

When instantiating each output, the in-transaction amount has the
onchain fee contribution tax deducted.
For the output indicated by the pool leader as the overpayment
beneficiary, the overpayment is added to the in-transaction
amount for that output; note that the nominated beneficiary still
must have its individual onchain fee contribution tax deducted.

    if output_is_overpayment_beneficiary:
        output = output - tax + overpayment
    else:
        output = output - tax
