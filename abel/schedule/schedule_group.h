//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_SCHEDULE_SCHEDULE_GROUP_H_
#define ABEL_SCHEDULE_SCHEDULE_GROUP_H_

#include <abel/asl/string_view.h>

namespace abel {

    ABEL_CONSTEXPR unsigned max_schedule_groups() { return 16; }


    template<typename... T>
    class future;

    class reactor;

    class schedule_group;

    namespace schedule_internal {
        // Returns an index between 0 and max_schedule_groups()
        /**
         *
         * @param sg
         * @return an index between 0 and max_schedule_groups()
         */
        unsigned schedule_group_index(schedule_group sg);

        /**
         *
         * @param index
         * @return
         */
        schedule_group schedule_group_from_index(unsigned index);

    } //namespace schedule_internal

    /**
     *
     * @param name A name that identifiers the group; will be used as a label
     *             in the group's metrics
     * @param shares number of shares of the CPU time allotted to the group;
     *               Use numbers in the 1-1000 range (but can go above).
     * @return a scheduling group that can be used on any shard
     */
    future<schedule_group> create_schedule_group(abel::string_view name, float shares);

    /**
     * Identifies function calls that are accounted as a group
     * a tag that can be used to mark a function call.
     * Executions of such tagged calls are accounted as a group.
     */
    class schedule_group {
    public:
        ABEL_CONSTEXPR_MEMBER schedule_group() ABEL_NOEXCEPT : _id(0) {}

        bool active() const;

        const abel::string_view name() const;

        bool operator==(schedule_group x) const { return _id == x._id; }

        bool operator!=(schedule_group x) const { return _id != x._id; }

        bool is_main() const { return _id == 0; }

        void set_shares(float shares);

        friend future<schedule_group> create_scheduling_group(sstring name, float shares);

        friend class reactor;

        friend unsigned schedule_internal::schedule_group_index(schedule_group sg);

        friend schedule_group schedule_internal::schedule_group_from_index(unsigned index);

    private:
        explicit schedule_group(unsigned i) : _id(i) {}

    private:
        unsigned _id;
    };

    namespace schedule_internal {

        ABEL_FORCE_INLINE unsigned schedule_group_index(schedule_group sg) {
            return sg._id;
        }

        ABEL_FORCE_INLINE schedule_group schedule_group_from_index(unsigned index) {
            return schedule_group(index);
        }

        ABEL_FORCE_INLINE schedule_group* current_scheduling_group_ptr() {
            // Slow unless constructor is constexpr
            static ABEL_THREAD_STACK_LOCAL scheduling_group sg;
            return &sg;
        }

    } //namespace schedule_internal

    ABEL_FORCE_INLINE schedule_group current_schedule_group() {
        return *schedule_internal::current_schedule_group_ptr();
    }

    ABEL_FORCE_INLINE schedule_group default_schedule_group() {
        return schedule_group();
    }

    ABEL_FORCE_INLINE bool schedule_group::active() const {
        return *this == current_schedule_group();
    }

} //namespace abel

#endif //ABEL_SCHEDULE_SCHEDULE_GROUP_H_
