#pragma once

#include <cassert>
#include <format>
#include <map>
#include <string>

enum class BinaryOperation
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Equals,
    NotEqual,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,

    Assignment,

    _BinaryOperationCount
};

extern std::map<BinaryOperation, int> BinaryOperationPrecedence;

template <>
struct std::formatter<BinaryOperation>
{
    constexpr auto parse(std::format_parse_context& ctx) { return std::cbegin(ctx); }

    auto format(const BinaryOperation& obj, std::format_context& ctx) const
    {
        static_assert(
            static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11,
            "Not all binary operations are handled in BinaryOperation formatter");
        std::string str = "???";

        switch (obj)
        {
            using enum BinaryOperation;
            case Add:                str = "Add (`+`)"; break;
            case Subtract:           str = "Subtract (`-`)"; break;
            case Multiply:           str = "Multiply (`*`)"; break;
            case Divide:             str = "Divide (`/`)"; break;
            case Equals:             str = "Equals (`==`)"; break;
            case NotEqual:           str = "NotEqual (`!=`)"; break;
            case GreaterThan:        str = "GreaterThan (`>`)"; break;
            case GreaterThanOrEqual: str = "GreaterThanOrEqual (`>=`)"; break;
            case LessThan:           str = "LessThan (`<`)"; break;
            case LessThanOrEqual:    str = "LessThanOrEqual (`<=`)"; break;
            case Assignment:         str = "Assignment (`=`)"; break;
            default:                 str = "???"; break;
        }

        return std::format_to(ctx.out(), "{}", str);
    }
};
