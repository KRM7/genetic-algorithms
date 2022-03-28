/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_INTEGER_GA_HPP
#define GA_INTEGER_GA_HPP

#include "ga_base.hpp"
#include <cstddef>

namespace genetic_algorithm
{
    /**
    * Integer coded GA. \n
    * Same as @ref BinaryGA, but the genes of the chromosomes can be any integer on [0, base], not just 0 or 1. \n
    * It also uses a slightly different mutation function with swaps and inversions.
    */
    class IntegerGA : public GA<size_t>
    {
    public:
        /**
        * Basic contructor for the IntegerGA.
        *
        * @param chrom_len The number of genes in each chromosome.
        * @param fitness_function The fitness function used in the algorithm.
        * @param base The number of values a gene can take. Must be > 1. If 2, same as the @ref BinaryGA.
        */
        IntegerGA(size_t chrom_len, FitnessFunction fitnessFunction, size_t base);

        /**
        * Sets the number of values a gene can take to @p base. \n
        * The value of the base must be at least 2, and the GA is essentially the same as the
        * BinaryGA if the base is set to 2.
        *
        * @param base The number of values a gene can be.
        */
        void base(size_t base);
        [[nodiscard]] size_t base() const;

    private:
        size_t base_ = 4;

        Candidate generateCandidate() const override;
    };

} // namespace genetic_algorithm


/* IMPLEMENTATION */

#include "../utility/rng.hpp"
#include <stdexcept>
#include <cassert>

namespace genetic_algorithm
{
    inline IntegerGA::IntegerGA(size_t chrom_len, FitnessFunction fitnessFunction, size_t base)
        : GA(chrom_len, fitnessFunction), base_(base)
    {
        if (base < 2) throw std::invalid_argument("The base must be at least 2.");
    }

    inline void IntegerGA::base(size_t base)
    {
        if (base < 2) throw std::invalid_argument("The base must be at least 2.");

        base_ = base;
    }

    inline size_t IntegerGA::base() const
    {
        return base_;
    }

    inline IntegerGA::Candidate IntegerGA::generateCandidate() const
    {
        assert(chrom_len_ > 0);
        assert(base_ > 1);

        Candidate sol;
        sol.chromosome.reserve(chrom_len_);
        for (size_t i = 0; i < chrom_len_; i++)
        {
            sol.chromosome.push_back(rng::randomInt(size_t{ 0 }, base_ - 1));
        }

        return sol;
    }

} // namespace genetic_algorithm

#endif // !GA_INTEGER_GA_HPP