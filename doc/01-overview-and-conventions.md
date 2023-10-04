SIDEPOOL-01 Overview And Conventions

Introduction
============

A *sidepool* is an offchain liquidity-management mechanism for
the Lightning Network.
Multiple routing nodes with high uptime cooperatively manage a
shared fund, which backs the entire sidepool.
Each routing node may have some of their own funds in this
shared fund, and use those funds to set the liquidity of their
channels with other routing nodes that are also participants in
the same sidepool.

Why?
----

A sidepool has the following differences from a Lightning Network
channel:

* It has 3 or more participants, preferably a dozen or more.
* It does not support high volume of transfers, due to having to
  coordinate among more participants.

The advantage of putting funds into a sidepool, rather than to a
Lightning Network channel with one of the other participants, is
that funds in a sidepool can be used, at low fees, to directly
pay other participants in the same sidepool.
A Lightning Network channel can only directly pay to the single
counterparty you have that channel with.
Thus, sidepools trade off speed and volume for liquidity
flexibility.

The intent of sidepools is to allow management of channel
liquidity by swapping funds between the sidepool and channels
with imbalanced liquidity.

With sidepools, imbalanced channels can be automatically
corrected periodically.

Due to the *much* lower transfer volume of sidepools (see the
transfer cadence recommended in [SIDEPOOL-04][] (TODO: section)),
Lightning Network channels need to be just big enough to handle
occassional spikes of usage.
Then the rest of channel capacity can be spliced out or
transferred to a sidepool with more participants.
If a node does this with multiple channels with multiple peers,
it gains greater flexibility without increasing the failure rate,
letting it effectively deploy more funds to the Lightning Network
while being less reliant on guessing which specific nodes are
likely to be common destinations in the future.

Sidepools are an alternative to the use of faster sidechains, as
follows:

* Practically all Bitcoin-using sidechains are federated
  sidechains, which are technically k-of-n custodians.
  Sidepools are non-custodial, extending the same argument that
  a 2-of-2 Lightning Network channel is non-custodial to n-of-n
  sidepools.
* Even if the sidechain is faster than Bitcoin blockchain, to
  reduce counterparty risk you want to periodically move funds from
  the sidechain to the Bitcoin blockchain anyway.
  You might as well schedule rebalancings of your channels to a
  slower cadence, to allow your channels to move away from
  perfectly balanced and then reset them to balanced, and it is
  not necesary to use a *faster* blockchain for that.

Sidepools are an alternative to the use of full-fledged CoinPools:

* Sidepools can be implemented with no changes to Bitcoin beyond
  Taproot.
  CoinPools and similar would require base layer changes.

Sidepools are an alternative to the use of channel factories:

* Channel factories *can* be implemented with current
  Taproot-enabled Bitcoin.
  However, merely riding high-volume high-speed channels on top of
  a slow Decker-Wattenhofer (which is what sidepools use) is
  already a significant software complexity increase.
  By putting the liquidity management "to the side" of existing
  Lightning Network chanenls instead of "around" them:
  * There is reduced complexity, as there is no need to implement
    channels-within-factories.
  * Existing channels can be used with sidepools; payers do not
    need to be updated to learn about channels inside channel
    factories, and existing channels can continue to remain open
    and retaining any scoring that external scorers may have
    assigned to those channels, while still allowing liquidity
    management.

Pool Leaders And Followers
==========================

Although all pool participants are "peers" in the sense that
they all equally control the sidepool, to simplify communication,
there is a "*pool leader*" that all the other participants
connect to, and communicate with, in order to manage the pool.
The other non-leader participants are called "*pool followers*".

* The pool leader is not a custodian: it cannot transfer funds
  without the assent of all of the pool followers.
  * Sidepools require N-of-N participation, i.e. a consensus.
    If even one participant is offline, then the sidepool cannot
    transfer funds inside it.
* The pool leader is required to always have a certain minimum
  amount it controls unilaterally in the sidepool.
  See [SIDEPOOL-02][] for more information.
