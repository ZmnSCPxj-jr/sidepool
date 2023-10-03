SIDEPOOL-00 Sidepools Are Awesome

Overview
========

Sidepools are awesome.

A *sidepool* is an offchain construct --- a pool of funds owned by
multiple Lightning Network forwarding nodes --- that sits "at the
side" of both the Lightning Network and the Bitcoin blockchain
network.

Sidepools occupy a "sweet spot", providing a strategic advantage
over plain Lightning Network channels, while not being as
complicated as channel factories or hierarchical channels.

* Like plain Lightning Network channels, they are implementable
  today without changes to the Bitcoin blockchain network (i.e.
  it sits "at the side" of the Bitcoin blockchain network, not
  requiring additional changes to it, thus removing a blocking
  dependency).
* Unlike plain Lightning Network channels, they do not "lock" your
  funds to specific counterparties, which includes risk of a
  "high pressure" condition where a certain node mismanages its
  liquidity, preventing other nodes from earning by forwarding
  payments to it.
  Often, the end result of such high-pressure conditions is that
  other nodes will start buying underpriced liquidity from each
  other, then attempting to resell at a higher price point.
  However, such actions do not benefit payers, as they only shift
  the high-pressure condition to nodes that foolishly underprice
  their liquidity.
* Hierarchical channels and true CoinPools require changes to the
  underlying Bitcoin consensus, which are difficult to get into
  Bitcoin implementations.
  We thus expect that deployment of such constructs will be slow.
* Nested constructs, such as hierarchical channels or channel
  factories, imply greater complexity of implementation.
  Lightning Network node implementations that currently have
  channels directly on the blockchain must now adjust their
  persistent stors formats to handle channels that are nested in
  other constructs.
  Nested constructs are leaky abstractions.
  In contrast, by sitting "at the side" and not nesting channels
  inside of novel offchain constructs, sidepools do not require
  changes to existing node implementations.

This is not part of the formal specification of the SIDEPOOL
protocol, but it does serve as an extended rationale for why
the SIDEPOOL protocol exists at all.

The Pressure Problem
====================

The Lightning Network as of late 2023 has a severe issue with
local high-pressure points.

Let us return some years back.

Naively, people thought that situations like this would somehow be
common:

         99            1
       A --------------- B
     1 |                 | 99
       |     99   1      |
       +------- C -------+

Briefly: the lines are channels, `A`, `B`, and `C` are Lightning
Network nodes, and the little numbers are how much each node has
in that channel.

Now, the above situation, it would *obviously* be to the interest
of all participants involved to shift all of the channel states to
"balanced", i.e. the ideal would be to set the amounts to this:

         50           50
       A --------------- B
    50 |                 | 50
       |     50   50     |
       +------- C -------+

There was even a paper proposing an extension to Lightning
protocol in order to achieve that state from the first state.

Of course, [some][LN Has Rebalance Already 1] [people][LN Has
Rebalance Already 2] pointed out the obvious: you could just send
a 49-unit payment `A`->`B`->`C`->`A` and get the same result.

[LN Has Rebalance Already 1]: https://lists.linuxfoundation.org/pipermail/lightning-dev/2017-December/000860.html
[LN Has Rebalance Already 2]: https://lists.linuxfoundation.org/pipermail/lightning-dev/2018-February/000996.html

Thus was circular rebalancing born.

Indeed, it has been proposed that some kind of
"friend-of-a-friend" network can be created, so that you can
search for such opportunities to perform rebalances by asking your
friends if they have channels with each other that would be
rebalanceable in the same direction as your imbalances with them.
Such friend-of-a-friend network would, instead of charging fees
for the rebalance, charge 0 fees, while charging fees for "real"
payments.

However, we should pay attention to what is actually observed on
the network, and from there, ask the question:

* Are such communally-incentivized opportunities for circular
  rebalancing actually common?

So let us return to the initial motivating example, and add a
few more nodes:

       D                     E
     0 |                     | 20
       |                     |
     20 \    99       1     / 0
         A --------------- B
       1 |                 | 99
         |     99   1      |
         +------- C -------+

Suppose `E` wants to pay `D` 20 units.

