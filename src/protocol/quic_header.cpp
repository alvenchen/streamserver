#include "quic_header.hpp"

namespace quic{

    HeaderForm getHeaderForm(uint8_t headerValue){
        if (headerValue & kHeaderFormMask) {
            return HeaderForm::Long;
        }
        return HeaderForm::Short;
    }



}