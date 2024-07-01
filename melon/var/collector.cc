//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <map>
#include <turbo/flags/flag.h>
#include <melon/utility/memory/singleton_on_pthread_once.h>
#include <melon/utility/threading/platform_thread.h>
#include <melon/var/var.h>
#include <melon/var/collector.h>

TURBO_FLAG(int32_t, var_collector_max_pending_samples, 1000,
           "Destroy unprocessed samples when they're too many");

TURBO_FLAG(int32_t, var_collector_expected_per_second, 1000,
           "Expected number of samples to be collected per second");

namespace melon::var {
    
    // CAUTION: Don't change this value unless you know exactly what it means.
    static const int64_t COLLECTOR_GRAB_INTERVAL_US = 100000L; // 100ms

    MELON_CASSERT(!(COLLECTOR_SAMPLING_BASE & (COLLECTOR_SAMPLING_BASE - 1)),
                  must_be_power_of_2);

    // Combine two circular linked list into one.
    struct CombineCollected {
        void operator()(Collected *&s1, Collected *s2) const {
            if (s2 == nullptr) {
                return;
            }
            if (s1 == nullptr) {
                s1 = s2;
                return;
            }
            s1->InsertBeforeAsList(s2);
        }
    };

    // A thread and a special var to collect samples submitted.
    class Collector : public melon::var::Reducer<Collected *, CombineCollected> {
    public:
        Collector();

        ~Collector();

        int64_t last_active_cpuwide_us() const { return _last_active_cpuwide_us; }

        void wakeup_grab_thread();

    private:
        // The thread for collecting TLS submissions.
        void grab_thread();

        // The thread for calling user's callbacks.
        void dump_thread();

        // Adjust speed_limit if grab_thread collected too many in one round.
        void update_speed_limit(CollectorSpeedLimit *speed_limit,
                                size_t *last_ngrab, size_t cur_ngrab,
                                int64_t interval_us);

        static void *run_grab_thread(void *arg) {
            mutil::PlatformThread::SetName("var_collector_grabber");
            static_cast<Collector *>(arg)->grab_thread();
            return nullptr;
        }

        static void *run_dump_thread(void *arg) {
            mutil::PlatformThread::SetName("var_collector_dumper");
            static_cast<Collector *>(arg)->dump_thread();
            return nullptr;
        }

        static int64_t get_pending_count(void *arg) {
            Collector *d = static_cast<Collector *>(arg);
            return d->_ngrab - d->_ndump - d->_ndrop;
        }

    private:
        // periodically modified by grab_thread, accessed by every submit.
        // Make sure that this cacheline does not include frequently modified field.
        int64_t _last_active_cpuwide_us;

        bool _created;      // Mark validness of _grab_thread.
        bool _stop;         // Set to true in dtor.
        pthread_t _grab_thread;     // For joining.
        pthread_t _dump_thread;
        int64_t _ngrab MELON_CACHELINE_ALIGNMENT;
        int64_t _ndrop;
        int64_t _ndump;
        pthread_mutex_t _dump_thread_mutex;
        pthread_cond_t _dump_thread_cond;
        mutil::LinkNode<Collected> _dump_root;
        pthread_mutex_t _sleep_mutex;
        pthread_cond_t _sleep_cond;
    };

    Collector::Collector()
            : _last_active_cpuwide_us(mutil::cpuwide_time_us()), _created(false), _stop(false), _grab_thread(0),
              _dump_thread(0), _ngrab(0), _ndrop(0), _ndump(0) {
        pthread_mutex_init(&_dump_thread_mutex, nullptr);
        pthread_cond_init(&_dump_thread_cond, nullptr);
        pthread_mutex_init(&_sleep_mutex, nullptr);
        pthread_cond_init(&_sleep_cond, nullptr);
        int rc = pthread_create(&_grab_thread, nullptr, run_grab_thread, this);
        if (rc != 0) {
            LOG(ERROR) << "Fail to create Collector, " << berror(rc);
        } else {
            _created = true;
        }
    }

