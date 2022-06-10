/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_CORE_GA_BASE_DECL_HPP
#define GA_CORE_GA_BASE_DECL_HPP

#include "ga_info.hpp"
#include "../population/candidate.hpp"
#include "../mutation/mutation_base.fwd.hpp"
#include "../crossover/crossover_base.fwd.hpp"
#include <algorithm>
#include <vector>
#include <utility>
#include <functional>
#include <type_traits>
#include <atomic>
#include <cstddef>
#include <memory>
#include <concepts>

namespace genetic_algorithm
{
    /**
    * Base GA class.
    *
    * @tparam geneType The type of the genes in the candidates' chromosomes.
    */
    template<Gene T, typename Derived>
    class GA : public GaInfo
    {
    public:

        // static assert that Derived is actually derived from this, and implements generateCandidate()

        using GeneType = T;                             /**< The gene type used in the algorithm. */
        using Candidate = Candidate<GeneType>;          /**< The candidate type used in the algorithm. */
        using Chromosome = std::vector<GeneType>;       /**< The type of the chromosomes used in the algorithm. */
        using CandidatePair = CandidatePair<GeneType>;  /**< A pair of Candidates. */
        using Candidates = std::vector<Candidate>;      /**< A vector of Candidates. */
        using Population = std::vector<Candidate>;      /**< The population type used in the algorithm. */

        using FitnessFunction = std::function<std::vector<double>(const Chromosome&)>;      /**< The type of the fitness function. */
        using CrossoverFunction = std::function<CandidatePair(const GaInfo&, const Candidate&, const Candidate&)>;
        using MutationFunction = std::function<void(const GaInfo&, Candidate&)>;
        using RepairFunction = std::function<Chromosome(const Chromosome&)>;                /**< The type of the repair function. */
        using Callback = std::function<void(const GA<T, Derived>&)>;

        /**
        * Create a genetic algorithm.
        *
        * @param chrom_len The length of the chromosomes (number of genes).
        * @param fitness_function The fitness function to find the maximum of.
        */
        GA(size_t chrom_len, FitnessFunction fitness_function);

        /**
        * Create a genetic algorithm.
        *
        * @param population_size The number of candidates in the population.
        * @param chrom_len The length of the chromosomes (number of genes).
        * @param fitness_function The fitness function to find the maximum of.
        */
        GA(size_t population_size, size_t chrom_len, FitnessFunction fitness_function);

        /**
        * Run the genetic algorithm for the set number of generations. \n
        * The algorithm will always stop when reaching the maximum number of
        * generations have been reached.
        *
        * @param num_generations The maximum number of generations.
        * @returns The optimal solutions.
        */
        Candidates run(size_t num_generations = 500);

        /**
        * Continue running the genetic algorithm for the set number of generations. \n
        * Equivalent to calling run if the algorithm hasn't been run before, or if the selection
        * method was changed.
        *
        * @param num_generations The number of generations to run the algorithm for.
        * @returns The optimal solutions.
        */
        Candidates continueFor(size_t num_generations);

        /** @returns The pareto optimal solutions found by the algorithm. */
        [[nodiscard]]
        Candidates solutions() const noexcept;

        /** @returns The current population of the algorithm. Not the same as the solutions. */
        [[nodiscard]]
        Population population() const noexcept;

        /**
        * Set the initial population to be used in the algorithm to @p pop instead of randomly generating it. \n
        * If @p pop is empty, the initial population will be randomly generated. \n
        * If the preset population's size is not equal to the population size set, either additional randomly generated
        * Candidates will be added to fill out the initial population, or some Candidates will be discarded from the preset.
        *
        * @param pop The initial population to use in the algorithm.
        */
        //void initial_population(const Population& pop);

        /**
        * Set the fitness function used by the algorithm to @p f. \n
        * The fitness function should return a vector with a size equal to the number of objectives.
        *
        * @param f The fitness function to find the maximum of.
        */
        void fitness_function(FitnessFunction f);

        /**
        * Set the crossover method the algorithm will use to @p f.
        * 
        * @param f The crossover method for the algorithm.
        */
        template<crossover::CrossoverMethod<T> F>
        void crossover_method(const F& f);

