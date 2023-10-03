SIDEPOOL-05 HTLCs In Sidepools

Overview
========

Sidepool version 1 pools are intended to primarily support the
offchain creation and resolution of HTLCs.

Hashlocked timelocked contracts (HTLCs) allow for the
(economically-rational) atomic swap of funds from one
cryptocurrency system to another.
Thus, HTLCs are used to manage liquidity between the sidepool
and other cryptocurrency systems, such as:

* Blockchains.
* Lightning Network channels.
* Other sidepools.

As there are multiple uses of HTLCs just within the sidepool
specification, this specification exists as a common reference
for how HTLCs are instantiated inside sidepools.

HTLCs in sidepools have two participants: the HTLC *offerrer*
and the HTLC *acceptor*.
The *offerrer* is the participant that funds the HTLC, and
demands knowledge of the preimage behind the hash attested in the
HTLC.
The *acceptor* is the participant that is paid by the HTLC if it
can reveal the preimage.

HTLCs are created during the Expansion Phase of a pool, and in
the cooperative case, are either fulfilled or failed during the
Contraction Phase of the pool.

Private Key Handover
--------------------

To simplify implementations, sidepools only support spending of
Taproot outputs, and only via the keyspend path.
Thus, within a sidepool, the HTLC itself cannot be directly
enforced; if the other participant does not cooperate, the
sidepool itself must be aborted and dropped onchain.

What participants *can* enforce, however, is to assent that the
HTLCs have been fulfilled or failed honestly by the other party.
In that case, the participants can cooperatively sign off on
keyspends of HTLCs inside the sidepool.

An even better idea than performing an expensive cooperative
multisign (such as MuSig2) comes from the observations below:

* In a typical HTLC, there are two parties with an interest in
  the HTLC: the offerrer and the acceptor.
* Once an HTLC is resolved, its entire amount either goes fully
  to the offerrer, or fully to the acceptor:
  * On failure or timeout, the offerrer gets a refund.
  * On fulfillment (acceptor reveals preimage) the acceptor gets
    the fund.

We can thus have an optimization:

* HTLC offerrers and acceptors can use a one-time-use, ephemeral
  keypair to control their interests in the fund.
* We can have a 2-of-2 of the above two keys, and use it as the
  internal public key of the Taproot output.
* On resolution:
  * On failure or timeout, the acceptor sends its one-time-use
    ephemeral private key to the offerrer, which can now
    reconstruct the aggregate private key.
  * On fulfillment, the offerrer sends its one-time-use ephemeral
    private key to the acceptor, which can now reconstruct the
    aggregate private key.

The above is known as "*private key handover*".
This allows HTLC participants to use the keyspend path (which is
the only spend condition supported by sidepools) without adding
more communication rounds and more complexity and more state.

> **Rationale** Handing over private keys over a communications
> channel is dangerous:
> Intermediate buffers would have the private keys in cleartext
> and might not be cleared by software that handles buffers,
> and modern I/O libraries and I/O OS software would maintain
> caches and buffers all the way from the sidepool software to
> libraries down to the network buffers.
> It may seem strange to be so careful about BIP-327 `secnonce`s
> yet willing to transfer private keys to counterparties.
>
> Against the above, we observe that:
>
> 1.  The private keys being handed over are ephemeral and thus
>     time-bound in their validity.
> 2.  A single private key is insufficient to gain control of the
>     fund, as the fund is controlled by the MuSig combination
>     of two keys.
> 3.  Communications is done over BOLT #8 communications tunnnels,
>     which are encrypted against third parties.
> 4.  Ephemeral keys are still not saved in persistent storage,
>     protecting against naive operators who back up the
>     persistent storage including private keys.
>
> Thus the risk is considered low.
>
> For a peerswap-in-sidepool scheme, there is a clear point
> of whether the sidepool HTLC is owned by one or the other
> participant: if the corresponding return in-channel HTLC
> was created and claimed, then the acceptor of the peerswap
> owns the fund, and if not, the initiator of the peerswap
> owns the fund.
> Thus, there is no case where both participants might send
> their private keys to each other, from which a third party
> that is somehow able to decrypt their communication can
> acquire both keys and synthesize the aggregate private key.