    Collector::~Collector() {
        if (_created) {
            _stop = true;
            pthread_join(_grab_thread, nullptr);
            _created = false;
        }
        pthread_mutex_destroy(&_dump_thread_mutex);
        pthread_cond_destroy(&_dump_thread_cond);
        pthread_mutex_destroy(&_sleep_mutex);
        pthread_cond_destroy(&_sleep_cond);
    }

    template<typename T>
    static T deref_value(void *arg) {
        return *(T *) arg;
    }

    // for limiting samples returning nullptr in speed_limit()
    static CollectorSpeedLimit g_null_speed_limit;

    void Collector::grab_thread() {
        _last_active_cpuwide_us = mutil::cpuwide_time_us();
        int64_t last_before_update_sl = _last_active_cpuwide_us;

        // This is the thread for collecting TLS submissions. User's callbacks are
        // called inside the separate _dump_thread to prevent a slow callback
        // (caused by busy disk generally) from blocking collecting code too long
        // that pending requests may explode memory.
        CHECK_EQ(0, pthread_create(&_dump_thread, nullptr, run_dump_thread, this));

        // vars
        melon::var::PassiveStatus<int64_t> pending_sampled_data(
                "var_collector_pending_samples", get_pending_count, this);
        double busy_seconds = 0;
        melon::var::PassiveStatus<double> busy_seconds_var(deref_value<double>, &busy_seconds);
        melon::var::PerSecond<melon::var::PassiveStatus<double> > busy_seconds_second(
                "var_collector_grab_thread_usage", &busy_seconds_var);

        melon::var::PassiveStatus<int64_t> ngrab_var(deref_value<int64_t>, &_ngrab);
        melon::var::PerSecond<melon::var::PassiveStatus<int64_t> > ngrab_second(
                "var_collector_grab_second", &ngrab_var);

        // Maps for calculating speed limit.
        typedef std::map<CollectorSpeedLimit *, size_t> GrapMap;
        GrapMap last_ngrab_map;
        GrapMap ngrab_map;
        // Map for group samples by preprocessors.
        typedef std::map<CollectorPreprocessor *, std::vector<Collected *> >
                PreprocessorMap;
        PreprocessorMap prep_map;

        // The main loop.
        while (!_stop) {
            const int64_t abstime = _last_active_cpuwide_us + COLLECTOR_GRAB_INTERVAL_US;

            // Clear and reuse vectors in prep_map, don't clear prep_map directly.
            for (PreprocessorMap::iterator it = prep_map.begin(); it != prep_map.end();
                 ++it) {
                it->second.clear();
            }

            // Collect TLS submissions and give them to dump_thread.
            mutil::LinkNode<Collected> *head = this->reset();
            if (head) {
                mutil::LinkNode<Collected> tmp_root;
                head->InsertBeforeAsList(&tmp_root);
                head = nullptr;

                // Group samples by preprocessors.
                for (mutil::LinkNode<Collected> *p = tmp_root.next(); p != &tmp_root;) {
                    mutil::LinkNode<Collected> *saved_next = p->next();
                    p->RemoveFromList();
                    CollectorPreprocessor *prep = p->value()->preprocessor();
                    prep_map[prep].push_back(p->value());
                    p = saved_next;
                }
                // Iterate prep_map
                mutil::LinkNode<Collected> root;
                for (PreprocessorMap::iterator it = prep_map.begin();
                     it != prep_map.end(); ++it) {
                    std::vector<Collected *> &list = it->second;
                    if (it->second.empty()) {
                        // don't call preprocessor when there's no samples.
                        continue;
                    }
                    if (it->first != nullptr) {
                        it->first->process(list);
                    }
                    for (size_t i = 0; i < list.size(); ++i) {
                        Collected *p = list[i];
                        CollectorSpeedLimit *speed_limit = p->speed_limit();
                        if (speed_limit == nullptr) {
                            ++ngrab_map[&g_null_speed_limit];
                        } else {
                            // Add up the samples of certain type.
                            ++ngrab_map[speed_limit];
                        }
                        // Drop samples if dump_thread is too busy.
                        // FIXME: equal probabilities to drop.
                        ++_ngrab;
                        if (_ngrab >= _ndrop + _ndump +
                                      turbo::get_flag(FLAGS_var_collector_max_pending_samples)) {
                            ++_ndrop;
                            p->destroy();
                        } else {
                            p->InsertBefore(&root);
                        }
                    }
                }
                // Give the samples to dump_thread
                if (root.next() != &root) {  // non empty
                    mutil::LinkNode<Collected> *head2 = root.next();
                    root.RemoveFromList();
                    MELON_SCOPED_LOCK(_dump_thread_mutex);
                    head2->InsertBeforeAsList(&_dump_root);
                    pthread_cond_signal(&_dump_thread_cond);
                }
            }
            int64_t now = mutil::cpuwide_time_us();
            int64_t interval = now - last_before_update_sl;
            last_before_update_sl = now;
            for (GrapMap::iterator it = ngrab_map.begin();
                 it != ngrab_map.end(); ++it) {
                update_speed_limit(it->first, &last_ngrab_map[it->first],
                                   it->second, interval);
            }

            now = mutil::cpuwide_time_us();
            // calcuate thread usage.
            busy_seconds += (now - _last_active_cpuwide_us) / 1000000.0;
            _last_active_cpuwide_us = now;

            // sleep for the next round.
            if (!_stop && abstime > now) {
                timespec abstimespec = mutil::microseconds_from_now(abstime - now);
                pthread_mutex_lock(&_sleep_mutex);
                pthread_cond_timedwait(&_sleep_cond, &_sleep_mutex, &abstimespec);
                pthread_mutex_unlock(&_sleep_mutex);
            }
            _last_active_cpuwide_us = mutil::cpuwide_time_us();
        }
        // make sure _stop is true, we may have other reasons to quit above loop
        {
            MELON_SCOPED_LOCK(_dump_thread_mutex);
            _stop = true;
            pthread_cond_signal(&_dump_thread_cond);
        }
        CHECK_EQ(0, pthread_join(_dump_thread, nullptr));
    }

