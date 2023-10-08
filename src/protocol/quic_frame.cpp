#include "quic_frame.hpp"


namespace quic{

    QuicFrame::~QuicFrame() {
        destroy();
    }

    QuicFrame::QuicFrame(QuicFrame&& other) noexcept {
        switch (other._type) {
            case QuicFrame::TYPE::PADDING_FRAME:
                new (&padding) PaddingFrame(std::move(other.padding));
                break;
            case QuicFrame::TYPE::PING_FRAME:
                new (&ping) PingFrame(std::move(other.ping));
                break;
            case QuicFrame::TYPE::READ_ACK_FRAME:
                new (&readAck) ReadAckFrame(std::move(other.readAck));
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                new (&writeAck) WriteAckFrame(std::move(other.writeAck));
                break;
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                new (&rst) RstStreamFrame(std::move(other.rst));
                break;
            case QuicFrame::TYPE::STOP_SENDING_FRAME:
                new (&stopSend) StopSendingFrame(std::move(other.stopSend));
                break;
            case QuicFrame::TYPE::READ_CRYPTO_FRAME:
                new (&readCrypto) ReadCryptoFrame(std::move(other.readCrypto));
                break;
            case QuicFrame::TYPE::READ_NEW_TOKEN_FRAME:
                new (&readNewToken) ReadNewTokenFrame(std::move(other.readNewToken));
                break;
            case QuicFrame::TYPE::READ_STREAM_FRAME:
                new (&readStream) ReadStreamFrame(std::move(other.readStream));
                break;
            case QuicFrame::TYPE::MAX_DATA_FRAME:
                new (&maxData) MaxDataFrame(std::move(other.maxData));
                break;
            case QuicFrame::TYPE::MAX_STREAM_DATA_FRAME:
                new (&maxStreamData) MaxStreamDataFrame(std::move(other.maxStreamData));
                break;
            case QuicFrame::TYPE::MAX_STREAMS_FRAME:
                new (&maxStreams) MaxStreamsFrame(std::move(other.maxStreams));
                break;
            case QuicFrame::TYPE::DATA_BLOCKED_FRAME:
                new (&dataBlocked) DataBlockedFrame(std::move(other.dataBlocked));
                break;
            case QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME:
                new (&streamDataBlocked) StreamDataBlockedFrame(std::move(other.streamDataBlocked));
                break;
            case QuicFrame::TYPE::STREAMS_BLOCKED_FRAME:
                new (&streamsBlocked) StreamsBlockedFrame(std::move(other.streamsBlocked));
                break;
            case QuicFrame::TYPE::NEW_CONNECTION_ID_FRAME:
                new (&newConnID) NewConnectionIdFrame(std::move(other.newConnID));
                break;
            case QuicFrame::TYPE::RETIRE_CONNECTION_ID_FRAME:
                new (&retireConnID) RetireConnectionIdFrame(std::move(other.retireConnID));
                break;
            case QuicFrame::TYPE::PATH_CHALLANGE_FRAME:
                new (&pathChallenge) PathChallengeFrame(std::move(other.pathChallenge));
                break;
            case QuicFrame::TYPE::PATH_RESPONSE_FRAME:
                new (&pathResponse) PathResponseFrame(std::move(other.pathResponse));
                break;
            case QuicFrame::TYPE::CONNECTION_CLOSE_FRAME:
                new (&connClose) ConnectionCloseFrame(std::move(other.connClose));
                break;
            case QuicFrame::TYPE::HANDSHAKE_DONE_FRAME:
                new (&handshakeDone) HandshakeDoneFrame(std::move(other.handshakeDone));
                break;
            case QuicFrame::TYPE::DATAGRAM_FRAME:
                new (&datagram) DatagramFrame(std::move(other.datagram));
                break;
            case QuicFrame::TYPE::KNOB_FRAME:
                new (&knob) KnobFrame(std::move(other.knob));
                break;
            case QuicFrame::TYPE::IMMEDIATE_ACK_FRAME:
                new (&immAck) ImmediateAckFrame(std::move(other.immAck));
                break;
            case QuicFrame::TYPE::ACK_FREQUENCY_FRAME:
                new (&ackFrequency) AckFrequencyFrame(std::move(other.ackFrequency));
                break;
        }
        _type = other._type;
    }