Due to using private key handover, HTLC offerrers and HTLC
acceptors MUST NOT use non-hardened derivation to generate
ephemeral keys for the keyspend path of HTLCs.
HTLC offerrers and HTLC acceptors MAY use hardened derivation
at the last branch of a BIP-32 derivation path, provided that
they ensure that they do not reuse derivation indexes.

> **Rationale** Knowledge of a child private key allows
> computation of a parent private key when using non-hardened
> derivation, provided the attacker can guess the derivation
> index, or can simply brute-force the derivation index (which
> is only 32 bits and trivially brute-forceable with modern
> hardware).
> However, knowledge of a child private key does *not* allow
> computation of a parent private key when using hardened
> derivation.

HTLC Taproot Public Key
=======================

An HTLC inside a sidepool involves four keypairs, two from the
HTLC offerrer, two from the HTLC acceptor:

* Offerrer:
  * Offerrer ephemeral keypair (for private key handover).
  * Offerrer persistent keypair.
* Acceptor:
  * Acceptor ephemeral keypair (for private key handover).
  * Acceptor persistent keypair.

Both offerrer and acceptor MUST store their persistent private
keys, or how to derive them from their root private key, in
persistent storage.

However, offerrer and acceptor MUST NOT store their ephemeral
private keys in persistent storage, and SHOULD store them in
non-swappable memory.
Production-quality implementations MUST store them in
non-swappable memory.

In addition to that, hashlocked timelocked contracts also require
a hash and an absolute block height lock time.
Both offerrer and acceptor MUST store these information in
persistent storage.

During HTLC setup, the offerrer and acceptor exchange the public
keys of their keypairs, and agree on the hash and absolute lock
time.

The HTLC Taproot Public Key is then derived as follows:

* The internal public key is the MuSig combination of the
  offerrer ephemeral public key, and the acceptor ephemeral
  public key, as per [BIP-327 Public Key Aggregation][].
* Generate two 0xC0 Tapleaf SCRIPTs:
  * `OP_HASH160 <RIPEMD160(hash)> OP_EQUALVERIFY <acceptor persistent public key> OP_CHECKSIG`.
  * `<1> OP_CHECKSEQUENCEVERIFY <absolute lock time> OP_CHECKLOCKTIMEVERIFY OP_DROP <offerrer persistent public key> OP_CHECKSIGVERIFY`.
* Combine the above Tapleaf SCRIPTs into a Taproot Merkle Abstract
  Syntax Tree and tweak the internal public key, as per [BIP-341
  Constructing And Spending Taproot Outputs][].

[BIP-327 Public Key Aggregation]: https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki#user-content-Public_Key_Aggregation
[BIP-341 Constructing And Spending Taproot Outputs]: https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki#user-content-Constructing_and_spending_Taproot_outputs

> **Rationale** A relative lock time is imposed on the timelocked
> branch to help support the edge case where the sidepool is
> forced to abort and drop onchain, but there is blockchain
> congestion that causes the Last update transaction of the
> sidepool to be confirmed after the absolute lock time, and the
> acceptor is able to reveal the preimage (and might have already
> paid for that knowledge).
>
> In that case, the hashlock branch has an advantage: it can be
> used to CPFP-RBF the Last update transaction, whereas the
> timelock branch cannot, due to requiring that the Last update
> transaction already be confirmed before that branch can be
> taken.
> This makes the hashlocked branch more likely to confirm, since
> the Last update transaction is likely to be confirmed together
> with the child that reveals the preimage.
>
> This is a probabilistic measure that depends on economic
> rationality of miners *and* on the network being able to
> propagate unconfirmed transactions quickly, but is better than
> not having it.

On deriving the internal public key, both the offerrer and
the acceptor:

