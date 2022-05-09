/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "selection_dtl.hpp"
#include "../utility/rng.hpp"
#include "../utility/math.hpp"
#include "../utility/utils.hpp"
#include "../utility/algorithm.hpp"
#include "../utility/functional.hpp"
#include <vector>
#include <utility>
#include <algorithm>
#include <limits>
#include <cmath>

namespace genetic_algorithm::selection::dtl
{
    std::vector<double> rouletteWeights(const FitnessMatrix& fmat)
    {
        auto fvec = detail::toFitnessVector(fmat);

        /* Roulette selection wouldn't work for negative fitness values. */
        double offset = *std::min_element(fvec.begin(), fvec.end());
        offset = 2.0 * offset;              /* The selection probability of the worst candidate should also be > 0. */
        offset = std::min(0.0, offset);     /* Only adjust fitness values if it's neccesary (has negative fitness). */

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        [offset](double f)
        {
            return f - offset;
        });

        return fvec;
    }

    std::vector<double> rankWeights(const FitnessMatrix& fmat, double wmin, double wmax)
    {
        assert(0.0 <= wmin && wmin <= wmax);

        const auto fvec = detail::toFitnessVector(fmat);
        const auto indices = detail::argsort(fvec.begin(), fvec.end());

        std::vector<double> weights(fmat.size());
        for (size_t i = 0; i < indices.size(); i++)
        {
            double t = i / (weights.size() - 1.0);
            weights[indices[i]] = std::lerp(wmin, wmax, t);
        }

        return weights;
    }

    std::vector<double> sigmaWeights(const FitnessMatrix& fmat, double scale)
    {
        assert(scale > 1.0);

        auto fvec = detail::toFitnessVector(fmat);
        double fmean = detail::mean(fvec);
        double fdev = std::max(detail::stdDev(fvec, fmean), 1E-6);

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        [fmean, fdev, scale](double f)
        {
            double weight = 1.0 + (f - fmean) / (scale * fdev);

            return std::max(weight, 0.0);  /* If ( fitness < (f_mean - scale * SD) ) the weight could be negative. */
        });

        return fvec;
    }

    std::vector<double> boltzmannWeights(const FitnessMatrix& fmat, double temperature)
    {
        auto fvec = detail::toFitnessVector(fmat);
        auto [fmin, fmax] = std::minmax_element(fvec.begin(), fvec.end());

        std::transform(fvec.begin(), fvec.end(), fvec.begin(),
        // dont try to capture the iterators by ref or value here
        [fmin = *fmin, fmax = *fmax, temperature](double f)
        {
            double df = std::max(fmax - fmin, 1E-6);
            double fnorm = (f - fmin) / df;

            return std::exp(fnorm / temperature);
        });

        return fvec;
    }

    double boltzmannDefaultTemp(size_t gen, size_t max_gen)
    {
        return -4.0 / (1.0 + std::exp(-10.0 * (double(gen) / max_gen) + 3.0)) + 4.0 + 0.25;
    }

    std::vector<double> weightsToCdf(const std::vector<double>& weights)
    {
        double wmean = detail::mean(weights);

        return detail::map(weights,
        [cdf = 0.0, wmean, n = weights.size()](double w) mutable
        {
            return cdf += w / wmean / n;
        });
    }

    ParetoFrontsInfo nonDominatedSort(const FitnessMatrix& fmat)
    {
        using namespace std;

        /* Find the number of candidates which dominate each candidate, and the indices of the candidates it dominates. */
        vector<size_t> better_sol_count(fmat.size(), 0);
        vector<vector<size_t>> dominated_idxs(fmat.size());

        for (size_t i = 0; i < fmat.size(); i++)
        {
            for (size_t j = 0; j < i; j++)
            {
                if (detail::paretoCompareLess(fmat[j], fmat[i]))
                {
                    better_sol_count[j]++;
                    dominated_idxs[i].push_back(j);
                }
                else if (detail::paretoCompareLess(fmat[i], fmat[j]))
                {
                    better_sol_count[i]++;
                    dominated_idxs[j].push_back(i);
                }
            }
        }
        vector<size_t> ranks(fmat.size());

        /* Find the indices of all non-dominated candidates (the first/best pareto front). */
        vector<size_t> current_front;
        for (size_t i = 0; i < fmat.size(); i++)
        {
            if (better_sol_count[i] == 0)
            {
                current_front.push_back(i);
                ranks[i] = 0;
            }
        }

        /* Find all the other pareto fronts. */
        vector<vector<size_t>> pareto_fronts;
        for (size_t rank = 1; !current_front.empty(); rank++)
        {
            /* Remove the current front from the population and find the next one. */
            vector<size_t> next_front;
            for (size_t sol_idx : current_front)
            {
                for (size_t dominated_idx : dominated_idxs[sol_idx])
                {
                    /* j belongs to the next front if it's domination count will become 0. */
                    if (--better_sol_count[dominated_idx] == 0)
                    {
                        next_front.push_back(dominated_idx);
                        ranks[dominated_idx] = rank;
                    }
                }
            }
            pareto_fronts.push_back(current_front);
            current_front = next_front;
        }

        return ParetoFrontsInfo(pareto_fronts, ranks);
    }

    std::vector<double> crowdingDistances(const FitnessMatrix& fmat, std::vector<std::vector<size_t>>& pfronts)
    {
        std::vector<double> distances(fmat.size(), 0.0);

        std::for_each(GA_EXECUTION_UNSEQ, pfronts.begin(), pfronts.end(),
        [&fmat, &distances](std::vector<size_t>& pfront)
        {
            /* Calculate the distances along each fitness component. */
            for (size_t d = 0; d < fmat[0].size(); d++)
            {
                std::sort(pfront.begin(), pfront.end(),
                [&fmat, d](size_t lidx, size_t ridx)
                {
                    return fmat[lidx][d] < fmat[ridx][d];
                });

                double finterval = fmat[pfront.back()][d] - fmat[pfront.front()][d];
                finterval = std::max(finterval, 1E-6);

                distances[pfront.front()] = distances[pfront.back()] = std::numeric_limits<double>::infinity();
                for (size_t i = 1; i < pfront.size() - 1; i++)
                {
                    size_t this_ = pfront[i];
                    size_t next = pfront[i + 1];
                    size_t prev = pfront[i - 1];

                    distances[this_] += (fmat[next][d] - fmat[prev][d]) / finterval;
                }
            }
        });

        return distances;
    }

    std::vector<double> crowdingDistances(const FitnessMatrix& fmat, std::vector<std::vector<size_t>>&& pfronts)
    {
        return crowdingDistances(fmat, pfronts);
    }

    std::vector<std::vector<double>> generateRefPoints(size_t n, size_t dim)
    {
        using namespace std;
        assert(n > 0);
        assert(dim > 1);

        /* Generate reference point candidates randomly. */
        size_t ratio = max(size_t{ 10 }, 2 * dim);
        vector<vector<double>> candidates(ratio * n - 1);
        generate(candidates.begin(), candidates.end(), [&dim]() { return rng::randomSimplexPoint(dim); });

        vector<vector<double>> refs;
        refs.reserve(n);
        refs.push_back(rng::randomSimplexPoint(dim));

        vector<double> min_distances(candidates.size(), numeric_limits<double>::infinity());
        while (refs.size() < n)
        {
            /* Calc the distance of each candidate to the closest ref point. */
            transform(GA_EXECUTION_UNSEQ, candidates.begin(), candidates.end(), min_distances.begin(), min_distances.begin(),
            [&refs](const vector<double>& candidate, double dmin)
            {
                double d = detail::euclideanDistanceSq(candidate, refs.back());
                return min(dmin, d);
            });

            /* Add the candidate with highest min_distance to the refs. */
            size_t argmax = detail::argmax(min_distances.begin(), min_distances.end());
            refs.push_back(move(candidates[argmax]));

            /* Remove the added candidate and the corresponding min_distance. */
            swap(candidates[argmax], candidates.back());
            candidates.pop_back();
            swap(min_distances[argmax], min_distances.back());
            min_distances.pop_back();
        }

        return refs;
    }

    std::pair<size_t, double> findClosestRef(const std::vector<std::vector<double>>& refs, const std::vector<double>& p)
    {
        auto distances = detail::map(refs,
        [&p](const auto& ref)
        {
            return detail::perpendicularDistanceSq(ref, p);
        });

        return detail::argmin_with_v(distances.begin(), distances.end());
    }

    double ASF(const std::vector<double>& f, const std::vector<double>& z, const std::vector<double>& w) noexcept
    {
        assert(!f.empty());
        assert(f.size() == z.size() && f.size() == w.size());

        double dmax = std::abs(f[0] - z[0]) / w[0];
        for (size_t j = 1; j < f.size(); j++)
        {
            dmax = std::max(dmax, std::abs(f[j] - z[j]) / w[j]);
        }

        return dmax;
    }

} // namespace genetic_algorithm::selection::dtl