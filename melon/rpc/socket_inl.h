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


// This file contains inlined implementation of socket.h

#ifndef MELON_RPC_SOCKET_INL_H_
#define MELON_RPC_SOCKET_INL_H_


namespace melon {

// Utility functions to combine and extract SocketId.
MUTIL_FORCE_INLINE SocketId
MakeSocketId(uint32_t version, mutil::ResourceId<Socket> slot) {
    return SocketId((((uint64_t)version) << 32) | slot.value);
}

MUTIL_FORCE_INLINE mutil::ResourceId<Socket> SlotOfSocketId(SocketId sid) {
    mutil::ResourceId<Socket> id = { (sid & 0xFFFFFFFFul) };
    return id;
}

MUTIL_FORCE_INLINE uint32_t VersionOfSocketId(SocketId sid) {
    return (uint32_t)(sid >> 32);
}

// Utility functions to combine and extract Socket::_versioned_ref
MUTIL_FORCE_INLINE uint32_t VersionOfVRef(uint64_t vref) {
    return (uint32_t)(vref >> 32);
}

MUTIL_FORCE_INLINE int32_t NRefOfVRef(uint64_t vref) {
    return (int32_t)(vref & 0xFFFFFFFFul);
}

MUTIL_FORCE_INLINE uint64_t MakeVRef(uint32_t version, int32_t nref) {
    // 1: Intended conversion to uint32_t, nref=-1 is 00000000FFFFFFFF
    return (((uint64_t)version) << 32) | (uint32_t/*1*/)nref;
}

inline SocketOptions::SocketOptions()
    : fd(-1)
    , user(NULL)
    , on_edge_triggered_events(NULL)
    , health_check_interval_s(-1)
    , force_ssl(false)
    , use_rdma(false)
    , keytable_pool(NULL)
    , conn(NULL)
    , app_connect(NULL)
    , initial_parsing_context(NULL)
    , fiber_tag(FIBER_TAG_DEFAULT)
{}

inline int Socket::Dereference() {
    const SocketId id = _this_id;
    const uint64_t vref = _versioned_ref.fetch_sub(
        1, std::memory_order_release);
    const int32_t nref = NRefOfVRef(vref);
    if (nref > 1) {
        return 0;
    }
    if (__builtin_expect(nref == 1, 1)) {
        const uint32_t ver = VersionOfVRef(vref);
        const uint32_t id_ver = VersionOfSocketId(id);
        // Besides first successful SetFailed() adds 1 to version, one of
        // those dereferencing nref from 1->0 adds another 1 to version.
        // Notice "one of those": The wait-free Address() may make ref of a
        // version-unmatched slot change from 1 to 0 for mutiple times, we
        // have to use version as a guard variable to prevent returning the
        // Socket to pool more than once.
        //
        // Note: `ver == id_ver' means this socket has been `SetRecycle'
        // before rather than `SetFailed'; `ver == ide_ver+1' means we
        // had `SetFailed' this socket before. We should destroy the
        // socket under both situation
        if (__builtin_expect(ver == id_ver || ver == id_ver + 1, 1)) {
            // sees nref:1->0, try to set version=id_ver+2,--nref.
            // No retry: if version changes, the slot is already returned by
            // another one who sees nref:1->0 concurrently; if nref changes,
            // which must be non-zero, the slot will be returned when
            // nref changes from 1->0 again.
            // Example:
            //   SetFailed(): --nref, sees nref:1->0           (1)
            //                try to set version=id_ver+2      (2)
            //    Address():  ++nref, unmatched version        (3)
            //                --nref, sees nref:1->0           (4)
            //                try to set version=id_ver+2      (5)
            // 1,2,3,4,5 or 1,3,4,2,5:
            //            SetFailed() succeeds, Address() fails at (5).
            // 1,3,2,4,5: SetFailed() fails with (2), the slot will be
            //            returned by (5) of Address()
            // 1,3,4,5,2: SetFailed() fails with (2), the slot is already
            //            returned by (5) of Address().
            uint64_t expected_vref = vref - 1;
            if (_versioned_ref.compare_exchange_strong(
                    expected_vref, MakeVRef(id_ver + 2, 0),
                    std::memory_order_acquire,
                    std::memory_order_relaxed)) {
                OnRecycle();
                return_resource(SlotOfSocketId(id));
                return 1;
            }
            return 0;
        }
        LOG(FATAL) << "Invalid SocketId=" << id;
        return -1;
    }
    LOG(FATAL) << "Over dereferenced SocketId=" << id;
    return -1;
}

inline int Socket::Address(SocketId id, SocketUniquePtr* ptr) {
    const mutil::ResourceId<Socket> slot = SlotOfSocketId(id);
    Socket* const m = address_resource(slot);
    if (__builtin_expect(m != NULL, 1)) {
        // acquire fence makes sure this thread sees latest changes before
        // Dereference() or Revive().
        const uint64_t vref1 = m->_versioned_ref.fetch_add(
            1, std::memory_order_acquire);
        const uint32_t ver1 = VersionOfVRef(vref1);
        if (ver1 == VersionOfSocketId(id)) {
            ptr->reset(m);
            return 0;
        }

        const uint64_t vref2 = m->_versioned_ref.fetch_sub(
            1, std::memory_order_release);
        const int32_t nref = NRefOfVRef(vref2);
        if (nref > 1) {
            return -1;
        } else if (__builtin_expect(nref == 1, 1)) {
            const uint32_t ver2 = VersionOfVRef(vref2);
            if ((ver2 & 1)) {
                if (ver1 == ver2 || ver1 + 1 == ver2) {
                    uint64_t expected_vref = vref2 - 1;
                    if (m->_versioned_ref.compare_exchange_strong(
                            expected_vref, MakeVRef(ver2 + 1, 0),
                            std::memory_order_acquire,
                            std::memory_order_relaxed)) {
                        m->OnRecycle();
                        return_resource(SlotOfSocketId(id));
                    }
                } else {
                    CHECK(false) << "ref-version=" << ver1
                                 << " unref-version=" << ver2;
                }
            } else {
                CHECK_EQ(ver1, ver2);
                // Addressed a free slot.
            }
        } else {
            CHECK(false) << "Over dereferenced SocketId=" << id;
        }
    }
    return -1;
}

inline void Socket::ReAddress(SocketUniquePtr* ptr) {
    _versioned_ref.fetch_add(1, std::memory_order_acquire);
    ptr->reset(this);
}

inline int Socket::AddressFailedAsWell(SocketId id, SocketUniquePtr* ptr) {
    const mutil::ResourceId<Socket> slot = SlotOfSocketId(id);
    Socket* const m = address_resource(slot);
    if (__builtin_expect(m != NULL, 1)) {
        const uint64_t vref1 = m->_versioned_ref.fetch_add(
            1, std::memory_order_acquire);
        const uint32_t ver1 = VersionOfVRef(vref1);
        if (ver1 == VersionOfSocketId(id)) {
            ptr->reset(m);
            return 0;
        }
        if (ver1 == VersionOfSocketId(id) + 1) {
            ptr->reset(m);
            return 1;
        }

        const uint64_t vref2 = m->_versioned_ref.fetch_sub(
            1, std::memory_order_release);
        const int32_t nref = NRefOfVRef(vref2);
        if (nref > 1) {
            return -1;
        } else if (__builtin_expect(nref == 1, 1)) {
            const uint32_t ver2 = VersionOfVRef(vref2);
            if ((ver2 & 1)) {
                if (ver1 == ver2 || ver1 + 1 == ver2) {
                    uint64_t expected_vref = vref2 - 1;
                    if (m->_versioned_ref.compare_exchange_strong(
                            expected_vref, MakeVRef(ver2 + 1, 0),
                            std::memory_order_acquire,
                            std::memory_order_relaxed)) {
                        m->OnRecycle();
                        return_resource(slot);
                    }
                } else {
                    CHECK(false) << "ref-version=" << ver1
                                 << " unref-version=" << ver2;
                }
            } else {
                // Addressed a free slot.
            }
        } else {
            CHECK(false) << "Over dereferenced SocketId=" << id;
        }
    }
    return -1;    
}

inline bool Socket::Failed() const {
    return VersionOfVRef(_versioned_ref.load(std::memory_order_relaxed))
        != VersionOfSocketId(_this_id);
}

inline bool Socket::MoreReadEvents(int* progress) {
    // Fail to CAS means that new events arrived.
    return !_nevent.compare_exchange_strong(
        *progress, 0, std::memory_order_release,
            std::memory_order_acquire);
}

inline void Socket::SetLogOff() {
    if (!_logoff_flag.exchange(true, std::memory_order_relaxed)) {
        if (fd() < 0) {
            // This socket hasn't been connected before (such as
            // short connection), so it won't receive any epoll
            // events. We need to `SetFailed' it to trigger health
            // checking, otherwise it may be blocked forever
            SetFailed(ELOGOFF, "The server at %s is stopping",
                      mutil::endpoint2str(remote_side()).c_str());
        }
    }
}

inline bool Socket::IsAvailable() const {
    return !_logoff_flag.load(std::memory_order_relaxed) &&
        (_ninflight_app_health_check.load(std::memory_order_relaxed) == 0);
}

static const uint32_t EOF_FLAG = (1 << 31);

inline void Socket::PostponeEOF() {
    if (CreatedByConnect()) { // not needed at server-side
        _ninprocess.fetch_add(1, std::memory_order_relaxed);
    }
}

inline void Socket::CheckEOF() {
    if (CreatedByConnect()) { // not needed at server-side
        CheckEOFInternal();
    }
}

inline void Socket::CheckEOFInternal() {
    uint32_t nref = _ninprocess.fetch_sub(1, std::memory_order_release);
    if ((nref & ~EOF_FLAG) == 1) {
         std::atomic_thread_fence(std::memory_order_acquire);
        // It's safe to call `SetFailed' each time `_ninprocess' hits 0
        SetFailed(EEOF, "Got EOF of %s", description().c_str());
    }
}

inline void Socket::SetEOF() {
    uint32_t nref = _ninprocess.fetch_or(EOF_FLAG, std::memory_order_relaxed);
    if ((nref & EOF_FLAG) == 0) {
        // Release the additional reference in `_ninprocess'
        CheckEOFInternal();
    }
}

inline void Socket::reset_parsing_context(Destroyable* new_context) {
    Destroyable* old_ctx = _parsing_context.exchange(
        new_context, std::memory_order_acq_rel);
    if (old_ctx) {
        old_ctx->Destroy();
    }
}

inline Destroyable* Socket::release_parsing_context() {
    return _parsing_context.exchange(NULL, std::memory_order_acquire);
}

template <typename T>
bool Socket::initialize_parsing_context(T** ctx) {
    Destroyable* expected = NULL;
    if (_parsing_context.compare_exchange_strong(
            expected, *ctx, std::memory_order_acq_rel,
            std::memory_order_acquire)) {
        return true;
    } else {
        (*ctx)->Destroy();
        *ctx = static_cast<T*>(expected);
        return false;
    }
}

// NOTE: Push/Pop may be called from different threads simultaneously.
inline void Socket::PushPipelinedInfo(const PipelinedInfo& pi) {
    MELON_SCOPED_LOCK(_pipeline_mutex);
    if (_pipeline_q == NULL) {
        _pipeline_q = new std::deque<PipelinedInfo>;
    }
    _pipeline_q->push_back(pi);
}

inline bool Socket::PopPipelinedInfo(PipelinedInfo* info) {
    MELON_SCOPED_LOCK(_pipeline_mutex);
    if (_pipeline_q != NULL && !_pipeline_q->empty()) {
        *info = _pipeline_q->front();
        _pipeline_q->pop_front();
        return true;
    }
    return false;
}

inline void Socket::GivebackPipelinedInfo(const PipelinedInfo& pi) {
    MELON_SCOPED_LOCK(_pipeline_mutex);
    if (_pipeline_q != NULL) {
        _pipeline_q->push_front(pi);
    }
}

inline bool Socket::ValidFileDescriptor(int fd) {
    return fd >= 0 && fd != STREAM_FAKE_FD;
}

inline Socket::SharedPart* Socket::GetSharedPart() const {
    return _shared_part.load(std::memory_order_consume);
}

inline Socket::SharedPart* Socket::GetOrNewSharedPart() {
    SharedPart* shared_part = GetSharedPart();
    if (shared_part != NULL) { // most cases
        return shared_part;
    }
    return GetOrNewSharedPartSlower();
}

} // namespace melon


#endif  // MELON_RPC_SOCKET_INL_H_