* MUST store the internal public key in persistent storage, so
  that it can spend the HTLC via their respective branch in case
  of the "unhappy path" where the HTLC needs to be resolved on
  the blockchain.
* MUST NOT store the ephemeral keypair in persistent storage,
  as it is unnecessary to keep this information beyond the HTLC
  "happy path" where it is resolved quickly.
  * MAY store how to derive the ephemeral key from a root private
    key that is more securely stored, provided the derivation
    path and the ephemeral key cannot be used to synthesize the
    root private key.

The offerrer:

* MUST NOT create the HTLC output in the sidepool (via
  `swap_party_expand_request`) until after it has saved the
  internal public key and other HTLC details into persistent
  storage.

The acceptor:

* MUST NOT send the preimage, or (if this is a swap or an HTLC is
  forwarded) offer to pay to learn te preimage, until after it has
  saved the internal public key and other HTLC details into
  persistent storage.

The details that MUST be stored in persistent storage are:

* Offerrer ephemeral public key.
* Offerrer persistent public key.
  * For the offerrer, how to derive or otherwise acquire its
    persistent private key.
* Acceptor ephemeral public key.
* Acceptor persistent public key.
  * For the acceptor, how to derive or otherwise acquire its
    persistent private key.
* HTLC hash.
* HTLC timeout.
* (optional, can be derived from above) The internal public key,
  i.e. the MuSig aggregation of the offerrer ephemeral public key
  and the acceptor ephemeral public key.

The participants MAY delete the information if one of the
conditions below are true:

* The swap party completes with the HTLC fund (re)claimed.
* The participant has handed over full control of the fund to the
  other participant.
* The sidepool was aborted:
  * The final state on the aborted sidepool does not include
    the HTLC.
  * Or the final state on the aborted sidepool does include the
    HTLC, and the HTLC has been (re)claimed, and the (re)claiming
    transaction has confirmed deeply enough.

Happy Path Spend
----------------

HTLCs in sidepools may be cooperatively resolved in two ways:

* The acceptor reveals the preimage to the offerrer ("*fulfill*").
* The acceptor turns down the offerred HTLC ("*fail*").

In both cases, in the happy path, private key handover of the
ephemeral key is performed:

* Cooperative fulfill:
  * The acceptor reveals the preimage to the offerrer.
  * The offerrer hands over the offerrer ephemeral private key
    to the acceptor.
* Cooperative fail:
  * The acceptor hands over the acceptor ephemeral private key
    to the offerrer.

Once private key handover is done, the funds in the HTLC are
now fully owned by the participant with both of the ephemeral
private keys, and that participant can now aggregate both
private keys in order to unilaterally spend the entire fund
during the Contraction Phase of the pool.

Note that the happy path spend may still suffer from a sidepool
abort, if the abort occurs before the Contraction Phase of the
pool completes.
In that case, the participant that now unilaterally owns the
HTLC fund MAY do either of the following once the Last update
transaction is confirmable:

* Use the above unilateral spend on the blockchain layer, using
  the aggregate private key.
* Use the unhappy path spend below.

> **Non-normative** It is recommended to fall back to unhappy
> path spend if the sidepool aborts before the Contraction Phase
> completes, so that the participant does not have to store the
> aggregate private key into persistent storage.
> However, using the aggregate private key (which would require
> persistently storing it) does have the advantage of a smaller
> witness required for the blockchain transaction.

The happy path completes only once the Contraction Phase
completes.
Participants MUST NOT treat the HTLC as having been resolved
until after the Contraction Phase completes.

Unhappy Path Spend
------------------

HTLCs in sidepools may need to be spent using an unhappy path
spend, where the sidepool is aborted at a state where the HTLC
has not been cooperatively resolved.

The sidepool may be aborted for other reasons, without the HTLC
being the reason for the abort, but participants must still be
prepared to handle this case.

An HTLC in a sidepool may also trigger the sidepool to abort:

