#pragma once

#include <Utils.h>
#include <concepts>

template <typename OutputType, typename InputType>
ALWAYS_INLINE bool is(InputType& input)
{
    static_assert(!std::same_as<OutputType, InputType>);
    return dynamic_cast<CopyConst<InputType, OutputType>*>(&input);
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE bool is(InputType* input)
{
    return input && is<OutputType>(*input);
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE bool is(Ref<InputType> input)
{
    return is<OutputType>(*input);
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>* as_if(InputType& input)
{
    if (!is<OutputType>(input))
        return nullptr;
    if constexpr (std::is_base_of_v<InputType, OutputType>)
    {
        return static_cast<CopyConst<InputType, OutputType>*>(&input);
    }
    else
    {
        return dynamic_cast<CopyConst<InputType, OutputType>*>(&input);
    }
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>* as_if(InputType* input)
{
    if (!input)
        return nullptr;
    return as_if<OutputType>(*input);
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>* as_if(Ref<InputType> input)
{
    if (!input)
        return nullptr;
    return as_if<OutputType>(*input);
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>& as(InputType& input)
{
    auto* result = as_if<OutputType>(input);
    assert(result);
    return *result;
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>* as(InputType* input)
{
    if (!input)
        return nullptr;
    auto* result = as_if<OutputType>(input);
    assert(result);
    return result;
}

template <typename OutputType, typename InputType>
ALWAYS_INLINE CopyConst<InputType, OutputType>* as(Ref<InputType> input)
{
    if (!input)
        return nullptr;
    auto* result = as_if<OutputType>(input);
    assert(result);
    return result;
}
