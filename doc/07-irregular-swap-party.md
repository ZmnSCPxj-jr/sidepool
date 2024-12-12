SIDEPOOL-07 Irregular Swap Party Request

Overview
========

As described in [SIDEPOOL-02 Swap Parties][], there
are two kinds of swap parties: regular swap parties
(which are initiated ideally once a day by the pool
leader within a specific hour of the day) and
irregular swap parties (which can be requested by any
pool member at any time, including the pool leader, by
paying some nominal amount, the
`irregular_swap_party_fee_base`, to the Pay-for-Reseat
Fund).

[SIDEPOOL-02 Swap Parties]: ./02-transactions.md#swap-parties

A pool member (pool leader or follower) can only
trigger an irregular swap party if they have
unilateral control of an output in the current
output set that is at least
`sidepool_version_1_min_amount + irregular_swap_party_fee_base`
satoshis, or 10,010 satoshis.

As swap parties are always formally started by the
pool leader, the pool leader can directly provide the
necessary proof that it authorizes the contribution
to pay for the irregular swap party.

This document describes how a pool follower can
request an irregular swap party, and provide the
authorization required to pay for it.

Requesting An Irregular Swap Party
==================================

If a pool follower wants to swap funds on the sidepool
for capacity on a channel, the pool follower sends
the `request_irregular_swap_party` message to the
pool leader.

1.  `request_irregular_swap_party` (700)
    - Sent from pool follower to pool leader.
2.  TLVs:
    * `request_irregular_swap_party_id` (70000)
      - Length: 16
      - Value: The sidepool identifier of the sidepool
        where the sender is requesting an irregular
        swap party.
      - Required.
    * `irregular_swap_party_authorization` (70002)
      - Length: 104
      - Value: A structure composed of:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit.
        - 64 bytes: Taproot signature, as described in
          [BIP-340][].
      - Required.

The signature for `irregular_swap_party_authorization`
signs a [BIP-340 Design][] tagged hash:

    tag = "sidepool version 1 irregular swap party"
    content = Funding Transaction Output ||
              Current Pool Update Counter ||
              X-only Pubkey ||
              Amount

The `Current Pool Update Counter` is the odd value
for the Update Counter before the Expansion Phase of
the swap party being requested.

The above signature is valid for the
`swap_party_begin_irregular_authorization` TLV of 
`swap_party_begin`, as described in [SIDEPOOL-04][].

Receivers of this message MUST validate the following:

* The receiver is the pool leader of the specified
  sidepool ID.
* The specified Taproot X-only Pubkey and Amount
  is in the current output state bag of the specified
  sidepool.
* The signature signs the message as specified above.

After validation, if the pool leader is already
running a swap party (whether regular or irregular),
it SHOULD ignore the request.

After validation, the pool leader MUST initiate a
`ping`-`pong` exchange with all pool followers.
If the pool follower does not respond with a `pong`
within a reasonable time, the pool leader SHOULD
reject the irregular swap party.
If all pool followers respond with `pong`, the
pool leader MUST start an irregular swap party via
`swap_party_begin` with

If the pool leader receives another request while
it is waiting for `pong` responses from the
pool followers, the pool leader SHOULD ignore the
succeeding request.

Rejecting An Irregular Swap Party
=================================

When the pool leader receives a request for an
irregular swap party, before the pool leader attempts
to initiate a swap party, it first attempts to ensure
that all pool members are online by sending `ping`
messages to all pool followers, and waiting for `pong`
responses within a reasonable timeframe.

However, if a pool follower is not able to be raised
in a `ping`-`pong` interaction, it is better for the
pool leader to simply not initiate a swap party.
In that case, an irregular swap party cannot be
requested at that point.

An explicit rejection can then be sent by the pool
leader via `reject_irregular_swap_party`.
Since the expected use of irregular swap parties is
to get additional liquidity just-in-time for a
forwarded payment, this allows the requesting pool
follower to use a different way of acquiring
additional liquidity if one particular sidepool
instance has offline members.

A corresponding "accept" message is unnecessary, as
the `swap_party_begin` message would be sent if an
irregular swap party is successfully initiated.

1.  `reject_irregular_swap_party` (702)
    - Sent from pool leader to pool follower.
2.  TLVs:
    * `reject_irregular_swap_party_id` (70200)
      - Length: 16
      - Value: The sidepool identifier of the sidepool
        where the sender is requesting an irregular
        swap party.
      - Required.
    * `reject_irregular_swap_party_auth` (70202)
      - Length: 104
      - Value: A structure composed of:
        - 32 bytes: Taproot X-only Pubkey
        - 8 bytes: Amount, in satoshis, big-endian 64-bit.
        - 64 bytes: Taproot signature, as described in
          [BIP-340][].
      - Required.
    * `reject_irregular_swap_party_error` (70204)
      - Length: >= 2
      - Value: A structure composed of:
        - 2 bytes: Error code, big-endian 16-bit.
        - variable: Error message, ostensibly a
          UTF-8 encoded human-readable string.
      - Required.

`reject_irregular_swap_party_auth` is the
`irregular_swap_party_authorization` from the
`request_irregular_swap_party` message.

`reject_irregular_swap_party_error` indicates the
specific reason for the rejection.
The error codes for this specification are written
below, however an implementation MAY return other
error codes.

| Error Code | Meaning                                 |
| :--------: | --------------------------------------- |
|      0     | At least one pool follower is offline   |

The error message is intended for a human-readable
UTF-8 string, but implementations MUST ensure proper
hygiene of this string in its context (e.g. by
replacing ASCII control codes with printable
characters, and skipping over invalid UTF-8 byte
sequences).

[BIP-340]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki
[BIP-340 Design]: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki#user-content-Design
[SIDEPOOL-04]: ./04-swap-party.md