* While the offerrer is waiting for the acceptor to either
  fulfill (send preimage) or fail (send acceptor ephemeral
  private key), it MUST NOT participate in the Contraction Phase
  until the acceptor responds.
  This can cause the Contraction Phase to time out if the
  acceptor exceeds the timeout.
* While the acceptor is waiting for private key handover after it
  has fulfilled the HTLC, it MUST NOT participate in the
  Contraction Phase until the offerrer responds.
  This can cause the Contraction Phase to time out if the
  acceptor exceeds the timeout.

In cases where the sidepool aborts before the HTLC can be
resolved with the Contraction Phase completing, the HTLC
participants MUST resolve the HTLC onchain.

The HTLC participants MUST wait for the Last update transaction
of the sidepool to be confirmable.

* If private key handover was completed, the participant with
  unilateral control MAY use the aggregate private key to spend
  the HTLC fund on the blockchain.
* Otherwise, if the acceptor can fulfill the HTLC by revealing
  the preimage, the acceptor MUST use the hashlock branch by
  revealing the aggregate public key (the internal public key of
  the HTLC) and the Merkle tree path to the hashlock branch,
  together with its signature (using the acceptor persistent
  keypair) and the preimage.
* Otherwise, the offerrer MUST wait for the absolute time lock
  to arrive AND for the Last update transaction to actually
  confirm, then use the timelock branch by revealing the
  aggregate public key and the Merkle tree path to the timelock
  branch, together with its signature (using the offerrer
  persistent keypair).
  * The transaction spending the HTLC MUST have an `nLockTime`
    equal or greater than the absolute lock time, and an
    `nSequence` that encodes a relative 1 block lock time.

Secure Private Key Handover
===========================

As noted above, HTLCs are "finalized" by handing over private
keys to the ephemeral keypair, which allows the other side to
compute the private key behind the aggregate ephemeral keypair
used as the Taproot internal public key.

To reduce the amount of code that is exposed to the handed over
private key, we use a secure private key handover protocol.

All SIDEPOOL protocols which require private key handover MUST
have messages that use the secure private key handover protocol
describe here, and MUST NOT have messages with a field, TLV, or
any part of its payload that has the private key in cleartext
after BOLT #8 decryption.

> **Rationale** Although the messages are encrypted on-the-wire by
> standard BOLT #8 tunnel encryption, once the message arrives at
> the node and is decrypted by the BOLT #8 implementation, the
> message payload is now in cleartext, and may be exposed to large
> amounts of code that is not intended to handle private keys.
>
> For example, suppose that SIDEPOOOL messages did not encrypt
> the private key additionally inside the SIDEPOOL message.
> Now suppose a sidepool implementation is run as a CLN plugin.
> CLN sends unrecognized odd messages to *all* plugins (that have
> registered interest in custom (i.e. odd) message IDs) via the
> plugin interface, which at its heart is a pair of UNIX pipes.
> It sends the *decrypted* message payload as a hex string.
>
> This exposes the message payload, as cleartext, to lower-level
> internal libraries used by CLN to handle I/O to plugins, to the
> OS code that is handling the UNIX pipe, and worse, to *all*
> plugins that registered an interest in custom message IDs, not
> just to the sidepool implementation.
> If the private key is sent inside the message payload without
> any additional encryption, all of that code is now exposed to
> the private key, and security escalations on *any* of that code
> can now exfiltrate the ephemeral private key.
>
> By using this secure private key handover, the handed-over
> private key is still encrypted even between the various parts
> of a modern Lightning Network implementation, and cannot be read
> by anything other than the holder of the other ephemeral private
> key, which should be only the sidepool implementation component
> of the node, and not any of the other components.
> This improves the overall security of the private key handover.
> As long as the sidepool implementation itself is secure against
> exploits, no other part of the overall node is exposed to the
> handed-over private key.
>
> This is technically an encryption-within-encryption that is
> superfluous, but is necessary as a defense-in-depth against
> irreducible software complexity.

Prior to private key handover, by necessity, both participants
MUST exchange the ephemeral public keys (as it is needed to form
the aggregate public key used as the Taproot internal public key).

