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
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                new (&rst) RstStreamFrame(std::move(other.rst));
            case QuicFrame::TYPE::READ_ACK_FRAME:
                new (&readAck) ReadAckFrame(std::move(other.readAck));
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                new (&writeAck) WriteAckFrame(std::move(other.writeAck));
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
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                new (&rst) RstStreamFrame(std::move(other.rst));
                break;
            case QuicFrame::TYPE::READ_ACK_FRAME:
                new (&readAck) ReadAckFrame(std::move(other.readAck));
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                new (&writeAck) WriteAckFrame(std::move(other.writeAck));
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

    QuicFrame::QuicFrame(RstStreamFrame &&in)
        :_type(QuicFrame::TYPE::RST_STREAM_FRAME){
        new (&rst) RstStreamFrame(std::move(in));
    }

    QuicFrame::QuicFrame(ReadAckFrame &&in)
        :_type(QuicFrame::TYPE::READ_ACK_FRAME){
        new (&readAck) ReadAckFrame(std::move(in));
    }

    QuicFrame::QuicFrame(WriteAckFrame &&in)
        :_type(QuicFrame::TYPE::WRITE_ACK_FRAME){
        new (&writeAck) WriteAckFrame(std::move(in));
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
            case QuicFrame::TYPE::RST_STREAM_FRAME:
                rst.~RstStreamFrame();
                break;
            case QuicFrame::TYPE::READ_ACK_FRAME:
                readAck.~ReadAckFrame();
                break;
            case QuicFrame::TYPE::WRITE_ACK_FRAME:
                writeAck.~WriteAckFrame();
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

    RstStreamFrame* QuicFrame::rstStreamFrame(){
        if(_type == QuicFrame::TYPE::RST_STREAM_FRAME){
            return &rst;
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