    QuicFrame& QuicFrame::operator=(QuicFrame&& other) noexcept{
        destroy();
        switch (other._type)
        {
            case QuicFrame::TYPE::PADDING_FRAME:
                new (&padding) PaddingFrame(std::move(other.padding));
                break;
            case QuicFrame::TYPE::PING_FRAME:
                new (&ping) PingFrame(std::move(other.ping));
                break;
            case QuicFrame::TYPE::READ_ACK_FRAME:
                new (&readAck) ReadAckFrame(std::move(other.readAck));
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                new (&writeAck) WriteAckFrame(std::move(other.writeAck));
                break;
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                new (&rst) RstStreamFrame(std::move(other.rst));
                break;
            case QuicFrame::TYPE::STOP_SENDING_FRAME:
                new (&stopSend) StopSendingFrame(std::move(other.stopSend));
                break;
            case QuicFrame::TYPE::READ_CRYPTO_FRAME:
                new (&readCrypto) ReadCryptoFrame(std::move(other.readCrypto));
                break;
            case QuicFrame::TYPE::READ_NEW_TOKEN_FRAME:
                new (&readNewToken) ReadNewTokenFrame(std::move(other.readNewToken));
                break;
            case QuicFrame::TYPE::READ_STREAM_FRAME:
                new (&readStream) ReadStreamFrame(std::move(other.readStream));
                break;
            case QuicFrame::TYPE::MAX_DATA_FRAME:
                new (&maxData) MaxDataFrame(std::move(other.maxData));
                break;
            case QuicFrame::TYPE::MAX_STREAM_DATA_FRAME:
                new (&maxStreamData) MaxStreamDataFrame(std::move(other.maxStreamData));
                break;
            case QuicFrame::TYPE::MAX_STREAMS_FRAME:
                new (&maxStreams) MaxStreamsFrame(std::move(other.maxStreams));
                break;
            case QuicFrame::TYPE::DATA_BLOCKED_FRAME:
                new (&dataBlocked) DataBlockedFrame(std::move(other.dataBlocked));
                break;
            case QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME:
                new (&streamDataBlocked) StreamDataBlockedFrame(std::move(other.streamDataBlocked));
                break;
            case QuicFrame::TYPE::STREAMS_BLOCKED_FRAME:
                new (&streamsBlocked) StreamsBlockedFrame(std::move(other.streamsBlocked));
                break;
            case QuicFrame::TYPE::NEW_CONNECTION_ID_FRAME:
                new (&newConnID) NewConnectionIdFrame(std::move(other.newConnID));
                break;
            case QuicFrame::TYPE::RETIRE_CONNECTION_ID_FRAME:
                new (&retireConnID) RetireConnectionIdFrame(std::move(other.retireConnID));
                break;
            case QuicFrame::TYPE::PATH_CHALLANGE_FRAME:
                new (&pathChallenge) PathChallengeFrame(std::move(other.pathChallenge));
                break;
            case QuicFrame::TYPE::PATH_RESPONSE_FRAME:
                new (&pathResponse) PathResponseFrame(std::move(other.pathResponse));
                break;
            case QuicFrame::TYPE::CONNECTION_CLOSE_FRAME:
                new (&connClose) ConnectionCloseFrame(std::move(other.connClose));
                break;
            case QuicFrame::TYPE::HANDSHAKE_DONE_FRAME:
                new (&handshakeDone) HandshakeDoneFrame(std::move(other.handshakeDone));
                break;
            case QuicFrame::TYPE::DATAGRAM_FRAME:
                new (&datagram) DatagramFrame(std::move(other.datagram));
                break;
            case QuicFrame::TYPE::KNOB_FRAME:
                new (&knob) KnobFrame(std::move(other.knob));
                break;
            case QuicFrame::TYPE::IMMEDIATE_ACK_FRAME:
                new (&immAck) ImmediateAckFrame(std::move(other.immAck));
                break;
            case QuicFrame::TYPE::ACK_FREQUENCY_FRAME:
                new (&ackFrequency) AckFrequencyFrame(std::move(other.ackFrequency));
                break;            
        }
        _type = other._type;
        return *this;
    }

    QuicFrame::QuicFrame(PaddingFrame &&in)
        :_type(QuicFrame::TYPE::PADDING_FRAME){
        new (&padding) PaddingFrame(std::move(in));
    }

    QuicFrame::QuicFrame(PingFrame &&in)
        :_type(QuicFrame::TYPE::PING_FRAME){
        new (&ping) PingFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ReadAckFrame &&in)
        :_type(QuicFrame::TYPE::READ_ACK_FRAME){
        new (&readAck) ReadAckFrame(std::move(in));
    }