As such, if participant A wants to hand over its own ephemeral
*private* key, it can encrypt the private key using asymmetric
encryption so that only participant B --- and in particular, only
the sidepool implementation of participant B that is holding the
other ephemeral private key --- can decrypt it.
This is done using trivial elliptic curve Diffie-Helman shared
secret being used in a symmetric encryption scheme.

Suppose participant A wants to hand over its ephemeral private
key to participant B:

* Both A and B have already exchanged the below while setting up
  the HTLC:
  * Participant A ephemeral public key.
  * Participant B ephemeral public key.
* Participant A creates a fresh random 256-bit number, the
  encryption private key, and generates the encryption public key.
* Participant A performs ECDH between the encryption keypair and
  the participant B ephemeral keypair:
  * It multiplies the encryption private key with the participant B
    ephemeral public key.
  * It takes the above resulting point and serializes it in the
    standard SECP256K1 33-byte serialization.
  * It takes the SHA-2 256-bit hash of the 33-byte serialization,
    getting the 256-bit symmetric encryption key.
* Participant A XORs the 256-bit symmetric encryption key with the
  (also 256-bit) participant B ephemeral private key, resulting in
  the encrypted participant A ephemeral private key.
* Participant A sends the encryption *public* key and the encrypted
  participant A ephemeral private key to participant B.
* Participant B performs ECDH between the encryption keypair and
  the participant B ephemeral keypair:
  * It multiplies the participant B ephemeral private key with the
    encryption public key.
  * It takes the above resulting point and serializes it in the
    standard SECP256K1 33-byte serialization.
  * It takes the SHA-2 256-bit hash of the 33-byte serialization,
    getting the 256-bit symmetric encryption key.
* Participant B XORs the 256-bit symmetric encryption key with the
  encrypted participant A ephemeral private key, resulting in the
  participant A ephemeral private key.
* Participant B validates that the received participant A
  ephemeral private key is indeed the scalar behind the
  previously-received participant A ephemeral public key.
* Participant B is now in possession of both participant A
  ephemeral private key and participant B ephemeral private key,
  and can compute MuSig homomorphically on the private keys to
  generate the private key corresponding to the aggregate
  ephemeral public key.

```
     Participant A
     .
        CSPRNG -------> enc prv key ------------+
                              |                 |
       A eph prv key ---------(------+     +---------+
       A eph pub key          |      |     | times G |
                              |      |     +---------+
       B eph pub key ------+  |      |          |
                           |  |      |     enc pub key
                           v  v      |          |
                        +--------+   |          |
                        |  ECDH  |   |          |
                        +--------+   |          |
                             |       |          |
                             v       v          |
                          +-------------+       |
                          |     XOR     |       |
                          +-------------+       |
                                 |              |
                         enc A eph prv key      |
                                 |              |
                                 v              v
                            +----------------------+
                            |         CAT          |
                            +----------------------+
      Participant A                    |
     --------------------------------- v ------------------------
                                   .:| | |:.
                             Magic `:| | |:'
        THE CRUEL WORLD      BOLT8 ^:| | |:-
                            Tunnel ,:| | |:.
                                   `:| | |:,
     --------------------------------- v ------------------------
     Participant B                     |
                                       |
       A eph pub key                   |
                                       |
       B eph prv key ------------------(------------------+
       B eph pub key                   |                  |
                                       v                  |
                            +----------------------+      |
                            |         CUT          |      |
                            +----------------------+      |
                              |                  |        |
                      enc A eph prv key     enc pub key   |
                              |                  |        |
                              |                  v        v
                              |               +--------------+
                              |               |     ECDH     |
                              |               +--------------+
                              +--------------------+ |
                                                   | |
                                                   v v
                                                +-------+
                                                |  XOR  |
                                                +-------+
                                                    |
                                   YAY! --->  A eph prv key
```

