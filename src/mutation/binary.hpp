/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_MUTATION_BINARY_HPP
#define GA_MUTATION_BINARY_HPP

#include "mutation_base.hpp"

/** Predefined mutation operators for the binary encoded genetic algorithm. */
namespace genetic_algorithm::mutation::binary
{
    /**
    * Standard flip mutation for the binary encoded genetic algorithm.
    * Each gene of the chromosome is flipped with the specified mutation probability.
    */
    class Flip final : public Mutation<char>
    {
    public:
        using Mutation::Mutation;
    private:
        void mutate(const GaInfo& ga, Candidate<char>& candidate) const override;
    };

} // namespace genetic_algorithm::mutation::binary

#endif // !GA_MUTATION_BINARY_HPP