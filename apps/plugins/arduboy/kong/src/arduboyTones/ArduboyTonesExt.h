#pragma once

#include <ArduboyTones.h>

/* Upstream's ArduboyTonesExt is a copy of ArduboyTones with a setter for the
 * "sound enabled" callback; the layer's ArduboyTones takes it in its
 * constructor. */
class ArduboyTonesExt : public ArduboyTones {
  public:
    ArduboyTonesExt() : ArduboyTones(nullptr) { }
    void setOutputEnabled(bool (*outEn)()) { ArduboyTones tmp(outEn); }
};

