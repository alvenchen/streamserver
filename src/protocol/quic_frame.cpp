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

    QuicFrame::QuicFrame(DataBlockedFrame &&in)
        :_type(QuicFrame::TYPE::DATA_BLOCKED_FRAME){
        new (&dataBlocked) DataBlockedFrame(std::move(in));
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

    PaddingFrame* QuicFrame::paddingFrame(){
        if(_type == QuicFrame::TYPE::PADDING_FRAME){
            return &padding;
        }
        return nullptr;
    }

    PingFrame* QuicFrame::pingFrame(){
        if(_type == QuicFrame::TYPE::PING_FRAME){
            return &ping;
        }
        return nullptr;
    }

    ReadAckFrame* QuicFrame::readAckFrame(){
        if(_type == QuicFrame::TYPE::READ_ACK_FRAME){
            return &readAck;
        }
        return nullptr;
    }

    WriteAckFrame* QuicFrame::writeAckFrame(){
        if(_type == QuicFrame::TYPE::WRITE_ACK_FRAME){
            return &writeAck;
        }
        return nullptr;
    }

    RstStreamFrame* QuicFrame::rstStreamFrame(){
        if(_type == QuicFrame::TYPE::RST_STREAM_FRAME){
            return &rst;
        }
        return nullptr;
    }

    StopSendingFrame* QuicFrame::stopSendingFrame(){
        if(_type == QuicFrame::TYPE::STOP_SENDING_FRAME){
            return &stopSend;
        }
        return nullptr;
    }

    ReadCryptoFrame* QuicFrame::readCryptoFrame(){
        if(_type == QuicFrame::TYPE::READ_CRYPTO_FRAME){
            return &readCrypto;
        }
        return nullptr;
    }

    ReadNewTokenFrame* QuicFrame::readNewTokenFrame(){
        if(_type == QuicFrame::TYPE::READ_NEW_TOKEN_FRAME){
            return &readNewToken;
        }
        return nullptr;
    }
    
    ReadStreamFrame* QuicFrame::readStreamFrame(){
        if(_type == QuicFrame::TYPE::READ_STREAM_FRAME){
            return &readStream;
        }
        return nullptr;
    }

    MaxDataFrame* QuicFrame::maxDataFrame(){
        if(_type == QuicFrame::TYPE::MAX_DATA_FRAME){
            return &maxData;
        }
        return nullptr;
    }

    MaxStreamDataFrame* QuicFrame::maxStreamDataFrame(){
        if(_type == QuicFrame::TYPE::MAX_STREAM_DATA_FRAME){
            return &maxStreamData;
        }
        return nullptr;
    }

    MaxStreamsFrame* QuicFrame::maxStreamsFrame(){
        if(_type == QuicFrame::TYPE::MAX_STREAMS_FRAME){
            return &maxStreams;
        }
        return nullptr;
    }

    DataBlockedFrame* QuicFrame::dataBlockedFrame(){
        if(_type == QuicFrame::TYPE::DATA_BLOCKED_FRAME){
            return &dataBlocked;
        }
        return nullptr;
    }

    ImmediateAckFrame* QuicFrame::immediateAckFrame(){
        if(_type == QuicFrame::TYPE::IMMEDIATE_ACK_FRAME){
            return &immAck;
        }
        return nullptr;
    }

    AckFrequencyFrame* QuicFrame::ackFrequencyFrame(){
        if(_type == QuicFrame::TYPE::ACK_FREQUENCY_FRAME){
            return &ackFrequency;
        }
        return nullptr;
    }



}