* As `E` is unaware of the state of the channels other than the
  channel between `E` and `B`, it naively selects a path
  `E`->`B`->`A`->`D`.
* The above path fails, because `B` cannot send 20 units to `A`;
  it only has 1 unit in the `B`->`A` direction.
* `E` receives the failure, and tries the path
  `E`->`B`->`C`->`A`->`D`.
* This succeeds since `B` can send 20 units to C, which can send
  20 units to `A` etc.

This suggests the following lemma:

* If opportunities to cleanly rebalance, such that *every* node
  would be happy about the rebalance, like in the motivating
  example where `A`, `B`, and `C` have liquidity allocations such
  that they would *want* a rebalance and would be willing to
  charge 0 fees for it:
  * *then* payment failures would occur if rebalancing were not
    done, but after a few retries (say no more than 5 times)
    the payment would succeed.

The above lemma is of the form `rebalance_ok` ⊃ `few_failures`.
By [modus tollens][], if we observe ~`few_failures`, we must
conclude ~`rebalance_ok`.

[modus tollens]: https://en.wikipedia.org/wiki/Modus_tollens

Or in other words: contrary to the above expectation, we see
payment failures where more than 5 attempts *still* fails to
reach the destination.
This implies that there are few, if any, opportunities where a
friend-of-a-friend network would agree that a rebalancing would
benefit all of them.
That is, the motivating example for supporting circular rebalances
is actually *rare* in practice, because if it were common, real
payments would *already* rebalance those channels naturally, and
explicit circular rebalances would be unnecessary.

This implies that the situation that *is* common is actually
closer to this:

       D                     E
     0 |                     | 20
       |                     |
     20 \    99       1     / 0
         A --------------- B
       1 |                 | 18
         |     99   82     |
         +------- C -------+

The above situation matches what we see more often --- `E` fails
to pay `D` even though `D` has enough inbound liquidity from its
LSP `A` to receive the 20-unit payment, and even though `E` takes
a dozen or more attempts at reaching `D`.

This implies that circular rebalances would *not* help network
health.
Any rebalancing here would not actually solve the "high pressure"
of `E` wanting to route to `D` but there is no viable route at
all among all possible paths, even ridiculously long paths.

However, we have some evidence that circular rebalancing is still
a popular pastime:

* Actual published statistics from merchants that have enabled
  Lightning suggests that Lightning adoption is slow, yet there is
  a surprising amount of activity (i.e. actual payments that
  succeed, therefore not probing attempts by cheapskates) that
  seems larger than wahat published statistics attest.
  * This is weak evidence, since merchants might lie, but one
    presumes that if they made the effort to integrate Lightning
    they would cry to the high heavens that they got a *lot* more
    of their economic activity on Lightning if they actually did
    get much more activity on Lightning.
* Nodes that charge 0 fees always report high activity (channels
  with them have a *lot* of HTLCs resolved).
  However, node implementors have noticed that 0-fee nodes have
  very high rates of failure for *actual* payments.
  In fact, some Lightning Network node implementors have
  specifically written pathfinders that *completely avoid* 0-fee
  nodes.
  * This bolsters and magnifies the above weak evidence: it
    suggests that there is a lot of activity here, that is *not*
    activity for customers-paying-merchants transactions.

The above two facts suggest that circular rebalancing is popular,
because other than circular rebalancing and
customers-paying-merchants, there is little reason to send out an
HTLC and get it fulfilled (people who are "only" probing would
send out HTLCs that they know cannot be fulfilled, so that they
would not pay fees).

Why is circular rebalancing still popular, even though we have
evidence (high failure rates) that situations that would be
*helped* by circular rebalancing are actually rare?

The reason is that some nodes (possibly only a few, or even just
one) are running scripts that try to find underpriced liquidity
and rebalance that liquidity, shifting it to their own nodes,
and then charging a higher price for the same liquidity.
They run these scripts continuously, so that any new cheap
liquidity is quickly bought and then transferred to their nodes,
but the much rarer customers-paying-merchant case, which trigger
at random times and do not happen continuously, are unable to
take advantage of cheap liquidity --- by the time the
customers-paying-merchants case occurs, there is a high
probability that cheap liquidity has already been moved to
higher-fee nodes.