    QuicFrame::QuicFrame(WriteAckFrame &&in)
        :_type(QuicFrame::TYPE::WRITE_ACK_FRAME){
        new (&writeAck) WriteAckFrame(std::move(in));
    }

    QuicFrame::QuicFrame(RstStreamFrame &&in)
        :_type(QuicFrame::TYPE::RST_STREAM_FRAME){
        new (&rst) RstStreamFrame(std::move(in));
    }

    QuicFrame::QuicFrame(StopSendingFrame &&in)
        :_type(QuicFrame::TYPE::STOP_SENDING_FRAME){
        new (&stopSend) StopSendingFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ReadCryptoFrame &&in)
        :_type(QuicFrame::TYPE::READ_CRYPTO_FRAME){
        new (&readCrypto) ReadCryptoFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ReadNewTokenFrame &&in)
        :_type(QuicFrame::TYPE::READ_NEW_TOKEN_FRAME){
        new (&readNewToken) ReadNewTokenFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ReadStreamFrame &&in)
        :_type(QuicFrame::TYPE::READ_STREAM_FRAME){
        new (&readStream) ReadStreamFrame(std::move(in));
    }

    QuicFrame::QuicFrame(MaxDataFrame &&in)
        :_type(QuicFrame::TYPE::MAX_DATA_FRAME){
        new (&maxData) MaxDataFrame(std::move(in));
    }

    QuicFrame::QuicFrame(MaxStreamDataFrame &&in)
        :_type(QuicFrame::TYPE::MAX_STREAM_DATA_FRAME){
        new (&maxStreamData) MaxStreamDataFrame(std::move(in));
    }

    QuicFrame::QuicFrame(MaxStreamsFrame&& in)
        :_type(QuicFrame::TYPE::MAX_STREAMS_FRAME){
        new (&maxStreams) MaxStreamsFrame(std::move(in));
    }

    QuicFrame::QuicFrame(DataBlockedFrame &&in)
        :_type(QuicFrame::TYPE::DATA_BLOCKED_FRAME){
        new (&dataBlocked) DataBlockedFrame(std::move(in));
    }

    QuicFrame::QuicFrame(StreamDataBlockedFrame &&in)
        :_type(QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME){
        new (&streamDataBlocked) StreamDataBlockedFrame(std::move(in));
    }

    QuicFrame::QuicFrame(StreamsBlockedFrame &&in)
        :_type(QuicFrame::TYPE::STREAMS_BLOCKED_FRAME){
        new (&streamsBlocked) StreamsBlockedFrame(std::move(in));
    }

    QuicFrame::QuicFrame(NewConnectionIdFrame &&in)
        :_type(QuicFrame::TYPE::NEW_CONNECTION_ID_FRAME){
        new (&newConnID) NewConnectionIdFrame(std::move(in));
    }

    QuicFrame::QuicFrame(RetireConnectionIdFrame &&in)
        :_type(QuicFrame::TYPE::RETIRE_CONNECTION_ID_FRAME){
        new (&retireConnID) RetireConnectionIdFrame(std::move(in));
    }

    QuicFrame::QuicFrame(PathChallengeFrame &&in)
        :_type(QuicFrame::TYPE::PATH_CHALLANGE_FRAME){
        new (&pathChallenge) PathChallengeFrame(std::move(in));
    }

    QuicFrame::QuicFrame(PathResponseFrame &&in)
        :_type(QuicFrame::TYPE::PATH_RESPONSE_FRAME){
        new (&pathResponse) PathResponseFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ConnectionCloseFrame &&in)
        :_type(QuicFrame::TYPE::CONNECTION_CLOSE_FRAME){
        new (&connClose) ConnectionCloseFrame(std::move(in));
    }

    QuicFrame::QuicFrame(HandshakeDoneFrame &&in)
        :_type(QuicFrame::TYPE::HANDSHAKE_DONE_FRAME){
        new (&handshakeDone) HandshakeDoneFrame(std::move(in));
    }

    QuicFrame::QuicFrame(DatagramFrame &&in)
        :_type(QuicFrame::TYPE::DATAGRAM_FRAME){
        new (&datagram) DatagramFrame(std::move(in));
    }

    QuicFrame::QuicFrame(KnobFrame &&in)
        :_type(QuicFrame::TYPE::KNOB_FRAME){
        new (&knob) KnobFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ImmediateAckFrame &&in)
        :_type(QuicFrame::TYPE::IMMEDIATE_ACK_FRAME){
        new (&immAck) ImmediateAckFrame(std::move(in));
    }

