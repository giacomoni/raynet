import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.transportlayer.contract.TransportHeaderBase;

namespace inet::rdp;

cplusplus {{
const B RDP_HEADER_LENGTH = B(20);  // default NDP header length: 20 bytes without options
}}

//
// Represents a NDP segment, to be used with the ~NDP module.
//
// NDP header fields not explicitly modelled: work on going
// - Data Offset (number of 32 bit words in the header): represented
//   by cMessage::length().
// - Reserved (reserved for future use)
// - Checksum (header checksum): modelled by cMessage::hasBitError()
// - Header Options: Currently only EOL, NOP, MSS, WS, SACK_PERMITTED, SACK and TS are implemented
// - Padding
//
// cMessage::getKind() may be set to an arbitrary value: NDP entities will
// ignore it and use only the header fields (synBit, ackBit, rstBit).
//
class RdpHeader extends TransportHeaderBase
{
    chunkLength = RDP_HEADER_LENGTH;

    // Source Port
    unsigned short srcPort; //2 Bytes
    // Destination Port
    unsigned short destPort; //2 Bytes
    unsigned int ackNo; //4 Bytes
	unsigned int nackNo; //4 Bytes
	bool isHeader;
	bool isPullPacket; //2 Bytes
	bool isDataPacket;
	unsigned int numPacketsToSend;
	unsigned int pullSequenceNumber;
	unsigned int dataSequenceNumber; //2 Bytes
	bool ackBit;
	bool nackBit;
    bool rstBit; // RST: reset the connection
    bool synBit; // SYN: synchronize seq. numbers
    simtime_t transmissionTime;
}

cplusplus(RdpHeader) {{
  public:
    // implements TransportHeaderBase functions:
    virtual unsigned int getSourcePort() const override { return getSrcPort(); }
    virtual void setSourcePort(unsigned int port) override { setSrcPort(port); }
    virtual unsigned int getDestinationPort() const override { return getDestPort(); }
    virtual void setDestinationPort(unsigned int port) override { setDestPort(port); }
    virtual std::string str() const override;
}}

