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


#include <signal.h>
#include <melon/fiber/interrupt_pthread.h>

namespace fiber {

    // TODO: Make sure SIGURG is not used by user.
    // This empty handler is simply for triggering EINTR in blocking syscalls.
    void do_nothing_handler(int) {}

    static pthread_once_t register_sigurg_once = PTHREAD_ONCE_INIT;

    static void register_sigurg() {
        signal(SIGURG, do_nothing_handler);
    }

    int interrupt_pthread(pthread_t th) {
        pthread_once(&register_sigurg_once, register_sigurg);
        return pthread_kill(th, SIGURG);
    }

}  // namespace fiber