    QuicFrame::QuicFrame(AckFrequencyFrame &&in)
        :_type(QuicFrame::TYPE::ACK_FREQUENCY_FRAME){
        new (&ackFrequency) AckFrequencyFrame(std::move(in));
    }

    void QuicFrame::destroy() noexcept{
        switch (_type) {
            case QuicFrame::TYPE::PADDING_FRAME:
                padding.~PaddingFrame();
                break;
            case QuicFrame::TYPE::PING_FRAME:
                ping.~PingFrame();
                break;
            case QuicFrame::TYPE::READ_ACK_FRAME:
                readAck.~ReadAckFrame();
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                writeAck.~WriteAckFrame();
                break;
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                rst.~RstStreamFrame();
                break;
            case QuicFrame::TYPE::STOP_SENDING_FRAME:
                stopSend.~StopSendingFrame();
                break;
            case QuicFrame::TYPE::READ_CRYPTO_FRAME:
                readCrypto.~ReadCryptoFrame();
                break;
            case QuicFrame::TYPE::READ_NEW_TOKEN_FRAME:
                readNewToken.~ReadNewTokenFrame();
                break;
            case QuicFrame::TYPE::READ_STREAM_FRAME:
                readStream.~ReadStreamFrame();
                break;
            case QuicFrame::TYPE::MAX_DATA_FRAME:
                maxData.~MaxDataFrame();
                break;
            case QuicFrame::TYPE::MAX_STREAM_DATA_FRAME:
                maxStreamData.~MaxStreamDataFrame();
                break;
            case QuicFrame::TYPE::MAX_STREAMS_FRAME:
                maxStreams.~MaxStreamsFrame();
                break;
            case QuicFrame::TYPE::DATA_BLOCKED_FRAME:
                dataBlocked.~DataBlockedFrame();
                break;
            case QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME:
                streamDataBlocked.~StreamDataBlockedFrame();
                break;
            case QuicFrame::TYPE::STREAMS_BLOCKED_FRAME:
                streamsBlocked.~StreamsBlockedFrame();
                break;
            case QuicFrame::TYPE::NEW_CONNECTION_ID_FRAME:
                newConnID.~NewConnectionIdFrame();
                break;
            case QuicFrame::TYPE::RETIRE_CONNECTION_ID_FRAME:
                retireConnID.~RetireConnectionIdFrame();
                break;
            case QuicFrame::TYPE::PATH_CHALLANGE_FRAME:
                pathChallenge.~PathChallengeFrame();
                break;
            case QuicFrame::TYPE::PATH_RESPONSE_FRAME:
                pathResponse.~PathResponseFrame();
                break;
            case QuicFrame::TYPE::CONNECTION_CLOSE_FRAME:
                connClose.~ConnectionCloseFrame();
                break;
            case QuicFrame::TYPE::HANDSHAKE_DONE_FRAME:
                handshakeDone.~HandshakeDoneFrame();
                break;
            case QuicFrame::TYPE::DATAGRAM_FRAME:
                datagram.~DatagramFrame();
                break;
            case QuicFrame::TYPE::KNOB_FRAME:
                knob.~KnobFrame();
                break;
            case QuicFrame::TYPE::IMMEDIATE_ACK_FRAME:
                immAck.~ImmediateAckFrame();
                break;
            case QuicFrame::TYPE::ACK_FREQUENCY_FRAME:
                ackFrequency.~AckFrequencyFrame();
                break;
        }
    }

    QuicFrame::TYPE QuicFrame::type() const{
        return _type;
    }

    PaddingFrame* QuicFrame::asPaddingFrame(){
        if(_type == QuicFrame::TYPE::PADDING_FRAME){
            return &padding;
        }
        return nullptr;
    }

    PingFrame* QuicFrame::asPingFrame(){
        if(_type == QuicFrame::TYPE::PING_FRAME){
            return &ping;
        }
        return nullptr;
    }

    ReadAckFrame* QuicFrame::asReadAckFrame(){
        if(_type == QuicFrame::TYPE::READ_ACK_FRAME){
            return &readAck;
        }
        return nullptr;
    }

    WriteAckFrame* QuicFrame::asWriteAckFrame(){
        if(_type == QuicFrame::TYPE::WRITE_ACK_FRAME){
            return &writeAck;
        }
        return nullptr;
    }

    RstStreamFrame* QuicFrame::asRstStreamFrame(){
        if(_type == QuicFrame::TYPE::RST_STREAM_FRAME){
            return &rst;
        }
        return nullptr;
    }