* The pool leader is itself another participant of the sidepool.
  Thus, in higher-level protocols built on top of sidepools, the
  pool leader may take on one of the protocol participant roles.
  While the protocol descriptions may mention sending messages
  between the protocol participant and the pool leader, the pool
  leader need not actually connect to itself and send those
  messages; the only requirement is that the code connected to
  the handling of those messages is triggered at the appropriate
  point in time.
  * Implementations may use an "internal" message queue, and
    split their code such that the "pool leader code" is
    separated from the "higher-level protocol code", and use
    internal messaging if the higher-level protocol code is
    running on the pool leader, or use actual messages if the
    higher-level protocol code is running on a pool follower.

[SIDEPOOL-02]: ./02-transactions.md

While *pool-level* operations follow the leader-and-followers
model for protocol communications, SIDEPOOL-defined protocols
built *on top of* sidepools do not follow the
leader-and-followers model.
Instead, higher-level protocols communicate between the
participants directly.

This is because higher-level protocols defined in the SIDEPOOL
specifications involves only two participants.
For example, SIDEPOOL-NNN (TODO) specifies how a sidepool
participant can use its in-pool funds to get more in-channel
funds on a channel with another sidepool participant.
Only those two participants need to talk to each other to handle
the transfer of funds in their Lightning Network channel, so it
is pointless to have them communicate via the pool leader.
However, sidepool operations require all participants, whether
pool leader or pool follower, to contribute partial signatures,
thus the handling of in-pool funds is done via the pool leader
doing signature aggregation and other tasks.

Sidepool Messaging Protocol
===========================

Sidepools are intended to be used by Lightning Network nodes, so
that they can manage their liquidity using the sidepool.

As such, sidepool protocol messages are sent using the Lightning
Network [BOLT 8][] specification.
That is, sidepool participants MUST be Lightning Network nodes,
and communicate with each other using the normal Lightning
Network communications protocol.

[BOLT 8]: https://github.com/lightning/bolts/blob/master/08-transport.md

Each BOLT 8 message is composed of two bytes of a 16-bit
big-endian message ID.
The rest of the message is then the message payload.

Sidepool messages have a message ID of 53609
(`sidepool_message_id`) in the BOLT 8 message.

> **Rationale** 53609 is odd and above 32768, as CLN requires
> custom message handlers to handle odd message IDs, while LDK
> and LND require them to handle message IDs above 32768.
> It is otherwise selected by random and has no special meaning.

The next two bytes after the message ID, equivalently the first
two bytes of the payload, are then the sidepool message sub-ID,
as a big-endian 16-bit number.

In all other SIDEPOOL documents, and in the rest of this
document, message IDs are actually sidepool message sub-IDs.

The rest of the payload after the sidepool message sub-ID is
composed of TLVs, as described in
[BOLT 1 Type-Length-Value Format][].
Types and lengths are encoded in [BigSize Integer Encoding][].

A difference here is that types below 2^16 (65536) are *not*
reserved for BOLT specifications, as the TLVs described in the
SIDEPOOL specifications are never put on standard BOLT messages,
only on SIDEPOOL-specific messages with `sidepool_message_id`
(53609) messages.

[BOLT 1 Type-Length-Value Format]: https://github.com/lightning/bolts/blob/master/01-messaging.md#type-length-value-format
[BigSize Integer Encoding]: https://github.com/lnbook/lnbook/blob/develop/13_wire_protocol.asciidoc#bigsize-integer-encoding

### Conventions

The types in TLVs are associated with specific sidepool message
IDs.
Given a particular `message_id`, TLV types from
`message_id * 100` to `message_id * 100 + 99`, inclusive, are
reserved for that message.

For example, if a sidepool message ID is 42, then the TLVs for it
are numbered from 4200 to 4299, inclusive.

The ["it's ok to be odd"][] rule is followed; sidepool
implementations may ignore unrecognized odd message IDs or TLV
types, but in case of even message IDs or TLV types, should raise
some kind of warning or alarm to the human operator.

["it's ok to be odd"]: https://github.com/lightning/bolts/blob/master/00-introduction.md#its-ok-to-be-odd

