// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// The exception hierarchy's contract, pinned.
//
// The point of the design is that it adds a way to catch library errors WITHOUT
// taking away the old one: every type derives both from the tag and from the
// `std::` exception it replaces, so code written against 2.x keeps compiling and
// keeps catching. These tests fail if either half of that is broken.

#include <string>
#include <stdexcept>
#include "gtest/gtest.h"
#include "Exceptions.h"
#include "CPPFImdlp.h"
#include "BinDisc.h"

namespace mdlp {

    // ---- backward compatibility: the std:: bases -------------------------- //

    TEST(Exceptions, AreCaughtByTheStdTypeTheyReplace)
    {
        EXPECT_THROW(throw InvalidParameter("x"), std::invalid_argument);
        EXPECT_THROW(throw ValidationError("x"), std::invalid_argument);
        EXPECT_THROW(throw NotFittedError("x"), std::runtime_error);
        EXPECT_THROW(throw IndexError("x"), std::out_of_range);
        EXPECT_THROW(throw UnderflowError("x"), std::underflow_error);
    }

    TEST(Exceptions, AreCaughtByStdException)
    {
        EXPECT_THROW(throw InvalidParameter("x"), std::exception);
        EXPECT_THROW(throw NotFittedError("x"), std::exception);
        EXPECT_THROW(throw IndexError("x"), std::exception);
    }

    // ---- the new capability: one handler for everything mdlp throws ------- //

    TEST(Exceptions, AreCaughtByTheLibraryTag)
    {
        EXPECT_THROW(throw InvalidParameter("x"), DiscretizerError);
        EXPECT_THROW(throw ValidationError("x"), DiscretizerError);
        EXPECT_THROW(throw NotFittedError("x"), DiscretizerError);
        EXPECT_THROW(throw IndexError("x"), DiscretizerError);
        EXPECT_THROW(throw UnderflowError("x"), DiscretizerError);
    }

    // A tag-typed handler cannot call what(), because the tag deliberately does
    // not derive from std::exception — that is what keeps the std:: base
    // unambiguous. message() is the way through.
    TEST(Exceptions, MessageReadsTheTextThroughTheTag)
    {
        try {
            throw ValidationError("something specific went wrong");
        }
        catch (const DiscretizerError& e) {
            EXPECT_STREQ("something specific went wrong", e.message());
        }
    }

    // Every type, not just a sample: message() is a separate override on each,
    // so covering three of five would leave two silently unverified.
    TEST(Exceptions, MessageAndWhatAgreeForEveryType)
    {
        const InvalidParameter a("text a");
        const ValidationError b("text b");
        const NotFittedError c("text c");
        const IndexError d("text d");
        const UnderflowError e("text e");
        EXPECT_STREQ(a.what(), a.message());
        EXPECT_STREQ(b.what(), b.message());
        EXPECT_STREQ(c.what(), c.message());
        EXPECT_STREQ(d.what(), d.message());
        EXPECT_STREQ(e.what(), e.message());

        // ...and each is reachable through the tag.
        const DiscretizerError* tags[] = { &a, &b, &c, &d, &e };
        const char* expected[] = { "text a", "text b", "text c", "text d", "text e" };
        for (size_t i = 0; i < 5; ++i) {
            EXPECT_STREQ(expected[i], tags[i]->message());
        }
    }

    // ---- the real throw sites map to the intended types ------------------- //

    TEST(Exceptions, ConstructorRejectionIsAnInvalidParameter)
    {
        EXPECT_THROW(CPPFImdlp(2, 10, 0), InvalidParameter);
        EXPECT_THROW(CPPFImdlp(3, 0, 0), InvalidParameter);
        EXPECT_THROW(CPPFImdlp(3, 10, -1), InvalidParameter);
        EXPECT_THROW(BinDisc(2), InvalidParameter);
    }

    TEST(Exceptions, BadInputDataIsAValidationError)
    {
        CPPFImdlp disc;
        samples_t empty_X;
        labels_t empty_y;
        EXPECT_THROW(disc.fit(empty_X, empty_y), ValidationError);

        samples_t X = { 1.0f, 2.0f, 3.0f };
        labels_t y_short = { 1, 2 };
        EXPECT_THROW(disc.fit(X, y_short), ValidationError);

        BinDisc bins(3);
        samples_t too_small = { 1.0f, 2.0f };
        labels_t labels = { 0, 1 };
        EXPECT_THROW(bins.fit(too_small, labels), ValidationError);
    }

    TEST(Exceptions, TransformBeforeFitIsANotFittedError)
    {
        BinDisc disc(3);
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f };
        EXPECT_THROW(disc.transform(X), NotFittedError);

        labels_t out;
        EXPECT_THROW(disc.transform(X, out), NotFittedError);
    }

    // ---- messages carry the offending value ------------------------------- //

    TEST(Exceptions, MessagesNameTheParameterAndItsValue)
    {
        try {
            CPPFImdlp(2, 10, 0);
            FAIL() << "expected InvalidParameter";
        }
        catch (const InvalidParameter& e) {
            const std::string what = e.what();
            EXPECT_NE(std::string::npos, what.find("min_length")) << what;
            EXPECT_NE(std::string::npos, what.find("2")) << what;
        }

        try {
            BinDisc bins(3);
            samples_t too_small = { 1.0f, 2.0f };
            labels_t labels = { 0, 1 };
            bins.fit(too_small, labels);
            FAIL() << "expected ValidationError";
        }
        catch (const ValidationError& e) {
            const std::string what = e.what();
            EXPECT_NE(std::string::npos, what.find("n_bins")) << what;
            EXPECT_NE(std::string::npos, what.find("(2)")) << what;
            EXPECT_NE(std::string::npos, what.find("(3)")) << what;
        }
    }

    // Floating point values must not read as "-1.000000"; std::to_string would.
    TEST(Exceptions, FloatingPointValuesAreFormattedReadably)
    {
        try {
            CPPFImdlp(3, 10, -1.0f);
            FAIL() << "expected InvalidParameter";
        }
        catch (const InvalidParameter& e) {
            const std::string what = e.what();
            EXPECT_NE(std::string::npos, what.find("-1")) << what;
            EXPECT_EQ(std::string::npos, what.find("-1.000000")) << what;
        }
    }

    // ---- the documented usage pattern actually compiles -------------------- //

    TEST(Exceptions, OneHandlerCatchesEverythingTheLibraryThrows)
    {
        int caught = 0;
        const auto attempt = [&caught](const std::function<void()>& body) {
            try {
                body();
            }
            catch (const DiscretizerError& e) {
                EXPECT_NE(nullptr, e.message());
                ++caught;
            }
            };

        attempt([] { CPPFImdlp(2, 10, 0); });
        attempt([] {
            CPPFImdlp disc;
            samples_t X;
            labels_t y;
            disc.fit(X, y);
            });
        attempt([] {
            BinDisc disc(3);
            samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f };
            disc.transform(X);
            });
        EXPECT_EQ(3, caught);
    }
}