    void Collector::wakeup_grab_thread() {
        pthread_mutex_lock(&_sleep_mutex);
        pthread_cond_signal(&_sleep_cond);
        pthread_mutex_unlock(&_sleep_mutex);
    }

// Adjust speed_limit to match collected samples per second
    void Collector::update_speed_limit(CollectorSpeedLimit *sl,
                                       size_t *last_ngrab, size_t cur_ngrab,
                                       int64_t interval_us) {
        // FIXME: May become too large at startup.
        const size_t round_ngrab = cur_ngrab - *last_ngrab;
        if (round_ngrab == 0) {
            return;
        }
        *last_ngrab = cur_ngrab;
        if (interval_us < 0) {
            interval_us = 0;
        }
        size_t new_sampling_range = 0;
        const size_t old_sampling_range = sl->sampling_range;
        if (!sl->ever_grabbed) {
            if (sl->first_sample_real_us) {
                interval_us = mutil::gettimeofday_us() - sl->first_sample_real_us;
                if (interval_us < 0) {
                    interval_us = 0;
                }
            } else {
                // Rare. the timestamp is still not set or visible yet. Just
                // use the default interval which may make the calculated
                // sampling_range larger.
            }
            new_sampling_range = turbo::get_flag(FLAGS_var_collector_expected_per_second)
                                 * interval_us * COLLECTOR_SAMPLING_BASE / (1000000L * round_ngrab);
        } else {
            // NOTE: the multiplications are unlikely to overflow.
            new_sampling_range = turbo::get_flag(FLAGS_var_collector_expected_per_second)
                                 * interval_us * old_sampling_range / (1000000L * round_ngrab);
            // Don't grow or shrink too fast.
            if (interval_us < 1000000L) {
                new_sampling_range =
                        (new_sampling_range * interval_us +
                         old_sampling_range * (1000000L - interval_us)) / 1000000L;
            }
        }
        // Make sure new value is sane.
        if (new_sampling_range == 0) {
            new_sampling_range = 1;
        } else if (new_sampling_range > COLLECTOR_SAMPLING_BASE) {
            new_sampling_range = COLLECTOR_SAMPLING_BASE;
        }

        // NOTE: don't update unmodified fields in sl to avoid meaningless
        // flushing of the cacheline.
        if (new_sampling_range != old_sampling_range) {
            sl->sampling_range = new_sampling_range;
        }
        if (!sl->ever_grabbed) {
            sl->ever_grabbed = true;
        }
    }