This hypothesis neatly explains the available evidence:

* Lightning is not actually as popular as we hope, because its UX
  for the customers-paaying-merchants use-case is worse than
  expected, with higher fees and lower success rates than we
  thought would happen.
  * This is evidenced by the fact that merchants that *have*
    implemented non-custodial Lightning do not see it being used
    as much as onchain payments.
  * The nodes that run rebalancing bots are increasing the actual
    price of actual customers-paying-merchants, because they are
    buying up all the available cheap liquidity.
    However, their rebalancing *does not* increase payment success
    rate, because they are only moving liquidity around, not
    adding new liquidity where it is vitally needed.
    * They are arbitraging, but unlike other examples of
      arbitrage, they are not actually helping by moving liquidity
      to where it is needed, they are just creating local
      liquidity monopolies and then overcharging users.
    * Indeed, this arbitrage is economically worthless, as 1
      satoshi in one channel is equivalent to 1 satoshi in another
      channel, once both channels are dropped onchain.
      There is no strategic advantage to having the right 1
      satoshi in the right place that ultimately benefits the
      entire economy.
* Nodes that charge 0 fees see a lot of payment activity --- which
  according to this hypotheis is not due to
  customers-paying-merchants, but instead due to arbitraging nodes
  buying up their literally free liquidity and then reselling it
  later at a higher non-0 price.
  * This also ties in to why payers hate 0-fee nodes so much that
    some node implementors have gone so far as to modify their
    pathfinders to remove them: by the time actual
    customers-paying-merchants get around to choosing to pay a
    merchant via Lightning, the liquidity of 0-fee nodes towards
    that merchant has already been purchased by the arbitrageurs
    and are now on higher-fee channels.

In short: there is a pressure problem in the Lightning Network.
Some nodes are popular destinations, but are simply not getting
enough inbound liquidity.

Circular rebalances do not help the situation: they just move
whatever little available liquidity there is from low-fee channels
to high-fee channels.

No amount of re-incentivizing or enabling circular rebalances will
help: the problem is that of raw lack of liquidity.
If there is a raw lack of liquidity, then no number of circular
rebalances would correct that lack: all that they would do is
to move what liquidity there is elsewhere.

This lack of liquidity is a sort of "pressure" in the network.
Since the Lightning Network is literally a series of pipes, the
pressure builds up (i.e. a node gets more payments than their
*actual* inbound liquidity can handle).
Circular rebalances move that pressure from one part of the
network to another, but do not release that pressure from the
network.

The Naivety Of Local Views
--------------------------

"But I have a ton of inbound liquidity on my channels!
Why are my customers still sufferring from payment failures?
Surely you are wrong, Zman!"

Suppose you, dear merchant `A`, can see that your channels look
like this:

           1        99
       A ------------- B
       | \
       |  \----------- C
       |   1        99
        \
         \------------ D
           1        99

What you do *not* see is that this is only a local view, and the
real situation globally, including your payer, `P`, is like this:

           1        99   99        1    1    99
       A ------------- B ----------- E --------- P
       | \                         / |
       |  \----------- C ---------/  |
       |   1        99   99      1   |
        \                           /
         \------------ D ----------/
           1        99   99      1

In short: yes, you bought inbound liquidity, probably at great
expense from some inbound liquidity market.
But where did the inbound liquidity provider get *their* inbound
liquidity?
Do they even *have* inbound liquidity to resell you?

Buyers beware: inbound liquidity marketplaces cannot assure you
that sellers of inbound liquidity have inbound liquidity to
actually sell.

Inbound liquidity marketplaces can only assure you that sellers
have *onchain funds* that they can put into new channels.
They might even measure the uptime of their sellers, which is nice
trivia to know about but still not what you want.
Whether those new channels to you are actually *effective* at
getting payments to you *from your payers* is another story.

What you, dear merchant, actually ***want*** is to get liquidity
*from your payers*.
There is a disconnect in what an inbound liquidity marketplace
*can* provide you, versus what you *actually want*.
Speaking of disconnect, heck, the inbound liquidity provider could
be offline 72 blocks every day, but if all your payers only buy
during the 72 blocks that their node *is* online, and your payers
have actual liquidity towards that node, then you do not care
about their uptime being a laughable 50%, you care about *payments
actually reaching you*.