> **Rationale** Both the data to encrypt (i.e. the participant A
> ephemeral private key), and the symmetric encryption key
> generated by ECDH, are 256 bit in length, and direct XOR is
> simple and does not require a more complicated symmetric stream
> cipher.
> A simple XOR reduces the code that is directly exposed to
> the private key being handed over to only a very simple
> loop that does XOR between two fixed-length byte arrays.

> **Non-normative** A participant handing over its ephemeral
> private key to the other participant implies that the sending
> participant intends to release full control of the fund to the
> other participant, meaning that the sending participant no
> longer has any interest in retaining any kind of even partial
> control of the fund.
>
> As it is no longer interested in control of the fund, the
> sending participant can, and should, securely erase its copy
> of its ephemeral private key as soon as it is encrypted and
> sendable to the counterparty.
>
> Thus, it is recommended that the XOR operation be done "in
> place", i.e. overwriting the buffer that contains the ephemeral
> private key being handed over.
> This simultaneously destroys the local copy of the private key
> being handed over, which is fine since the sender no longer
> intends to make use of it, which reduces the scope in which the
> ephemeral private key is available locally in cleartext.
> This is also unlikely to be optimized away by compilers or
> language interpreters, as the buffer would then be used to send
> data to external I/O.

Sequence Diagrams
=================

Happy Path HTLC Fulfillment
---------------------------

     Leader                     Offerrer                 Acceptor
        |                           |                        |
        |      swap_party_begin     |                        |
        |-------------------------->|                        |
        |---------------------------(----------------------->|
        |                           |                        |
        |                      +-----------------------------------+
        |                      |       Negotiate HTLC details      |
        |                      +-----------------------------------+
        |                           |                        |
        | swap_party_expand_request |                        |
        |<--------------------------(------------------------|
        |                           |                        |
        |<--------------------------|                        |
        | swap_party_expand_request |                        |
        |    with new HTLC output   |                        |
        |                           |                        |
        |  swap_party_expand_state  |                        |
        |-------------------------->|                        |
        |---------------------------(----------------------->|
        |                           |                        |
        |                      +-----------------------------------+
        |                      |    Check HTLC exists in state     |
        |                      +-----------------------------------+
        |                           |                        |
        |  swap_party_expand_sign   |                        |
        |<--------------------------|                        |
        |<--------------------------(------------------------|
        |                           |                        |
        |  swap_party_expand_done   |                        |
        |-------------------------->|                        |
        |---------------------------(----------------------->|
        |                           |                        |
     Leader                     Offerrer                 Acceptor
        |                           |                        |
        |                           |(message with preimage) |
        |                           |<-----------------------|
        |                           |                        |
        |                           | (private key handover) |
        |                           |----------------------->|
        |                           |                        |
        |swap_party_contract_request|                        |
        |<--------------------------|                        |
        |                           |                        |
        |<--------------------------(------------------------|
        |swap_party_contract_request|                        |
        |    claiming HTLC output   |                        |
        |                           |                        |
        | swap_party_contract_state |                        |
        |-------------------------->|                        |
        |---------------------------(----------------------->|
        |                           |                        |
        | swap_party_contract_sign  |                        |
        |<--------------------------|                        |
        |<--------------------------(------------------------|
        |                           |                        |
        | swap_party_contract_done  |                        |
        |-------------------------->|                        |
        |---------------------------(----------------------->|
        |                           |                        |
        |swap_party_contract_done_ack                        |
        |<--------------------------|                        |
        |<--------------------------(------------------------|
        |                           |                        |

In the above diagram `(message with preimage)` is shown as being
from acceptor to offerrer.
However, actual protocols built on top may have the preimage be
sent from the HTLC-in-sidepool offerrer to the acceptor.
For example, there may be an HTLC in a Lightning Network channel
that goes from the "acceptor" above to the "offerrer" above, with
an earlier absolute lock time, so that the "offerrer" here (which
is the acceptor in the Lightning Network HTLC) sends the preimage
to the "acceptor", and then sends the HTLC offerrer ephemeral
private key during handover.
