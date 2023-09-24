SIDEPOOL-01 Overview And Coventions

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
1263 (`sidepool_feature_bit`) in their `init` messages and in
their `node_annoucement`s.

Nodes that support the SIDEPOOL protocol MUST NOT send
`sidepool_message_id` (53609) messages if the counterparty does
not signal support via `sidepool_feature_bit` (1263) in their
`init` message.
