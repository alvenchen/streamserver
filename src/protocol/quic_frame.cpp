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
        }
    }

    QuicFrame::QuicFrame(PaddingFrame &&in)
        :_type(QuicFrame::TYPE::PADDING_FRAME){
        new (&padding) PaddingFrame(std::move(in));
    }

    QuicFrame::QuicFrame(PingFrame &&in)
        :_type(QuicFrame::TYPE::PING_FRAME){
        new (&padding) PingFrame(std::move(in));
    }

    void QuicFrame::destroy() noexcept {
        switch (_type) {
            case QuicFrame::TYPE::PADDING_FRAME:
                padding.~PaddingFrame();
                break;
            case QuicFrame::TYPE::PING_FRAME:
                ping.~PingFrame();
                break;
        }
    }

    QuicFrame::TYPE QuicFrame::type(){
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


}
