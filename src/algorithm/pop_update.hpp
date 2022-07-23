/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_ALGORITHM_POP_UPDATE_HPP
#define GA_ALGORITHM_POP_UPDATE_HPP

#include "../population/population.hpp"
#include "../core/ga_info.hpp"
#include <concepts>
#include <type_traits>
#include <vector>
#include <cstddef>

/** Predefined methods used to update the populations in each generation of the algorithms. */
namespace genetic_algorithm::pop_update
{
    using detail::FitnessVector;
    using detail::FitnessMatrix;

    /**
     * Concept specifying the interface required for the population update methods. \n
     * 
     * The method should be callable with a const& to a GaInfo object, and the FitnessMatrix
     * of the combined parent and child populations specified by 3 iterators:
     * [parents_first, ... , children_first, ... , children_last),
     * and return the indices of the candidates selected for the next generation's population,
     * assuming that parents_first points to the idx 0.
     */
    template<typename T>
    concept Updater = requires(T updater, const GaInfo& ga, FitnessMatrix::const_iterator it)
    {
        requires std::copyable<std::decay_t<T>>;
        requires std::destructible<std::decay_t<T>>;

        { updater(ga, it, it, it) } -> std::same_as<std::vector<size_t>>;
    };

    /**
    * A population update method that selects only the child Candidates from the
    * combined parent and child populations, and uses these as the population of
    * the next generation of the algorithm. \n
    * If the number of children is greater than the population size used in the algorithm,
    * only the first pop_size children will be selected.
    */
    class KeepChildren
    {
    public:
        /**
        * Take a FitnessMatrix of the combined parent and child solutions specified by
        * the range [first, last), and return the indices of the fitness vectors that
        * belong to the children. \n
        * 
        * In the range [first, last), the [first, children_first) range should be the fitness matrix
        * of the parent population, while the range [children_first, last) should be the fitness matrix
        * of the child population. \n
        * 
        * For the returned indices, it is assumed that the index of @p first is 0.
        * 
        * @param ga The genetic algorithm this operator is used in.
        * @param first Iterator pointing to the fitness vector of the first parent.
        * @param children_first Iterator pointing to the fitness vector of the first child.
        * @param last Iterator pointing to the last element of the fitness matrix.
        * @returns The indices of the Candidates that will make up the next generation of the algorithm.
        */
        std::vector<size_t> operator()(const GaInfo& ga,
                                       FitnessMatrix::const_iterator first,
                                       FitnessMatrix::const_iterator children_first,
                                       FitnessMatrix::const_iterator last);
    };

    /**
    * A population update method that selects the candidates of the next generation
    * using elitism. \n
    * Of the combined parent and child populations, the N best candidates of the parent
    * population are carried over to the next population, while the remaining
    * (pop_size - N) slots are filled by the child solutions. \n
    * If N = 0, this is equivalent to only keeping the children for the next
    * generation (KeepChildren).
    */
    class Elitism
    {
    public:
        /**
        * Create an elitist population update operator.
        * 
        * @param n The number of the best parents that will be carried over to the next
        *          generation of the algorithm.
        */
        Elitism(size_t n = 1);

        /**
        * Set the number of elites used to @p n.
        * 
        * @param n The number of the best parents that will be carried over to the next
        *          generation of the algorithm.
        */
        void elite_num(size_t n);

        /** @returns The number of elites used. */
        [[nodiscard]]
        size_t elite_num() noexcept { return n_; }

        /**
        * Take a FitnessMatrix of the combined parent and child solutions specified by
        * the range [first, last), and return the indices of the fitness vectors that
        * are going to make up the population of the next generation of the algorithm. \n
        *
        * In the range [first, last), the [first, children_first) range should be the fitness matrix
        * of the parent population, while the range [children_first, last) should be the fitness matrix
        * of the child population. \n
        *
        * For the returned indices, it is assumed that the index of @p first is 0.
        *
        * @param ga The genetic algorithm this operator is used in.
        * @param first Iterator pointing to the fitness vector of the first parent.
        * @param children_first Iterator pointing to the fitness vector of the first child.
        * @param last Iterator pointing to the last element of the fitness matrix.
        * @returns The indices of the Candidates that will make up the next generation of the algorithm.
        */
        std::vector<size_t> operator()(const GaInfo& ga,
                                       FitnessMatrix::const_iterator parents_first,
                                       FitnessMatrix::const_iterator children_first,
                                       FitnessMatrix::const_iterator children_last);

    private:
        size_t n_;
    };

    /**
    * A population update method that selects the best (pop_size) candidates of the combined
    * parent and child populations (assuming fitness maximization), and uses these as the
    * candidates of the next generation of the algorithm.
    */
    struct KeepBest
    {
    public:
        /**
        * Take a FitnessMatrix of the combined parent and child solutions specified by
        * the range [first, last), and return the indices of the fitness vectors that
        * are going to make up the population of the next generation of the algorithm. \n
        *
        * In the range [first, last), the [first, children_first) range should be the fitness matrix
        * of the parent population, while the range [children_first, last) should be the fitness matrix
        * of the child population. \n
        *
        * For the returned indices, it is assumed that the index of @p first is 0.
        *
        * @param ga The genetic algorithm this operator is used in.
        * @param first Iterator pointing to the fitness vector of the first parent.
        * @param children_first Iterator pointing to the fitness vector of the first child.
        * @param last Iterator pointing to the last element of the fitness matrix.
        * @returns The indices of the Candidates that will make up the next generation of the algorithm.
        */
        std::vector<size_t> operator()(const GaInfo& ga,
                                       FitnessMatrix::const_iterator parents_first,
                                       FitnessMatrix::const_iterator children_first,
                                       FitnessMatrix::const_iterator children_last);
    };

    static_assert(Updater<KeepChildren>);
    static_assert(Updater<Elitism>);
    static_assert(Updater<KeepBest>);

} // namespace genetic_algorithm::pop_update


#endif // !GA_ALGORITHM_POP_UPDATE_HPP