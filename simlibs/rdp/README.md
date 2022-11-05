# NDP - Re-architecting datacenter networks and stacks for low latency and high performance OMNeT++/INET Implementation



**Changes to INET **

**inet/common/**


**Add line in Protocol.cc:**
```
const Protocol Protocol::ndp("ndp", "NDP", Protocol::TransportLayer);
```
**Add line in Protocol.h**
```
static const Protocol ndp;
```
**Replace the following code in ProtocolGroup.cc**
```
ProtocolGroup ProtocolGroup::ipprotocol("ipprotocol", {
{ 1, &Protocol::icmpv4 },
{ 2, &Protocol::igmp },
{ 4, &Protocol::ipv4 },
{ 6, &Protocol::tcp },

{ 144, &Protocol::ndp }, //unassigned number for NDP

{ 8, &Protocol::egp },
{ 9, &Protocol::igp },
{ 17, &Protocol::udp },
{ 36, &Protocol::xtp },
{ 41, &Protocol::ipv6 },
{ 46, &Protocol::rsvpTe },
{ 48, &Protocol::dsr },
{ 58, &Protocol::icmpv6 },
{ 89, &Protocol::ospf },
{ 103, &Protocol::pim },
{ 132, &Protocol::sctp },
{ 135, &Protocol::mobileipv6 },
{ 138, &Protocol::manet },
{ 249, &Protocol::linkStateRouting }, // INET specific: Link State Routing Protocol
{ 250, &Protocol::flooding }, // INET specific: Probabilistic Network Protocol
{ 251, &Protocol::probabilistic }, // INET specific: Probabilistic Network Protocol
{ 252, &Protocol::wiseRoute }, // INET specific: Probabilistic Network Protocol
{ 253, &Protocol::nextHopForwarding }, // INET specific: Next Hop Forwarding
{ 254, &Protocol::echo }, // INET specific: Echo Protocol
});
```
