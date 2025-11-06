// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef PKIDISC_H
#define PKIDISC_H

#include "BinDisc.h"

namespace mdlp {
    class PKIDisc : public BinDisc {
    public:
        PKIDisc() = default;
        ~PKIDisc() = default;
        void fit(samples_t& X_, labels_t& y) override;
    };
}
#endif