        /**
        * Set the crossover method the algorithm will use to @p f.
        *
        * @param f The crossover method for the algorithm.
        */
        template<crossover::CrossoverMethod<T> F>
        void crossover_method(std::unique_ptr<F>&& f);

        /**
        * Set the crossover method the algorithm will use to @p f.
        *
        * @param f The crossover method for the algorithm.
        */
        void crossover_method(CrossoverFunction f);

        /** @returns The current crossover method used by the algorithm. */
        template<crossover::CrossoverMethod<T> F = crossover::Crossover<GeneType>>
        F& crossover_method() &;

        /**
        * Sets the crossover rate of the crossover operator used by the algorithm to @p pc.
        *
        * @param pc The crossover probability. Must be in the closed interval [0.0, 1.0].
        */
        void crossover_rate(double pc) final;

        /** @returns The current crossover rate set for the crossover operator. */
        [[nodiscard]]
        double crossover_rate() const noexcept final;

        /**
        * Set the mutation method the algorithm will use to @p f.
        *
        * @param f The crossover method for the algorithm.
        */
        template<mutation::MutationMethod<T> F>
        void mutation_method(const F& f);

        /**
        * Set the mutation method the algorithm will use to @p f.
        *
        * @param f The crossover method for the algorithm.
        */
        template<mutation::MutationMethod<T> F>
        void mutation_method(std::unique_ptr<F>&& f);

        /**
        * Set the mutation method the algorithm will use to @p f.
        *
        * @param f The crossover method for the algorithm.
        */
        void mutation_method(MutationFunction f);

        /** @returns The current mutation method used by the algorithm. */
        template<mutation::MutationMethod<T> F = mutation::Mutation<GeneType>>
        F& mutation_method() &;

        /**
        * Sets the mutation rate of the mutation operator used by the algorithm to @p pm.
        *
        * @param pm The mutation probability. Must be in the closed interval [0.0, 1.0].
        */
        void mutation_rate(double pm) final;

        /** @returns The current mutation rate set for the mutation operator. */
        [[nodiscard]]
        double mutation_rate() const noexcept final;

        /**
        * Set a repair function for the genetic algorithm. \n
        * This function will be applied to each chromosome after the mutations. \n
        * This can be used for example to perform a local search. \n
        * 
        * No repair function is used in the algorithm by default, set to nullptr if
        * no repair function should be used.
        * 
        * @param f The repair function the algorithm will use.
        */
        void repair_function(const RepairFunction& f);

        Callback endOfGenerationCallback = nullptr;

    protected:

        Population population_;
        Candidates solutions_;

        FitnessFunction fitness_function_;
        std::unique_ptr<crossover::Crossover<GeneType>> crossover_;
        std::unique_ptr<mutation::Mutation<GeneType>> mutation_;
        RepairFunction repair_ = nullptr;

        // reorder funcs
        void setDefaultAlgorithm();
        void initializeAlgorithm();
        Population generatePopulation(size_t pop_size) const;
        const Candidate& selectCandidate() const;
        CandidatePair crossover(const Candidate& parent1, const Candidate& parent2) const;
        void mutate(Candidate& sol) const;
        void repair(Candidate& sol) const;

        void evaluateSolution(Candidate); // for single solution, maybe?
        [[nodiscard]] FitnessMatrix evaluatePopulation(Population& pop);
        void updatePopulation(Population& pop, Population&& children);
        void updateOptimalSolutions(Candidates& optimal_sols, const Population& pop) const;
        bool stopCondition() const;
        void advance();
        
    private:

        Derived& derived() noexcept;
        const Derived& derived() const noexcept;
        Candidate generateCandidate() const;
        size_t findNumObjectives(const FitnessFunction& f) const;
    };

    /** Genetic algorithm types. */
    template<typename T>
    concept GeneticAlgorithm = detail::DerivedFromSpecializationOf<T, GA>;

} // namespace genetic_algorithm

#endif // !GA_CORE_GA_BASE_DECL_HPP