Unfortunately for you, there is no incentive to tell you that
they actually have no liquidity from your payers.
There *is* an incentive for the inbound liquidity provider to lie
to you about it: it means you pay them and they can charge you for
the cost to open the channel, then if they manage to route even
just 1 measly payment they already win because their cost is 0
(because they helpfully shifted the cost to you).

(No, being forced to lock their funds towards you does not change
that argument, what do you think the word "*cost*" means?
If they lock the funds to you for some time mandated by the
inbound liquidity marketplace, when they could have instead ran a
JoinMarket maker bot and earned passively, then the fact that they
lock their funds is an opportunity cost (they could have instead
ran a JoinMarket maker during the time they locked the funds to a
channel), but just like the cost of channel opening, you can bet
that the osst has been shifted to you.)

Worse, there is also an incentive for runners of inbound liquidity
marketplaces to cover up the above, because obviously the sellers
of the inbound liquidity are going to pay, at least in terms of
mind-share, the inbound liquidity marketplace.
And of course the inbound liquidity providers already also
helpfully shifted that cost to you.

The Real Solution: Offchain-to-onchain Swaps
--------------------------------------------

My strong suggestion is to do something like the following:

First, pick a node on the network.
Any node.
Yes, any node.
Strongly prefer nodes with high connectivity and good uptime
(which you can measure directly by yourself, over the course of a
few days, no need to ask that information from some dubious third
party whose incentives are not necessarily aligned with yours).
Even if you filter out most public nodes that have low capacity of
channels and bad uptime, you will still have a few hundred
possible candidates for `R`.

Make a channel to that random node `R` (where you are `A`).

                A
                | 100
                |
                | 0
                R

Now you do not see the real condition of the network beyond your
channel to `R`, but as I will demonstrate, that will not matter.

Now pick any kind of swap service provider.
What is a swap service provider?
It is something like Lightning Loop or Boltz: a service that you
can pay over Lightning in order to get onchain funds, or vice
versa, a service you can pay onchain in order to get funds over
Lightning.
Yes, any swap service provider.

Now use the swap service provider: pay them 50 units over
Lightning to get 50 units of onchain funds.

Suppose that R has a channel with the swap service provider `S`.
It does not matter whether it has a *direct* channel or not, it
only matters that there is some path of channels between `R` and
`S` (and therefore that `S` is reachable from you, `A`).

Suppose that the channel (or sequence of channels, whatever)
between `R` and `S` looks like this:

                A
                | 100
                |
                | 0
                R
                 \-------- S
                 1000    0

Then you perform the swap, paying 50 units over Lightning to `S`,
and then getting the same amount onchain.
On Lightning, the state becomes:

                A
                | 50
                |
                | 50
                R
                 \-------- S
                  950   50

Now you have 50 units of inbound liquidity from `S` to `R` to you,
meaning you effectively bought inbound liquidity from `S`.
And now that the swap has succeeded.

* You got inbound liquidity from `S`->`R`->`A`.
* You got important evidence: *it is possible for a payer to reach
  `S` from some randomly-selected node `R`*.

The last point is the key advantage of swap providers over inbound
liquidity marketplaces.
Providing this bit of evidence is *atomic* and inseparable from
the same act that gets you inbound liquidity!

Suppose that instead, the original state was like this before you
swapped from Lightning to onchain funds:

                A
                | 100
                |
                | 0
                R
                 \-------- S
                 0     1000

In the above case, then you will fail to pay `S` over Lightning,
meaning the swap does *not* happen.

However, *because* the swap does not happen, it means that *you do
not pay `S`*, because the swap did not occur and they only get
paid if the swap succeeds.
Sure, you failed to get inbound liquidity --- but you also did not
pay `S` for *bad* inbound liquidity that you cannot actually use.
What you should now do is to look for a differrent `S` swap
service, while still retaining the channel to `R`.

This is very incentive-compatible to you as someone buying inbound
liquidity:

