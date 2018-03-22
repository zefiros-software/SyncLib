/**
 * @cond ___LICENSE___
 *
 * Copyright (c) 2016-2018 Zefiros Software.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @endcond
 */
#pragma once
#ifndef __SYNCLIB_TIMINGSCOLLECTOR_H__
#define __SYNCLIB_TIMINGSCOLLECTOR_H__

#include "nlohmann/json.hpp"

#include <armadillo>

#include <stdint.h>
#include <string>
#include <vector>

namespace SyncLib
{
    namespace Bench
    {

        class TimingsCollector
        {
        public:

            TimingsCollector(size_t p, size_t s, size_t maxCount);

            const std::vector<double> &GetCountTimings(size_t t, size_t count) const;

            void AddTiming(size_t t, size_t count, double timing);
            const size_t &GetP() const;
            const size_t &GetS() const;
            const size_t &GetMaxCount() const;

        private:

            std::vector<std::vector<std::vector<double>>> mTimings;
            size_t mP, mS, mMaxCount, mRotations;
        };

        class AggregatedTimings
        {
        public:

            AggregatedTimings(size_t p, size_t s, size_t maxCount)
                : mAggregated(arma::zeros(p, maxCount)),
                  mP(p),
                  mS(s),
                  mMaxCount(maxCount)
            {

            }

            void Aggregate(const TimingsCollector &timings)
            {
                for (size_t t = 0; t < timings.GetP(); ++t)
                {
                    if (t == timings.GetS())
                    {
                        continue;
                    }

                    for (size_t count = 0; count < timings.GetMaxCount(); ++count)
                    {
                        auto &vec = timings.GetCountTimings(t, count + 1);
                        mAggregated.at(t, count) = arma::mean(arma::vec(vec));
                    }
                }
            }

            arma::mat &GetBuffer()
            {
                return mAggregated;
            }

            const arma::mat &GetBuffer() const
            {
                return mAggregated;
            }

            const size_t &GetP() const
            {
                return mP;
            }

            const size_t &GetS() const
            {
                return mS;
            }

            const size_t &GetMaxCount() const
            {
                return mMaxCount;
            }

        private:

            arma::mat mAggregated;

            size_t mP, mS, mMaxCount;
        };
    }
}

namespace nlohmann
{
    template<>
    struct adl_serializer<arma::vec>
    {
        static ::arma::vec from_json(const json &j)
        {
            size_t size = j.size();
            ::arma::vec vect = ::arma::zeros(size);

            std::copy(j.begin(), j.end(), vect.begin());

            return vect;
        }

        static void to_json(json &j, const ::arma::vec &vect)
        {
            for (auto &entry : vect)
            {
                j.push_back(entry);
            }
        }
    };
    template<>
    struct adl_serializer<arma::rowvec>
    {
        static ::arma::rowvec from_json(const json &j)
        {
            size_t size = j.size();
            ::arma::rowvec vect = ::arma::zeros(size);

            std::copy(j.begin(), j.end(), vect.begin());

            return vect;
        }

        static void to_json(json &j, const ::arma::rowvec &vect)
        {
            for (auto &entry : vect)
            {
                j.push_back(entry);
            }
        }
    };

    template<>
    struct adl_serializer<arma::mat>
    {
        static ::arma::mat from_json(const json &j)
        {
            size_t rows = j.size();
            ::arma::mat matrix = ::arma::zeros(j.size(), j[0].size());

            for (size_t row = 0; row < rows; ++row)
            {
                ::arma::rowvec matRow = matrix.row(row);
                std::copy(j[row].begin(), j[row].end(), matRow.begin());
            }

            return matrix;
        }

        static void to_json(json &j, const ::arma::mat &matrix)
        {
            for (size_t row = 0, rows = matrix.n_rows; row < rows; ++row)
            {
                j.push_back(::arma::rowvec(matrix.row(row)));
            }
        }
    };

    template<>
    struct adl_serializer<SyncLib::Bench::AggregatedTimings>
    {
        static SyncLib::Bench::AggregatedTimings from_json(const json &j)
        {
            size_t s = j["source"].get<size_t>();
            json data = j["data"];
            size_t p = data.size();
            size_t maxCount = data[0].size();

            SyncLib::Bench::AggregatedTimings timings(p, s, maxCount);
            auto &buff = timings.GetBuffer();

            for (auto &entry : data)
            {
                size_t t = entry["target"].get<size_t>();

                arma::rowvec tTimings = buff.row(t);
                const auto &entryTimings = entry["timings"];
                std::copy(entryTimings.begin(), entryTimings.end(), tTimings.begin());
            }

            return timings;
        }

        static void to_json(json &j, const SyncLib::Bench::AggregatedTimings &timings)
        {
            size_t p = timings.GetP();
            size_t s = timings.GetS();
            j["source"] = timings.GetS();
            json data = json::array();
            const auto &buff = timings.GetBuffer();


            for (size_t t = 0; t < p; ++t)
            {
                if (t == s)
                {
                    continue;
                }

                json entry = json::object();
                entry["target"] = t;
                entry["timings"] = arma::rowvec(buff.row(t));

                data.emplace_back(entry);
            }

            j["data"] = data;
        }
    };
}

#endif