/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_GA_BASE_IMPL_HPP
#define GA_GA_BASE_IMPL_HPP

#include "ga_base.decl.hpp"
#include "../selection/selection_base.hpp"
#include "../crossover/crossover_base.hpp"
#include "../mutation/mutation_base.hpp"
#include "../stop_condition/stop_condition_base.hpp"
#include "../utility/rng.hpp"
#include "../utility/math.hpp"
#include "../population/population.hpp"
#include "../utility/utils.hpp"
#include <execution>
#include <numeric>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <cmath>

namespace genetic_algorithm
{
    template<Gene T, typename D>
    GA<T, D>::GA(size_t chrom_len, FitnessFunction fitness_function)
        : GaInfo(chrom_len)
    {
        if (!fitness_function)
        {
            throw std::invalid_argument("The fitness function is required for the GA.");
        }
        fitness_function_ = std::move(fitness_function);
    }

    template<Gene T, typename D>
    D& GA<T, D>::derived()
    {
        return static_cast<D&>(*this);
    }

    template<Gene T, typename D>
    const D& GA<T, D>::derived() const
    {
        return static_cast<const D&>(*this);
    }

    template<Gene T, typename D>
    auto GA<T, D>::solutions() const -> const Population&
    {
        return solutions_;
    }

    template<Gene T, typename D>
    auto GA<T, D>::population() const -> const Population&
    {
        return population_;
    }

    template<Gene T, typename D>
    std::vector<std::vector<double>> GA<T, D>::fitness_matrix() const
    {
        return detail::toFitnessMatrix(population_);
    }

    template<Gene T, typename D>
    void GA<T, D>::initial_population(const Population& pop)
    {
        if (!std::all_of(pop.begin(), pop.end(), [this](const Candidate& c) { return c.chromosome.size() == chrom_len_; }))
        {
            throw std::invalid_argument("The length of each chromosome in the preset pop must be equal to chrom_len.");
        }
        initial_population_ = pop;
    }

    template<Gene T, typename D>
    void GA<T, D>::fitness_function(FitnessFunction f)
    {
        if (!f)
        {
            throw std::invalid_argument("The fitness function is requires for the GA.");
        }
        fitness_function_ = std::move(f);

        num_objectives(getNumObjectives(fitness_function_));
    }

    template<Gene T, typename D>
    template<selection::SelectionMethod F>
    void GA<T, D>::selection_method(const F& f)
    {
        selection_ = std::make_unique<F>(f);
        can_continue_ = false;
    }

    template<Gene T, typename D>
    template<selection::SelectionMethod F>
    void GA<T, D>::selection_method(std::unique_ptr<F>&& f)
    {
        if (!f)
        {
            throw std::invalid_argument("The selection method can't be a nullptr.");
        }
        selection_ = std::move(f);
        can_continue_ = false;
    }

    template<Gene T, typename D>
    template<selection::SelectionMethod F>
    F& GA<T, D>::selection_method()
    {
        return dynamic_cast<F&>(*selection_);
    }

    template<Gene T, typename D>
    template<crossover::CrossoverMethod F>
    void GA<T, D>::crossover_method(const F& f)
    {
        crossover_ = std::make_unique<F>(f);
    }

    template<Gene T, typename D>
    template<crossover::CrossoverMethod F>
    void GA<T, D>::crossover_method(std::unique_ptr<F>&& f)
    {
        if (!f)
        {
            throw std::invalid_argument("The crossover method can't be a nullptr.");
        }
        crossover_ = std::move(f);
    }

    template<Gene T, typename D>
    template<crossover::CrossoverMethod F>
    F& GA<T, D>::crossover_method()
    {
        return dynamic_cast<F&>(*crossover_);
    }

    template<Gene T, typename D>
    template<mutation::MutationMethod F>
    void GA<T, D>::mutation_method(const F& f)
    {
        mutation_ = std::make_unique<F>(f);
    }

    template<Gene T, typename D>
    template<mutation::MutationMethod F>
    void GA<T, D>::mutation_method(std::unique_ptr<F>&& f)
    {
        if (!f)
        {
            throw std::invalid_argument("The mutation method can't be a nullptr.");
        }
        mutation_ = std::move(f);
    }

