// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef MDLP_EXCEPTIONS_H
#define MDLP_EXCEPTIONS_H

#include <sstream>
#include <stdexcept>
#include <string>

namespace mdlp {

    /**
     * @brief Tag base identifying every exception this library throws
     *
     * Lets a caller catch anything from mdlp in one handler:
     * @code
     * try { disc.fit(X, y); }
     * catch (const mdlp::DiscretizerError& e) { log(e.message()); }
     * @endcode
     *
     * @note It deliberately does **not** derive from `std::exception`. Every
     *       concrete type below also derives from the `std::` exception it
     *       replaces, so code written against 2.x — `catch (const
     *       std::invalid_argument&)` and friends — keeps working unchanged. If
     *       this tag derived from `std::exception` as well, that base would be
     *       inherited twice and become ambiguous.
     *
     * @note Because of that, a handler typed on this tag cannot call `what()`.
     *       Use message(), which every concrete type forwards to `what()`.
     */
    class DiscretizerError {
    public:
        virtual ~DiscretizerError() = default;
        /** @brief Same text as what(), reachable through a tag-typed handler */
        virtual const char* message() const noexcept = 0;
    };

    namespace detail {
        /**
         * @brief Format a number for an error message
         *
         * `std::to_string` always emits six decimals for floating point, so a
         * rejected `proposed_cuts` of -1 would read "-1.000000". This does not.
         *
         * Written as a plain function rather than a template on purpose: lcov
         * cannot derive end lines for implicit template members declared in a
         * header, and fails the coverage step over it.
         */
        inline std::string str(double value)
        {
            std::ostringstream os;
            os << value;
            return os.str();
        }
    }

    // The five concrete types are spelled out rather than generated from a
    // template, for the same lcov reason noted above. Each pairs the library tag
    // with the std:: exception it replaces.

    /**
     * @brief A constructor or method parameter is outside its valid range
     * @note Also a `std::invalid_argument`.
     */
    class InvalidParameter : public std::invalid_argument, public DiscretizerError {
    public:
        explicit InvalidParameter(const std::string& what_arg) : std::invalid_argument(what_arg) {}
        ~InvalidParameter() override = default;
        const char* message() const noexcept override { return what(); }
    };

    /**
     * @brief Input data failed validation — empty, mismatched or malformed
     * @note Also a `std::invalid_argument`.
     */
    class ValidationError : public std::invalid_argument, public DiscretizerError {
    public:
        explicit ValidationError(const std::string& what_arg) : std::invalid_argument(what_arg) {}
        ~ValidationError() override = default;
        const char* message() const noexcept override { return what(); }
    };

    /**
     * @brief transform() was called before a successful fit()
     * @note Also a `std::runtime_error`.
     */
    class NotFittedError : public std::runtime_error, public DiscretizerError {
    public:
        explicit NotFittedError(const std::string& what_arg) : std::runtime_error(what_arg) {}
        ~NotFittedError() override = default;
        const char* message() const noexcept override { return what(); }
    };

    /**
     * @brief An index fell outside the array it addresses
     * @note Also a `std::out_of_range`.
     */
    class IndexError : public std::out_of_range, public DiscretizerError {
    public:
        explicit IndexError(const std::string& what_arg) : std::out_of_range(what_arg) {}
        ~IndexError() override = default;
        const char* message() const noexcept override { return what(); }
    };

    /**
     * @brief Unsigned arithmetic would have wrapped around
     * @note Also a `std::underflow_error`.
     */
    class UnderflowError : public std::underflow_error, public DiscretizerError {
    public:
        explicit UnderflowError(const std::string& what_arg) : std::underflow_error(what_arg) {}
        ~UnderflowError() override = default;
        const char* message() const noexcept override { return what(); }
    };
}
#endif
