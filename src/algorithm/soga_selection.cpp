/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "soga_selection.hpp"
#include "../core/ga_info.hpp"
#include "../population/population.hpp"
#include "../utility/rng.hpp"
#include "../utility/algorithm.hpp"
#include "../utility/functional.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <cassert>

namespace genetic_algorithm::selection_
{
    /* Calculate the cumulative distribution function of the population from the selection weights. */
    static std::vector<double> weightsToCdf(const std::vector<double>& weights)
    {
        double wmean = detail::mean(weights);

        return detail::map(weights,
        [cdf = 0.0, wmean, n = weights.size()](double w) mutable
        {
            return cdf += w / wmean / n;
        });
    }

    void Roulette::prepareSelections(const GaInfo&, const FitnessMatrix& fmat)
    {
        auto fvec = detail::toFitnessVector(fmat.begin(), fmat.end());

        /* Roulette selection wouldn't work for negative fitness values. */
        double offset = *std::min_element(fvec.begin(), fvec.end());
        offset = 2.0 * offset;              /* The selection probability of the worst candidate should also be > 0. */
        offset = std::min(0.0, offset);     /* Only adjust fitness values if it's neccesary (there are negative fitness values). */

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        [offset](double f)
        {
            return f - offset;
        });

        cdf_ = weightsToCdf(fvec);
    }

    size_t Roulette::select(const GaInfo&, const FitnessMatrix&)
    {
        return rng::sampleCdf(cdf_);
    }

    Tournament::Tournament(size_t size)
    {
        this->size(size);
    }

    void Tournament::size(size_t size)
    {
        if (size < 2) throw std::invalid_argument("The tournament size must be at least 2.");

        tourney_size_ = size;
    }

    void Tournament::prepareSelections(const GaInfo&, const FitnessMatrix& fmat)
    {
        assert(fmat.size() >= tourney_size_);
        assert(std::none_of(fmat.begin(), fmat.end(), [](const FitnessVector& fvec) { return fvec.empty(); }));

        fvec_ = detail::map(fmat, [](const FitnessVector& fvec) noexcept { return fvec[0]; });
    }

    size_t Tournament::select(const GaInfo&, const FitnessMatrix&)
    {
        auto candidates = rng::sampleUnique(0_sz, fvec_.size(), tourney_size_);

        return *std::max_element(candidates.begin(), candidates.end(),
        [this](size_t lidx, size_t ridx) noexcept
        {
            return fvec_[lidx] < fvec_[ridx];
        });
    }

    Rank::Rank(double min_weight, double max_weight)
    {
        this->weights(min_weight, max_weight);
    }

    void Rank::min_weight(double min_weight)
    {
        this->weights(min_weight, max_weight_);
    }

    void Rank::max_weight(double max_weight)
    {
        this->weights(min_weight_, max_weight);
    }

    void Rank::weights(double min_weight, double max_weight)
    {
        if (!(0.0 <= min_weight && min_weight <= max_weight))
        {
            throw std::invalid_argument("The minimum weight must be in the closed interval [0.0, max_weight].");
        }
        if (!(min_weight <= max_weight && max_weight <= std::numeric_limits<double>::max()))
        {
            throw std::invalid_argument("The maximum weight must be in the closed interval [min_weight, DBL_MAX].");
        }

        min_weight_ = min_weight;
        max_weight_ = max_weight;
    }

    void Rank::prepareSelections(const GaInfo&, const FitnessMatrix& fmat)
    {
        assert(0.0 <= min_weight_ && min_weight_ <= max_weight_);

        auto fvec = detail::toFitnessVector(fmat.begin(), fmat.end());
        auto indices = detail::argsort(fvec.begin(), fvec.end());

        std::vector<double> weights(fmat.size());
        for (size_t i = 0; i < indices.size(); i++)
        {
            double t = i / (weights.size() - 1.0);
            weights[indices[i]] = std::lerp(min_weight_, max_weight_, t);
        }

        cdf_ = weightsToCdf(weights);
    }

    size_t Rank::select(const GaInfo&, const FitnessMatrix&)
    {
        return rng::sampleCdf(cdf_);
    }

    Sigma::Sigma(double scale)
    {
        this->scale(scale);
    }

    void Sigma::scale(double scale)
    {
        if (!(1.0 <= scale && scale <= std::numeric_limits<double>::max()))
        {
            throw std::invalid_argument("Scale must be in the closed interval [1.0, DBL_MAX].");
        }

        scale_ = scale;
    }

    void Sigma::prepareSelections(const GaInfo&, const FitnessMatrix& fmat)
    {
        assert(scale_ > 1.0);

        auto fvec = detail::toFitnessVector(fmat.begin(), fmat.end());
        double fmean = detail::mean(fvec);
        double fdev = std::max(detail::stdDev(fvec, fmean), 1E-6);

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        [this, fmean, fdev](double f)
        {
            double weight = 1.0 + (f - fmean) / (scale_ * fdev);

            return std::max(weight, 0.0);  /* If ( fitness < (f_mean - scale * SD) ) the weight could be negative. */
        });

        cdf_ = weightsToCdf(fvec);
    }

    size_t Sigma::select(const GaInfo&, const FitnessMatrix&)
    {
        return rng::sampleCdf(cdf_);
    }

    Boltzmann::Boltzmann(TemperatureFunction f)
        : temperature_(std::move(f))
    {
    }

    void Boltzmann::temperature_function(TemperatureFunction f)
    {
        if (!f) throw std::invalid_argument("The temperature function can't be a nullptr.");

        temperature_ = std::move(f);
    }

    void Boltzmann::prepareSelections(const GaInfo& ga, const FitnessMatrix& fmat)
    {
        auto fvec = detail::toFitnessVector(fmat.begin(), fmat.end());
        auto [fmin, fmax] = std::minmax_element(fvec.begin(), fvec.end());

        auto temperature = temperature_(ga.generation_cntr(), ga.max_gen());

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        // dont try to capture the iterators by ref or value here
        [fmin = *fmin, fmax = *fmax, temperature](double f) noexcept
        {
            double df = std::max(fmax - fmin, 1E-6);
            double fnorm = (f - fmin) / df;

            return std::exp(fnorm / temperature);
        });

        cdf_ = weightsToCdf(fvec);
    }

    size_t Boltzmann::select(const GaInfo&, const FitnessMatrix&)
    {
        return rng::sampleCdf(cdf_);
    }

    double Boltzmann::boltzmannDefaultTemp(size_t gen, size_t max_gen) noexcept
    {
        return -4.0 / (1.0 + std::exp(-10.0 * (double(gen) / max_gen) + 3.0)) + 4.0 + 0.25;
    }

} // namespace genetic_algorithm::selection