    template<Gene T, typename D>
    template<mutation::MutationMethod F>
    F& GA<T, D>::mutation_method()
    {
        return dynamic_cast<F&>(*mutation_);
    }

    template<Gene T, typename D>
    template<stopping::StopMethod F>
    void GA<T, D>::stop_condition(const F& f)
    {
        stop_condition_ = std::make_unique<F>(f);
    }

    template<Gene T, typename D>
    template<stopping::StopMethod F>
    void GA<T, D>::stop_condition(std::unique_ptr<F>&& f)
    {
        stop_condition_ = std::move(f);
    }

    template<Gene T, typename D>
    template<stopping::StopMethod F>
    F& GA<T, D>::stop_condition()
    {
        return dynamic_cast<F&>(*stop_condition_);
    }

    template<Gene T, typename D>
    void GA<T, D>::repair_function(const RepairFunction& f)
    {
        repair_ = f;
    }

    template<Gene T, typename D>
    void GA<T, D>::advance()
    {
        using namespace std;

        size_t num_children = population_size_ + population_size_ % 2;
        vector<CandidatePair> parent_pairs(num_children / 2);

        auto current_fmat = fitness_matrix();

        (*selection_).prepare(*this, current_fmat);
        if (archive_optimal_solutions)
        {
            updateOptimalSolutions(solutions_, population_);
        }

        /* Selections. */
        generate(GA_EXECUTION_UNSEQ, parent_pairs.begin(), parent_pairs.end(),
        [this, &current_fmat]() -> CandidatePair
        {
            return make_pair(population_[(*selection_).select(*this, current_fmat)],
                             population_[(*selection_).select(*this, current_fmat)]);
        });

        /* Crossovers. */
        for_each(GA_EXECUTION_UNSEQ, parent_pairs.begin(), parent_pairs.end(),
        [this](CandidatePair& p) -> void
        {
            p = (*crossover_)(*this, p.first, p.second);
        });

        vector<Candidate> children;
        children.reserve(num_children);
        for (size_t i = 0; i < parent_pairs.size(); i++)
        {
            children.push_back(move(parent_pairs[i].first));
            children.push_back(move(parent_pairs[i].second));
        }

        /* Mutations. */
        for_each(GA_EXECUTION_UNSEQ, children.begin(), children.end(),
        [this](Candidate& c) -> void
        {
            (*mutation_)(*this, c);
        });

        /* Apply repair function to the children if set. */
        repair(children);

        /* Overwrite the current population with the children. */
        evaluate(children);
        population_ = nextPopulation(population_, children);

        if (endOfGenerationCallback != nullptr) endOfGenerationCallback(*this);
        generation_cntr_++;
    }

    template<Gene T, typename D>
    auto GA<T, D>::run(size_t num_generations) -> const Candidates&
    {
        max_gen(num_generations);

        initialize();

        population_ = generateInitialPopulation();
        evaluate(population_);

        (*selection_).init(*this);
        while (!stopCondition())
        {
            advance();
        }
        updateOptimalSolutions(solutions_, population_);

        can_continue_ = true;

        return solutions_;
    }

    template<Gene T, typename D>
    auto GA<T, D>::continueFor(size_t num_generations) -> const Candidates&
    {
        if (!can_continue_) { run(num_generations); }

        max_gen(max_gen_ + num_generations);

        while (!stopCondition())
        {
            advance();
        }
        updateOptimalSolutions(solutions_, population_);

        return solutions_;
    }

    template<Gene T, typename D>
    void GA<T, D>::initialize()
    {
        num_objectives(getNumObjectives(fitness_function_));

        can_continue_ = false;
        generation_cntr_ = 0;
        num_fitness_evals_ = 0;
        solutions_.clear();
        population_.clear();
    }
    
    template<Gene T, typename D>
    size_t GA<T, D>::getNumObjectives(FitnessFunction& f) const
    {
        Candidate c = generateCandidate();
        return f(c.chromosome).size();
    }

    template<Gene T, typename D>
    auto GA<T, D>::generateCandidate() const -> Candidate
    {
        return derived().generateCandidate();
    }