    size_t is_collectable_before_first_time_grabbed(CollectorSpeedLimit *sl) {
        if (!sl->ever_grabbed) {
            int before_add = sl->count_before_grabbed.fetch_add(
                    1, std::memory_order_relaxed);
            if (before_add == 0) {
                sl->first_sample_real_us = mutil::gettimeofday_us();
            } else if (before_add >= turbo::get_flag(FLAGS_var_collector_expected_per_second)) {
                mutil::get_leaky_singleton<Collector>()->wakeup_grab_thread();
            }
        }
        return sl->sampling_range;
    }

    // Call user's callbacks in this thread.
    void Collector::dump_thread() {
        int64_t last_ns = mutil::cpuwide_time_ns();

        // vars
        double busy_seconds = 0;
        melon::var::PassiveStatus<double> busy_seconds_var(deref_value<double>, &busy_seconds);
        melon::var::PerSecond<melon::var::PassiveStatus<double> > busy_seconds_second(
                "var_collector_dump_thread_usage", &busy_seconds_var);

        melon::var::PassiveStatus<int64_t> ndumped_var(deref_value<int64_t>, &_ndump);
        melon::var::PerSecond<melon::var::PassiveStatus<int64_t> > ndumped_second(
                "melon::var::collector_dump_second", &ndumped_var);

        mutil::LinkNode<Collected> root;
        size_t round = 0;

        // The main loop
        while (!_stop) {
            ++round;
            // Get new samples set by grab_thread.
            mutil::LinkNode<Collected> *newhead = nullptr;
            {
                MELON_SCOPED_LOCK(_dump_thread_mutex);
                while (!_stop && _dump_root.next() == &_dump_root) {
                    const int64_t now_ns = mutil::cpuwide_time_ns();
                    busy_seconds += (now_ns - last_ns) / 1000000000.0;
                    pthread_cond_wait(&_dump_thread_cond, &_dump_thread_mutex);
                    last_ns = mutil::cpuwide_time_ns();
                }
                if (_stop) {
                    break;
                }
                newhead = _dump_root.next();
                _dump_root.RemoveFromList();
            }
            CHECK(newhead != &_dump_root);
            newhead->InsertBeforeAsList(&root);

            // Call callbacks.
            for (mutil::LinkNode<Collected> *p = root.next(); !_stop && p != &root;) {
                // We remove p from the list, save next first.
                mutil::LinkNode<Collected> *saved_next = p->next();
                p->RemoveFromList();
                Collected *s = p->value();
                s->dump_and_destroy(round);
                ++_ndump;
                p = saved_next;
            }
        }
    }

    void Collected::submit(int64_t cpuwide_us) {
        Collector *d = mutil::get_leaky_singleton<Collector>();
        // Destroy the sample in-place if the grab_thread did not run for twice
        // of the normal interval. This also applies to the situation that
        // grab_thread aborts due to severe errors.
        // Collector::_last_active_cpuwide_us is periodically modified by grab_thread,
        // cache bouncing is tolerable.
        if (cpuwide_us < d->last_active_cpuwide_us() + COLLECTOR_GRAB_INTERVAL_US * 2) {
            *d << this;
        } else {
            destroy();
        }
    }

    static double get_sampling_ratio(void *arg) {
        return ((const CollectorSpeedLimit *) arg)->sampling_range /
               (double) COLLECTOR_SAMPLING_BASE;
    }

    DisplaySamplingRatio::DisplaySamplingRatio(const char *name,
                                               const CollectorSpeedLimit *sl)
            : _var(name, get_sampling_ratio, (void *) sl) {
    }

}  // namespace melon::var
