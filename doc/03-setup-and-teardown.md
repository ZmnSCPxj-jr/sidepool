SIDEPOOL-03 Sidepool Setup And Teardown

Overview
========

Sidepool version 1 pools, to be used, must first be set up.
Setting up a new pool is:

* An initiator nominates itself as the pool leader for the new
  pool.
  The initiator must then reserve some of its own funds for use
  in the pool.
* The pool leader invites a number of Lightning Network nodes to
  become pool followers.
  Invited pool followers may or may not accept.
* The pool followers contribute:
  * Pool participant public keys.
  * Some random entropy to generate the sidepool identifier.
  * Optionally, funds.
* The pool leader creates a finalized sidepool identifier, the
  aggregate pool public key, and the list of pool participants
  who have accepted the invitation.
* The pool participants who contribute actual funds cooperatively
  create but not sign the pool funding transaction.
* The pool participants create signatures for the starting state.
  The pool leader then distributes the signatures to all
  participants.
* The pool funding transaction is cooperatively signed and
  broadcasted.
* The pool participants wait for the pool funding transaction to
  be confirmed deeply enough.
  Once confirmed deeply, the pool leader may then initiate swap
  parties, as per [SIDEPOOL-04][].

[SIDEPOOL-04]: ./04-swap-party.md

(TODO)
