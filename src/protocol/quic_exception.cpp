#include "quic_exception.h"


namespace quic{

    QuicErrorCode::~QuicErrorCode() {
        destroy();
    }

    QuicErrorCode::QuicErrorCode(QuicErrorCode&& other) noexcept {
        switch (other._type) {
            case QuicErrorCode::TYPE::APP_ERR_CODE:
                new (&appErr) ApplicationErrorCode(std::move(other.appErr));
                break;
            case QuicErrorCode::TYPE::LOCAL_ERR_CODE:
                new (&localErr) LocalErrorCode(std::move(other.localErr));
                break;
            case QuicErrorCode::TYPE::TRANSPOORT_ERR_CODE:
                new (&transportErr) TransportErrorCode(std::move(other.transportErr));
                break;
        }
        _type = other._type;
    }

    QuicErrorCode& QuicErrorCode::operator=(QuicErrorCode&& other) noexcept{
        destroy();
        switch (other._type) {
            case QuicErrorCode::TYPE::APP_ERR_CODE:
                new (&appErr) ApplicationErrorCode(std::move(other.appErr));
                break;
            case QuicErrorCode::TYPE::LOCAL_ERR_CODE:
                new (&localErr) LocalErrorCode(std::move(other.localErr));
                break;
            case QuicErrorCode::TYPE::TRANSPOORT_ERR_CODE:
                new (&transportErr) TransportErrorCode(std::move(other.transportErr));
                break;
        }
        _type = other._type;
        return *this;
    }

    QuicErrorCode::QuicErrorCode(ApplicationErrorCode &&in)
        :_type(QuicErrorCode::TYPE::APP_ERR_CODE){
        new (&appErr) ApplicationErrorCode(std::move(in));
    }

    QuicErrorCode::QuicErrorCode(LocalErrorCode &&in)
        :_type(QuicErrorCode::TYPE::LOCAL_ERR_CODE){
        new (&localErr) LocalErrorCode(std::move(in));
    }

    QuicErrorCode::QuicErrorCode(TransportErrorCode &&in)
        :_type(QuicErrorCode::TYPE::TRANSPOORT_ERR_CODE){
        new (&transportErr) TransportErrorCode(std::move(in));
    }

    void QuicErrorCode::destroy(){
        switch (_type) {
            case QuicErrorCode::TYPE::APP_ERR_CODE:
                appErr.~ApplicationErrorCode();
                break;
            case QuicErrorCode::TYPE::LOCAL_ERR_CODE:
                localErr.~LocalErrorCode();
                break;
            case QuicErrorCode::TYPE::TRANSPOORT_ERR_CODE:
                transportErr.~TransportErrorCode();
                break;
        }
    }

    QuicErrorCode::TYPE QuicErrorCode::type(){
        return _type;
    }

    ApplicationErrorCode* QuicErrorCode::ApplicationErrorCode(){
        if(_type == QuicErrorCode::TYPE::APP_ERR_CODE){
            return &appErr;
        }
        return nullptr;
    }

    LocalErrorCode* QuicErrorCode::LocalErrorCode(){
        if(_type == QuicErrorCode::TYPE::LOCAL_ERR_CODE){
            return &localErr;
        }
        return nullptr;
    }

    TransportErrorCode* QuicErrorCode::TransportErrorCode(){
        if(_type == QuicErrorCode::TYPE::TRANSPOORT_ERR_CODE){
            return &transportErr;
        }
        return nullptr;
    }
}