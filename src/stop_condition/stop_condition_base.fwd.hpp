/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_STOP_CONDITION_BASE_FWD_HPP
#define GA_STOP_CONDITION_BASE_FWD_HPP

#include <concepts>

namespace genetic_algorithm::stopping
{
    class StopCondition;

    template<typename T>
    concept StopMethod = requires
    {
        requires std::derived_from<T, StopCondition>;
        requires std::copy_constructible<T>;
    };

} // namespace genetic_algorithm::stopping

#endif // !GA_STOP_CONDITION_BASE_FWD_HPP