* You actually *get evidence* that the swap provider has good
  inbound liquidity from some random node, which implies that it
  has good inbound liquidity from the rest of the network.
  * After all, the node was chosen randomly, and it is hard for
    the swap provider to predict which random node `R` you are
    going to use as random sample.
    If you want stronger evidence, connect to more random nodes
    `R` and buy more inbound liquidity by doing more
    Lightning-to-onchain swaps: every successful swap is another
    bit of evidence of the reliability of node `S` in terms of
    inbound liquidity.
* If the swap provider is unable to provide that evidence that they
  can be reached over a random node on the network (i.e. paying
  them over Lightning fails), you *do not* buy inbound liquidity
  from them.
  This is atomic: the failure to reach them means you never buy
  inbound liquidity from them and can instead find some other
  swap provider that *can* take your money.

Thus, swap providers are a superior method of acquiring inbound
liquidity than inbound liquidity marketplaces!
This is why [CLBOSS uses swap services][].

[CLBOSS uses swap services]: https://zmnscpxj.github.io/clboss/01-inbound.html

Further, swap providers are in the business of, well, *providing
swaps*.
They have a strong incentive to ensure that *they* have good
inbound liquidity frome elsewhere, because they get paid if and
only if the Lightning side of the swap reaches them.
If they do not have good inbound liquidity themselves, then they
fail to get paid for swap services and have no business model.

Of course, current swap providers tend to be somewhat expensive,
but mostly only because there is not enough competition among swap
providers.
And another reason is that onchain is expensive, and one hop is
done onchain.

But as noted a few subsections up, you really cannot "release the
pressure" on the Lightning Network by just doing circular
rebalances over just Lightning Network channels.
The Lightning Network is just a bunch of pipes ("channels") andd
if you move pressure from one pipe, you have to put more pressure
in another pipe.
In order to actually *release the pressure* you have to have
*one* hop that is done *outside* the Lightning Network pipes,
e.g. by having a sequence of hops inside Lightning Network
pipes, then terminating into a hop on the blockchain, also known
as "offchain-to-onchain swap", like what you just did in buying
inbound liquidity via a swap provider.

Sidepools Are Awesome Part 1: Sidepools Make Swaps Better
---------------------------------------------------------

The key insight here is that buying inbound liquidity from swap
providers is awesome because it "relieves pressure" by shunting
insufficient liquidity issues to the larger Bitcoin blockchain
layer.

Because sidepools are offchain, operations in the sidepool are
less expensive than blockchain operations, reducing the costs of
swapping between Lightning and some external "pressure chamber".

Now, sidepools are not as good as the Bitcoin blockchain:
Swaps on the Bitcoin blockchain can go to *anyone at all*, but
swaps between Lightning and a sidepool can only go to participants
in the same sidepool.

However, releasing the pressure inside the Lightning Network pipes
to a wider part that is still limited to a small set of nodes, is
still an improvement over the situation where pipes cannot relieve
their pressure at all.

In short, sidepools improve swap-with-Lightning by providing a
viable alternative: instead of just a few Lightning-to-onchain
swap providers, you can now spin up some number of sidepools and
additionally have the option of swapping between sidepool and
Lightning.
By providing competition, the additional earnings margins of
Lightning-to-onchain swaps is reduced, to the benefit of the rest
of the network and the wider economy.

I should also make it very clear that a sidepool is less ideal a
pressure-release chamber for Lightning than the Bitcoin
blockchain:
they only release pressure to more nodes, instead of just having
pressure in the lowest-fee nodes.
Swapping to the Bitcoin blockchain effectively releases pressure
to the rest of the Bitcoin world.
However, this is already represented by the fact that sidepool
operations are cheaper ("less valuable") than Bitcoin blockchain
operations.

Sidepools effectively act as an additional buffer holding pending
payments that have been finalized in the network but not yet
published on the blockchain.
The hope is that aggregation of payments between multiple nodes
will allow effective cut-through and sufficient circular economy
that fewer transfers of funds to and from the sidepool to the
wider Bitcoin blockchain need to happen.
That is: the hope is that most swaps occur between Lightning and
sidepool, and then as the pressure in Lightning "leaks into" the
sidepools supporting it, some of the pressure in the sidepools
are then released to the wider Bitcoin blockchain, with fewer
operations as you move from Lightning to sidepool to blockchain.

