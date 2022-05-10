/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "stop_condition_base.hpp"
#include <utility>

namespace genetic_algorithm::stopping::dtl
{
    Lambda::Lambda(StopConditionFunction f)
        : f_(std::move(f))
    {
    }

    bool Lambda::operator()(const GaInfo& ga)
    {
        return f_(ga);
    };

} // namespace genetic_algorithm::stopping::dtl