    template<Gene T, typename D>
    auto GA<T, D>::generateInitialPopulation() const -> Population
    {
        assert(population_size_ > 0);

        if (!std::all_of(initial_population_.begin(), initial_population_.end(),
        [this](const Candidate& sol)
        {
            return sol.chromosome.size() == chrom_len_;
        }))
        {
            throw std::domain_error("The chromosome lengths in the preset initial population must be equal to the chrom_len set.");
        }

        Population pop;
        pop.reserve(population_size_);

        for (size_t i = 0; i < std::min(population_size_, initial_population_.size()); i++)
        {
            pop.push_back(initial_population_[i]);
        }
        while (pop.size() < population_size_)
        {
            pop.push_back(generateCandidate());
        }

        return pop;
    }

    template<Gene T, typename D>
    void GA<T, D>::evaluate(Population& pop)
    {
        assert(fitness_function_ != nullptr);

        std::for_each(GA_EXECUTION_UNSEQ, pop.begin(), pop.end(),
        [this](Candidate& sol)
        {
            if (dynamic_fitness || !sol.is_evaluated)
            {
                sol.fitness = fitness_function_(sol.chromosome);
                sol.is_evaluated = true;

                num_fitness_evals_++;
            }
        });

        for (const auto& sol : pop)
        {
            if (sol.fitness.size() != num_objectives_)
            {
                throw std::domain_error("A fitness vector returned by the fitness function has incorrect size.");
            }
            if (!std::all_of(sol.fitness.begin(), sol.fitness.end(), [](double val) { return std::isfinite(val); }))
            {
                throw std::domain_error("A non-finite fitness value was returned by the fitness function.");
            }
        }
    }

    template<Gene T, typename D>
    void GA<T, D>::updateOptimalSolutions(Candidates& optimal_sols, const Population& pop) const
    {
        assert(std::all_of(pop.begin(), pop.end(), [](const Candidate& sol) { return sol.is_evaluated; }));

        optimal_sols.insert(optimal_sols.end(), pop.begin(), pop.end());
        if (num_objectives_ == 1)
        {
            optimal_sols = detail::findParetoFront1D(optimal_sols);
        }
        else
        {
            optimal_sols = detail::findParetoFrontKung(optimal_sols);
        }

        /* Remove duplicate solutions. */
        std::sort(optimal_sols.begin(), optimal_sols.end());
        auto last = std::unique(optimal_sols.begin(), optimal_sols.end());
        optimal_sols.erase(last, optimal_sols.end());
    }

    template<Gene T, typename D>
    void GA<T, D>::repair(Population& pop) const
    {
        /* Don't do anything unless a repair function is specified. */
        if (repair_ == nullptr) return;

        std::for_each(GA_EXECUTION_UNSEQ, pop.begin(), pop.end(),
        [this](Candidate& sol)
        {
            Chromosome improved_chrom = repair_(sol.chromosome);
            if (improved_chrom != sol.chromosome)
            {
                sol.is_evaluated = false;
                sol.chromosome = std::move(improved_chrom);
            }
        });

        for (const auto& sol : pop)
        {
            if (sol.chromosome.size() != chrom_len_)
            {
                throw std::domain_error("The repair function must return chromosomes of chrom_len length.");
            }
        }
    }

    template<Gene T, typename D>
    auto GA<T, D>::nextPopulation(Population& pop, Population& children) const -> Population
    {
        pop.insert(pop.end(), std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()));

        auto fitness_matrix = detail::toFitnessMatrix(pop);
        auto selected_indices = (*selection_).nextPopulation(*this, fitness_matrix);

        return detail::map(selected_indices, [&pop](size_t idx) { return pop[idx]; });
    }

    template<Gene T, typename D>
    bool GA<T, D>::stopCondition() const
    {
        if (stop_condition_)
        {
            return (generation_cntr_ >= (max_gen_ - 1)) || (*stop_condition_)(*this);
        }
        else
        {
            return (generation_cntr_ >= (max_gen_ - 1));
        }
    }

} // namespace genetic_algorithm

#endif // !GA_GA_BASE_IMPL_HPP