Sidepools Are Awesome Part 1.1: Forwardable Peerswaps Are Awesome
-----------------------------------------------------------------

It is a truth universally acknowledged, that a single network in
possession of a good number of nodes, must be in want of a
packet-switching scheme.

Packet switching is simply that individual nodes decide locally
where to route objects known as "packets".

We should note that all networks are like the Lightning Network.

Sometimes, when a node wants to send something --- data in a
"normal" Internet Protocol network, an HTLC in Lightning --- to
some neighboring node it has some connection with --- a physical
connection in an Internet Protocol network, a channel in Lightning
--- the "next hop" would be unable to handle that due to capacity
issues --- filling too many buffers in an Internet Protocol
network, channel liquidity depletion in Lightning.

(It is not material that in an Internet Protocol network the
buffers will be "quickly" cleared as the next node continues its
processing work, or just decides to drop data; the same "clearing"
would also occur in Lightning once payment flows in the opposite
direction.
It is only a matter of expected time frames --- "quick" clearing
of buffers in Internet Protocol, vs. slow, maybe many months that
a Lightning Network channel would be congested before payment
flows reverse and decongest the channel.)

What the Internet Protocol does is to let individual intermediate
nodes decide *where* to push data at.
If there are two possible "next nodes" at this node, and one of
them is congested, then obviously this node can select the other
one.

This is not what the Lightning Network does.

The reason the Internet Protocol can do that is because the
destination Internet Protocol address (a.k.a. IP Address) is
known by each individual node;
the destination is sent in cleartext and as the data travels
over the Internet Protocol network, the destination, in cleartext,
travels with it.
This allows intermediate routing nodes on the Internet Protocol
network to decide the best next hop (by measuring the distance
there to the final destination), and if that next hop is
congested, select the next best next hop (while still ensuring
that they can progress to become nearer to the destination), etc.

Unfortunately, because the destination is given in cleartext,
intermediate nodes can now easily snoop who is sending what data
to whom.
This is the basis of egregious censorship and surveillance, such
as the Great Firewall of China.

In an effort to gain freedom against censorship and surveillance,
the Lightning Network decided to not give the destination in
cleartext to every intermediate node.
Unfortunately, this lead to a problem: how do we get the payment
to reach the destination?
Obviously we cannot use packet routing, a la the Internet
Protocol, anymore.

What Lightning Network does is to have the entire route of a
payment mapped out by the source of the payment.
That way, only the source knows the final destination.
Intermediate nodes only know the next hop, but (1) cannot be
sure that the next hop is the last and (2) cannot be sure if the
previous hop they got it from was the first.

Unfortunately, this means that if there is congestion along the
route --- i.e. a lack of liquidity --- then the intermediate
nodes cannot "correct" by routing the payment to an alternate
next hop.
The intermediate node cannot know which among its other,
non-congested neighbors will still be able to get the payment to
the destination, because the intermediate node does not know the
destination.
Instead, the intermediate node has to communicate *all* the way
back to the payment source, which has to re-decide a new next hop,
but without any information about which alternate hop has
enough liquidity (because giving out that information is equivalent
to leaking how much funds you own in a channel, allowing payment
sources to surveill intermediate nodes!).

This is why payment failures ***SUCK*** in the Lightning Network.
Most (>90%) of failures are due to insufficient liquidity (i.e.
too much congestion on a hop).
When that occurs, the local hop cannot correct, it has to feed the
failure back all the way to the source node, and then the source
node guesses some other alternative route (which might not work
either!), then has to re-establish payments all the way back to
the erring node with a new next hop to try.
Payment failures suck because source routing sucks.

Unfortunately we cannot safely use packet routing for payments
because that totally leaks who gets paid, and we consider privacy
more important.