    StopSendingFrame* QuicFrame::asStopSendingFrame(){
        if(_type == QuicFrame::TYPE::STOP_SENDING_FRAME){
            return &stopSend;
        }
        return nullptr;
    }

    ReadCryptoFrame* QuicFrame::asReadCryptoFrame(){
        if(_type == QuicFrame::TYPE::READ_CRYPTO_FRAME){
            return &readCrypto;
        }
        return nullptr;
    }

    ReadNewTokenFrame* QuicFrame::asReadNewTokenFrame(){
        if(_type == QuicFrame::TYPE::READ_NEW_TOKEN_FRAME){
            return &readNewToken;
        }
        return nullptr;
    }
    
    ReadStreamFrame* QuicFrame::asReadStreamFrame(){
        if(_type == QuicFrame::TYPE::READ_STREAM_FRAME){
            return &readStream;
        }
        return nullptr;
    }

    MaxDataFrame* QuicFrame::asMaxDataFrame(){
        if(_type == QuicFrame::TYPE::MAX_DATA_FRAME){
            return &maxData;
        }
        return nullptr;
    }

    MaxStreamDataFrame* QuicFrame::asMaxStreamDataFrame(){
        if(_type == QuicFrame::TYPE::MAX_STREAM_DATA_FRAME){
            return &maxStreamData;
        }
        return nullptr;
    }

    MaxStreamsFrame* QuicFrame::asMaxStreamsFrame(){
        if(_type == QuicFrame::TYPE::MAX_STREAMS_FRAME){
            return &maxStreams;
        }
        return nullptr;
    }

    DataBlockedFrame* QuicFrame::asDataBlockedFrame(){
        if(_type == QuicFrame::TYPE::DATA_BLOCKED_FRAME){
            return &dataBlocked;
        }
        return nullptr;
    }

    StreamDataBlockedFrame* QuicFrame::asStreamDataBlockedFrame(){
        if(_type == QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME){
            return &streamDataBlocked;
        }
        return nullptr;
    }

    StreamsBlockedFrame* QuicFrame::asStreamsBlockedFrame(){
        if(_type == QuicFrame::TYPE::STREAMS_BLOCKED_FRAME){
            return &streamsBlocked;
        }
        return nullptr;
    }

    NewConnectionIdFrame* QuicFrame::asNewConnectionIdFrame(){
        if(_type == QuicFrame::TYPE::NEW_CONNECTION_ID_FRAME){
            return &newConnID;
        }
        return nullptr;
    }

    RetireConnectionIdFrame* QuicFrame::asRetireConnectionIdFrame(){
        if(_type == QuicFrame::TYPE::RETIRE_CONNECTION_ID_FRAME){
            return &retireConnID;
        }
        return nullptr;
    }

    PathChallengeFrame* QuicFrame::asPathChallengeFrame(){
        if(_type == QuicFrame::TYPE::PATH_CHALLANGE_FRAME){
            return &pathChallenge;
        }
        return nullptr;
    }

    PathResponseFrame* QuicFrame::asPathResponseFrame(){
        if(_type == QuicFrame::TYPE::PATH_RESPONSE_FRAME){
            return &pathResponse;
        }
        return nullptr;
    }

    ConnectionCloseFrame* QuicFrame::asConnectionCloseFrame(){
        if(_type == QuicFrame::TYPE::CONNECTION_CLOSE_FRAME){
            return &connClose;
        }
        return nullptr;
    }

    HandshakeDoneFrame* QuicFrame::asHandshakeDoneFrame(){
        if(_type == QuicFrame::TYPE::HANDSHAKE_DONE_FRAME){
            return &handshakeDone;
        }
        return nullptr;
    }

    DatagramFrame* QuicFrame::asDatagramFrame(){
        if(_type == QuicFrame::TYPE::DATAGRAM_FRAME){
            return &datagram;
        }
        return nullptr;
    }

    KnobFrame* QuicFrame::asKnobFrame(){
        if(_type == QuicFrame::TYPE::KNOB_FRAME){
            return &knob;
        }
        return nullptr;
    }

    ImmediateAckFrame* QuicFrame::asImmediateAckFrame(){
        if(_type == QuicFrame::TYPE::IMMEDIATE_ACK_FRAME){
            return &immAck;
        }
        return nullptr;
    }

    AckFrequencyFrame* QuicFrame::asAckFrequencyFrame(){
        if(_type == QuicFrame::TYPE::ACK_FREQUENCY_FRAME){
            return &ackFrequency;
        }
        return nullptr;
    }


}