Sidepool Feature Bit
====================

Nodes that support the SIDEPOOL protocol MUST set feature bit
1263 (`sidepool_bolt_feature_bit`) in their `init` messages and in
their `node_annoucement`s.

Nodes that support the SIDEPOOL protocol MUST NOT send
`sidepool_message_id` (53609) messages if the counterparty does
not signal support via `sidepool_feature_bit` (1263) in their
`init` message.

SIDEPOOL-protocol Feature Set
=============================

The SIDEPOOL protocol also includes a "feature bits" concept
similar to that specified in the Lightning Network BOLT
specifications.

The SIDEPOOL feature bits are a separate set from any of the
Lightning Network BOLT feature bits.

The "it's okay to be odd" rule applies to SIDEPOOL feature bits in
the following manner:

* If the counterparty has an even feature bit that you do not
  recognize, you MUST NOT send any `sidepool_message_id` messages
  to that counterparty.
  * Otherwise, you may send any `sidepool_message_id` messages to
    the counterparty, provided the sub-messages are allowed by the
    set of features the counterparty indicates.

Participants MUST always support The "basic set" of protocols,
described in [SIDEPOOL-01][], [SIDEPOOL-03][], [SIDEPOOL-04][],
and [SIDEPOOL-05][].
The above protocols MAY be used without any feature bits being set.
However, participants MUST NOT send any other messages other than
`my_sidepool_features` below until they have received a
`my_sidepool_features` message from the peer.

[SIDEPOOL-01]: ./01-overview-and-conventions.md
[SIDEPOOL-03]: ./03-setup-and-teardown.md
[SIDEPOOL-04]: ./04-swap-party.md
[SIDEPOOL-05]: ./05-reseat-splice.md

In addition, the encoding of SIDEPOOL feature bits is
significantly different.
Briefly, it is the varsize-encoded run-length-encoding of an
array of bits.

This array of bits is a variable-length array whose individual bits
can be indexed.
For example, suppose a feature set has bits 1, 2, and 42 set.
The array could be represented as:

```JSON
[ 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1
]
```

The encoding is done this way:

* Limit the array of bits to the highest non-zero bit.
* Start at the array start.
* Count the number of 0s, then emit it as a `varsize`:
  * If the value is `< 0xFD`, emit it as a single byte.
  * If the value is `0xFD <= x <= 0xFFFF`, emit `0xFD` and then a
    16-bit unsigned integer in big-endian order.
  * If the value is `0x10000 <= x <= 0xFFFFFFFF`, emit `0xFE` and
    then a 32-bit unsigned integer in big-endian order.
  * Otherwise, emit `0xFF` and then a 64-bit unsigned integer in
    big-endian order,
  * (This is the same as the encoding of Type and Length in TLVs)
* Skip over the 1.
  If we are now at the end of the array (i.e. the highest set bit),
  stop, otherwise repeat the above step.

So a feature set with feature bits 1, 2, and 42 will be encoded as
the following hex dump:

```
0x01 0x00 0x27
```

Which is interpreted this way:

* `0x01`: there is a 1 x 0-bits, then a 1-bit.
* `0x00`: there are no 0-bits, then a 1-bit.
* `0x27`: there are 39 x 0-bits, then a 1-bit.
  (`0x27 == 39`)
* Resulting in the original array:

```json
// 0x01: (0, 1)
// 0x00: (1)
// 0x27: (0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 1)
// [ 0, 1,
//   1,
//   0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 1
// ]
[ 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1
]
```

If the set of feature bits is empty, then the encoding is a 0-length
sequence of bytes, i.e. the encoding is also empty.

To decode:

* Clear the array of bits.
* Read a `varsize` from the encoding.
  Emit that number of 0s, then emit a 1.
* If we are at the end of the encoding, stop, otherwise repeat the
  above step.

At connection establishment of the Lightning Network connection:

* Check if the BOLT-1 `init` message from the counterparty has the
  `sidepool_bolt_feature_bit` set, OR the counterparty has this
  feature bit set in its `node_announcement`.
  If so:
  * Send the `my_sidepool_features` message to the counterparty.