However, some random wog with an unpronounceable name realized
that he could apply the virtualization argument ("how do you know
the difference between the real world and the dream world, Neo?")
to a novel swap protocol called "peerswap".

A "*peerswap*" is a swap that is restricted to *only* being with a
direct peer.
That is, a peerswap requires that you have a direct channel with
the swap service.

Now this sucks totally because the entire point of using a swap
service to buy inbound liquidity is that you can use random
sampling to get evidence that the swap service *has* inbound
liquidity to resell you.
The technique fails if you make a channel directly to the swap
service.
Thus, peerswaps, as-is are pointless --- they are just an indirect
way of buying inbound liquidity, but you still suffer from the
problem of any inbound liquidity marketplace: you have no evidence
that the inbound liquidity you are buying is any good at all.

However, you can do two things to improve peerswaps and create
"*forwardable peerswaps*":

* Restrict the swap protocol to only allowing paying onchain to
  get funds in-Lightning, and disallow th reverse.
* Apply the virtualization argument:
  * You can offer to pay onchain to get in-Lightning funds to a
    direct peer (hence "*peer*swap").
  * The direct peer can, instead of actually handling the onchain
    funds, actually send the same offer to *its* peer (hence
    "*forwardable*").
  * And so on, until some node on the network decides it really
    does want to get onchain funds and give you in-Lightning
    funds.
  * How would you even know the difference?
    "What *is* 'real'?  How do you define 'real'?"

The neat thing about forwardable peerswaps is that *it allows
packet switching*.

Recall that when you requested a peer so you can pay them onchain
to get in-Lightning funds, that peer has the *choice* of accepting
directly at their node --- or to forward the peerswap to *any*
other node.
This is in fact the same as the packet switching in the Internet
Protocol: the node chooses where to send the hop.
The only requirement is that the forwarding node only really needs
to find a peer where it wants to have more liquidity towards.

And so, we are able to reacquire packet switching in the Lightning
Network.

Note that there is no privacy issue here.
Unlike with payments, you, who has some funds onchain, do not
actually care *where* the peerswap ends up getting forwarded to.
The peerswap has no destination address, because it *can* be
forwarded *anywhere*, the only requirement is it has to reach
*any* node that is willing to get onchain funds if they pay
in-Lightning funds.

Now, why would *you* initiate a peerswap (that you are not
forwarding from somewhere else) in the firrst place?
This is because you need to have some additional liquidity towards
that peer.

If that peer needs some additional liquidity towards some *other*
node that is *its* peer, then it can forward the peerswap to that
node.

What if the node does not need additional liquidity towards any
other of its own peers?
That implies that it already *has* liquidity towards its other
peers.
But high liquidity *towards* its peers implies a *lack* of
liquidity *from* its peers, because Lightning Network channels
have maximum capacities.
This means that they lack inbound liquidity, and would willingly
accept some onchain funds in order to get inbound liquidity.

You, as an initiator of a peerswap that ends up getting forwarded,
want to have more outbound liquidity.
Every hop along the path where the peerswap ends up getting,
will naturally look for a next hop that needs liquidity coming
from your direction (it would be foolish of them to forward the
peerswap towards a node that has more inbound liquidity than it
needs, because that node will naturally reject it).
Then the sequence of forwarded peerswaps will terminate at some
node that wants inbound liquidity and will be willing to handle
some onchain funds to get that inbound liquidity.

Thus:

* The forwardable peerswap naturally routes from nodes that want
  outbound liquidity to nodes that want inbound liquidity.
* None of the intermediate nodes know who the start and end points
  are.
* The start and end points do not know if this was a single-hop
  swap or one over many hops.
* Decisions are made locally with local knowledge, without having
  to keep asking the start point.
* The entire thing improves the health of the network globally,
  while requiring only local knowledge and local decisionmaking.

Thus, forwardable peerswaps are awesome.

Now, as described above, forwardable peerswaps are about moving
onchain funds to in-Lightning funds.
But nothing really *requires* the non-Lightning side to be a
blockchain.
We can implement forwardable peerswaps where the initiator has
some funds in a *sidepool* and wants to pay over the sidepool and
get in-Lightning funds in return.

Indeed, the sidepool protocol has at its core the use of swaps
between peers as the primary method of managing liquidity on the
Lightning Network using the sidepool.
And the sidepol protocol is designed to make it easy to implement
forwarding of the peerswaps.

That is another reason why sidepools are awesome.

The HTLC Default Problem
========================

(TODO)
