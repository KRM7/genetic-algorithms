/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "nd_sort.hpp"
#include "../utility/math.hpp"
#include <algorithm>
#include <iterator>
#include <vector>
#include <utility>
#include <cstddef>

namespace genetic_algorithm::algorithm::dtl
{
    ParetoFronts nonDominatedSort(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) >= 0);

        size_t pop_size = size_t(last - first);

        /* Find the number of candidates which dominate each candidate, and the indices of the candidates it dominates. */
        std::vector<size_t> better_count(pop_size, 0);
        std::vector<std::vector<size_t>> worse_indices(pop_size);

        std::for_each(worse_indices.begin(), worse_indices.end(), [&pop_size](std::vector<size_t>& vec) { vec.reserve(pop_size); });

        for (size_t lhs = 0; lhs < pop_size; lhs++)
        {
            for (size_t rhs = 1; rhs < lhs; rhs++)
            {
                auto comp = detail::paretoCompare(*(first + lhs), *(first + rhs));
                if (comp < 0)
                {
                    better_count[lhs]++;
                    worse_indices[rhs].push_back(lhs);
                }
                else if (comp > 0) [[likely]]
                {
                    better_count[rhs]++;
                    worse_indices[lhs].push_back(rhs);
                }
            }
        }

        /* [idx, rank] */
        ParetoFronts sorted_indices;
        sorted_indices.reserve(pop_size);

        /* Find the indices of all non-dominated candidates (the first/best pareto front). */
        for (size_t i = 0; i < pop_size; i++)
        {
            if (better_count[i] == 0)
            {
                sorted_indices.emplace_back(i, 0);
            }
        }

        /* Find all the other pareto fronts. */
        auto current_first = sorted_indices.cbegin();
        auto current_last = sorted_indices.cend();

        while (sorted_indices.size() != pop_size)
        {
            size_t next_rank = current_first->second + 1;

            /* Remove the current front from the population and find the next one. */
            for (; current_first != current_last; current_first++)
            {
                for (const auto& worse_idx : worse_indices[current_first->first])
                {
                    if (--better_count[worse_idx] == 0)
                    {
                        current_last--;
                        sorted_indices.emplace_back(worse_idx, next_rank);
                        current_last++;
                    }
                }
            }
            current_last = sorted_indices.cend();
        }

        return sorted_indices;
    }

    std::vector<size_t> paretoRanks(const ParetoFronts& pareto_fronts)
    {
        std::vector<size_t> ranks(pareto_fronts.size());

        for (const auto& [idx, rank] : pareto_fronts)
        {
            ranks[idx] = rank;
        }

        return ranks;
    }

    ParetoFronts::iterator nextFrontBegin(ParetoFronts::iterator current, ParetoFronts::iterator last) noexcept
    {
        return std::find_if(current, last,
        [current_rank = current->second](const std::pair<size_t, size_t>& elem) noexcept
        {
            size_t rank = elem.second;
            return rank != current_rank;
        });
    }

    auto paretoFrontBounds(ParetoFronts& pareto_fronts) -> std::vector<std::pair<ParetoFronts::iterator, ParetoFronts::iterator>>
    {
        using Iter = ParetoFronts::iterator;
        using IterPair = std::pair<Iter, Iter>;

        std::vector<IterPair> front_bounds;
        front_bounds.reserve(pareto_fronts.back().second);

        for (auto first = pareto_fronts.begin(); first != pareto_fronts.end(); )
        {
            auto last = nextFrontBegin(first, pareto_fronts.end());
            front_bounds.emplace_back(first, last);
            first = last;
        }

        return front_bounds;
    }

} // namespace genetic_algorithm::algorithm::dtl