The `my_sidepool_features` message encodes the SIDEPOOL feature
set, as described above:

1.  `my_sidepool_features` (101)
    - Sent by SIDEPOOL-aware participants at connection
      establishment.
2.  TLVs:
    * `my_sidepool_features_set` (10100)
      - Length: Variable.
      - Value: The encoded feature set, as described above.
      - Required.

MuSig2
======

Sidepool participants use a "true multisignature", where a single
public key represents all of the sidepool participants.

Sidepool version 1 uses MuSig2, as described in [BIP-327][].

[BIP-327]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki

Sidepool participants need to generate n-of-n aggregate public
keys in various contexts.
The aggregation of public keys is described in [BIP-327 Key
Generation and Aggregation][].
In some contexts, there is a need to aggregate private keys;
while not described in detail, operations homomorphic to public
key (SECP256K1 curve points) operations can be performed in the
private key (scalars of the SECP256K1 group order).

[BIP-327 Key Generation and Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Key_Generation_and_Aggregation

Non-Persistent MuSig2
---------------------

In sidepool implementations, MuSig2 signing sessions MUST be
*non-persistent*.
This simply means that the secret used to generate nonces for a
MuSig2 signing session is stored only in memory, and not saved in
persistent storage.

During a MuSig2 signing session, participants need to generate
and retain a structure, called `secnonce` in [BIP-327 Nonce
Generation][].
Participants first need to generate `secnonce`, then derive a
structure called `pubnonce`, the latter of which is then sent to
the pool leader, which aggregates the `pubnonce`s into an
`aggnonce`, as per [BIP-327 Nonce Aggregation][].
The pool leader then broadcasts the `aggnonce` to the pool
followers, which then need the `secnonce` to generate their
partial signatures together with the `aggnonce`, as per
[BIP-327 Signing][].

[BIP-327 Nonce Generation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Nonce_Generation-2
[BIP-327 Nonce Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Nonce_Aggregation
[BIP-327 Signing]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Signing

Pool participants MUST NOT store the `secnonce`, or equivalent
structure, in persistent storage.
Pool participants SHOULD store these structures in non-swappable
memory.
Production-quality sidepool implementations MUST store `secnonce`
in non-swappable memory.

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
> session, implying that MuSig2 signing rounds are stored in
> persistent storage, i.e. persistent MuSig2.
> This makes more sense for Lightning Network channels in order
> to reduce latency and thereby increase actions-per-second.
> In the case of sidepools, updates to the sidepool are relatively
> much rarer than in the Lightning Network, and extra latency is
> negligible in comparison.

Pool participants MUST persistently store only the completed
MuSig2 aggregate signature after it has locally validated that
the aggregate signatures are valid.

Once a pool participant has generated its partial signature, the
pool participant MUST delete the `secnonce`-equivalent before
sending the partial signature to the pool leader (or for the pool
leader, before it adds its partial signature to the aggregate
signature):

* The memory storing it SHOULD be securely zeroed out or otherwise
  securely overwritten in a way that is not optimized out by
  compilers or other language interpretation tools.
  * Production-quality implementations MUST securely zero out or
    otherwise securely overwrite the memory.
* The memory MUST be freed, or in a memory-managed environment,
  all references to it dropped.

> **Rationale** Deleting the `secnonce` as soon as the partial
> signature is generated reduces the scope in which bugs or
> other unexpected behavior could cause the software to sign
> different messages with the same nonce.

A further implication is that in case of a crash before the
partial signature can be computed, the software cannot recover the
`secnonce`, which is required to generate a partial signature (and
from there, the aggregate signature).
This implies that in case of a crash during a sidepool update, the
only recovery is to abort the entire sidepool (i.e. drop it
onchain unilaterally).

> **Rationale** A sidepool abort due to a crash during a state
> update is expected to be rare due to state updates being rare.
> More complex crash recovery is possible, but these increase the
> risk of private key exfiltration, as noted above.
