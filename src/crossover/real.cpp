/*
*  MIT License
*
*  Copyright (c) 2021 Kriszti�n Rug�si
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#include "real.hpp"
#include "../utils.h"
#include "../rng.h"
#include "../mo_detail.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cassert>

namespace genetic_algorithm::crossover::real
{
    BLXa::BLXa(const std::vector<std::pair<double, double>>& bounds, double pc, double alpha) :
        BoundedCrossover(bounds, pc)
    {
        this->alpha(alpha);
    }

    void BLXa::alpha(double alpha)
    {
        if (!(0.0 <= alpha && alpha <= std::numeric_limits<double>::max()))
        {
            throw std::invalid_argument("Alpha must be a nonnegative, finite value.");
        }

        alpha_ = alpha;
    }


    SimulatedBinary::SimulatedBinary(const std::vector<std::pair<double, double>>& bounds, double pc, double eta) :
        BoundedCrossover(bounds, pc)
    {
        this->eta(eta);
    }

    void SimulatedBinary::eta(double eta)
    {
        if (!(0.0 <= eta && eta <= std::numeric_limits<double>::max()))
        {
            throw std::invalid_argument("Eta must be a nonnegative, finite value.");
        }

        eta_ = eta;
    }


    CandidatePair<double> Arithmetic::crossover(const GA<double>& ga, const Candidate<double>& parent1, const Candidate<double>& parent2) const
    {
        GA_UNUSED(ga);
        
        if (parent1.chromosome.size() != parent2.chromosome.size())
        {
            throw std::invalid_argument("The parent chromosomes must be the same length for the arithmetic crossover.");
        }

        Candidate child1{ parent1 }, child2{ parent2 };

        double alpha = rng::randomReal();
        for (size_t i = 0; i < parent1.chromosome.size(); i++)
        {
            child1.chromosome[i] = alpha * parent1.chromosome[i] + (1.0 - alpha) * parent2.chromosome[i];
            child2.chromosome[i] = (1.0 - alpha) * parent1.chromosome[i] + alpha * parent2.chromosome[i];
        }
        /* No bounds check, the generated children's genes will always be within the bounds if the parents' genes were also within them. */

        return { child1, child2 };
    }

    CandidatePair<double> BLXa::crossover(const GA<double>& ga, const Candidate<double>& parent1, const Candidate<double>& parent2) const
    {
        GA_UNUSED(ga);
        assert(alpha_ >= 0.0);

        if (parent1.chromosome.size() != parent2.chromosome.size())
        {
            throw std::invalid_argument("The parent chromosomes must be the same length for the BLXa crossover.");
        }
        if (parent1.chromosome.size() != bounds_.size())
        {
            throw std::invalid_argument("The chromosome and bounds vector lengths must be the same to perform the BLXa crossover.");
        }

        Candidate child1{ parent1 }, child2{ parent2 };

        for (size_t i = 0; i < parent1.chromosome.size(); i++)
        {
            /* Calc interval to generate the childrens genes on. */
            auto [range_min, range_max] = std::minmax(parent1.chromosome[i], parent2.chromosome[i]);
            double range_ext = alpha_ * (range_max - range_min);
            /* Generate genes from an uniform distribution on the interval. */
            child1.chromosome[i] = rng::randomReal(range_min - range_ext, range_max + range_ext);
            child2.chromosome[i] = rng::randomReal(range_min - range_ext, range_max + range_ext);
            /* The children's genes might be outside the allowed interval. */
            child1.chromosome[i] = std::clamp(child1.chromosome[i], bounds_[i].first, bounds_[i].second);
            child2.chromosome[i] = std::clamp(child2.chromosome[i], bounds_[i].first, bounds_[i].second);
        }

        return { child1, child2 };
    }

    CandidatePair<double> SimulatedBinary::crossover(const GA<double>& ga, const Candidate<double>& parent1, const Candidate<double>& parent2) const
    {
        GA_UNUSED(ga);
        assert(eta_ > 0.0);

        if (parent1.chromosome.size() != parent2.chromosome.size())
        {
            throw std::invalid_argument("The parent chromosomes must be the same length for the Simulated Binary crossover.");
        }
        if (parent1.chromosome.size() != bounds_.size())
        {
            throw std::invalid_argument("The chromosome and bounds vector lengths must be the same to perform the Simulated Binary crossover.");
        }

        Candidate child1{ parent1 }, child2{ parent2 };

        /* Generate beta from the appropriate distribution. */
        double u = rng::randomReal();
        double beta = (u <= 0.5) ? std::pow(2.0 * u, 1.0 / (eta_ + 1.0)) :
                                   std::pow(1.0 / (2.0 * (1.0 - u)), 1.0 / (eta_ + 1.0));

        /* Perform crossover. */
        for (size_t i = 0; i < parent1.chromosome.size(); i++)
        {
            child1.chromosome[i] = 0.5 * ((1 - beta) * parent1.chromosome[i] + (1 + beta) * parent2.chromosome[i]);
            child2.chromosome[i] = 0.5 * ((1 + beta) * parent1.chromosome[i] + (1 - beta) * parent2.chromosome[i]);
            /* The children's genes might be outside the allowed interval. */
            child1.chromosome[i] = std::clamp(child1.chromosome[i], bounds_[i].first, bounds_[i].second);
            child2.chromosome[i] = std::clamp(child2.chromosome[i], bounds_[i].first, bounds_[i].second);
        }

        return { child1, child2 };
    }

    CandidatePair<double> Wright::crossover(const GA<double>& ga, const Candidate<double>& parent1, const Candidate<double>& parent2) const
    {
        GA_UNUSED(ga);

        if (parent1.chromosome.size() != parent2.chromosome.size())
        {
            throw std::invalid_argument("The parent chromosomes must be the same length for the Wright crossover.");
        }
        if (parent1.chromosome.size() != bounds_.size())
        {
            throw std::invalid_argument("The chromosome and bounds vector lengths must be the same to perform the Wright crossover.");
        }

        Candidate child1(parent1), child2(parent2);

        /* p1 is always the better parent. */
        const Candidate<double>* p1 = detail::paretoCompare(parent1.fitness, parent2.fitness) ? &parent2 : &parent1;
        const Candidate<double>* p2 = detail::paretoCompare(parent1.fitness, parent2.fitness) ? &parent1 : &parent2;
        /* Get random weights. */
        double w1 = rng::randomReal();
        double w2 = rng::randomReal();
        /* Perform crossover. */
        for (size_t i = 0; i < p1->chromosome.size(); i++)
        {
            child1.chromosome[i] = w1 * (p1->chromosome[i] - p2->chromosome[i]) + p1->chromosome[i];
            child2.chromosome[i] = w2 * (p1->chromosome[i] - p2->chromosome[i]) + p1->chromosome[i];
            /* The children's genes might be outside the allowed intervals. */
            child1.chromosome[i] = std::clamp(child1.chromosome[i], bounds_[i].first, bounds_[i].second);
            child2.chromosome[i] = std::clamp(child2.chromosome[i], bounds_[i].first, bounds_[i].second);
        }

        return { child1, child2 };
    }

} // namespace genetic_algorithm::crossover::real