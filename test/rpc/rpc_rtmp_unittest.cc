// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//




// Date: Fri May 20 15:52:22 CST 2016

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/rtmp/rtmp.h"
#include "melon/rpc/rtmp/amf.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

class TestRtmpClientStream : public melon::RtmpClientStream {
public:
    TestRtmpClientStream()
        : _called_on_stop(0)
        , _called_on_first_message(0)
        , _nvideomsg(0)
        , _naudiomsg(0) {
        LOG(INFO) << __FUNCTION__;
    }
    ~TestRtmpClientStream() {
        LOG(INFO) << __FUNCTION__;
        assertions_on_stop();
    }
    void assertions_on_stop() {
        ASSERT_EQ(1, _called_on_stop);
    }
    void assertions_on_successful_play() {
        ASSERT_EQ(1, _called_on_first_message);
        ASSERT_LT(0, _nvideomsg);
        ASSERT_LT(0, _naudiomsg);
    }
    void assertions_on_failure() {
        ASSERT_EQ(0, _called_on_first_message);
        ASSERT_EQ(0, _nvideomsg);
        ASSERT_EQ(0, _naudiomsg);
        assertions_on_stop();
    }
    void OnFirstMessage() {
        ++_called_on_first_message;
    }
    void OnStop() {
        ++_called_on_stop;
    }
    void OnVideoMessage(melon::RtmpVideoMessage* msg) {
        ++_nvideomsg;
        // video data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
    void OnAudioMessage(melon::RtmpAudioMessage* msg) {
        ++_naudiomsg;
        // audio data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
private:
    int _called_on_stop;
    int _called_on_first_message;
    int _nvideomsg;
    int _naudiomsg;
};

class TestRtmpRetryingClientStream
    : public melon::RtmpRetryingClientStream {
public:
    TestRtmpRetryingClientStream()
        : _called_on_stop(0)
        , _called_on_first_message(0)
        , _called_on_playable(0) {
        LOG(INFO) << __FUNCTION__;
    }
    ~TestRtmpRetryingClientStream() {
        LOG(INFO) << __FUNCTION__;
        assertions_on_stop();
    }
    void assertions_on_stop() {
        ASSERT_EQ(1, _called_on_stop);
    }
    void OnStop() {
        ++_called_on_stop;
    }
    void OnFirstMessage() {
        ++_called_on_first_message;
    }
    void OnPlayable() {
        ++_called_on_playable;
    }

    void OnVideoMessage(melon::RtmpVideoMessage* msg) {
        // video data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
    void OnAudioMessage(melon::RtmpAudioMessage* msg) {
        // audio data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
private:
    int _called_on_stop;
    int _called_on_first_message;
    int _called_on_playable;
};

const char* UNEXIST_NAME = "unexist_stream";

class PlayingDummyStream : public melon::RtmpServerStream {
public:
    enum State {
        STATE_UNPLAYING,
        STATE_PLAYING,
        STATE_STOPPED
    };
    PlayingDummyStream(int64_t sleep_ms)
        : _state(STATE_UNPLAYING), _sleep_ms(sleep_ms) {
        LOG(INFO) << __FUNCTION__ << "(" << this << ")";
    }
    ~PlayingDummyStream() {
        LOG(INFO) << __FUNCTION__ << "(" << this << ")";
    }
    void OnPlay(const melon::RtmpPlayOptions& opt,
                mutil::Status* status,
                google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got play{stream_name=" << opt.stream_name
                  << " start=" << opt.start
                  << " duration=" << opt.duration
                  << " reset=" << opt.reset << '}';
        if (opt.stream_name == UNEXIST_NAME) {
            status->set_error(EPERM, "Unexist stream");
            return;
        }
        if (_sleep_ms > 0) {
            LOG(INFO) << "Sleep " << _sleep_ms
                      << " ms before responding play request";
            fiber_usleep(_sleep_ms * 1000L);
        }
        int rc = fiber_start_background(&_play_thread, NULL,
                                          RunSendData, this);
        if (rc) {
            status->set_error(rc, "Fail to create thread");
            return;
        }
        State expected = STATE_UNPLAYING;
        if (!_state.compare_exchange_strong(expected, STATE_PLAYING)) {
            if (expected == STATE_STOPPED) {
                fiber_stop(_play_thread);
                fiber_join(_play_thread, NULL);
            } else {
                CHECK(false) << "Impossible";
            }
        }
    }

    void OnStop() {
        LOG(INFO) << "OnStop of PlayingDummyStream=" << this;
        if (_state.exchange(STATE_STOPPED) == STATE_PLAYING) {
            fiber_stop(_play_thread);
            fiber_join(_play_thread, NULL);
        }
    }

    void SendData();
    
private:
    static void* RunSendData(void* arg) {
        ((PlayingDummyStream*)arg)->SendData();
        return NULL;
    }

    mutil::atomic<State> _state;
    fiber_t _play_thread;
    int64_t _sleep_ms;
};

void PlayingDummyStream::SendData() {
    LOG(INFO) << "Enter SendData of PlayingDummyStream=" << this;

    melon::RtmpVideoMessage vmsg;
    melon::RtmpAudioMessage amsg;

    vmsg.timestamp = 1000;
    amsg.timestamp = 1000;
    for (int i = 0; !fiber_stopped(fiber_self()); ++i) {
        vmsg.timestamp += 20;
        amsg.timestamp += 20;

        vmsg.frame_type = melon::FLV_VIDEO_FRAME_KEYFRAME;
        vmsg.codec = melon::FLV_VIDEO_AVC;
        vmsg.data.clear();
        vmsg.data.append(mutil::string_printf("video_%d(ms_id=%u)",
                                             i, stream_id()));
        //failing to send is possible
        SendVideoMessage(vmsg);

        amsg.codec = melon::FLV_AUDIO_AAC;
        amsg.rate = melon::FLV_SOUND_RATE_44100HZ;
        amsg.bits = melon::FLV_SOUND_16BIT;
        amsg.type = melon::FLV_SOUND_STEREO;
        amsg.data.clear();
        amsg.data.append(mutil::string_printf("audio_%d(ms_id=%u)",
                                             i, stream_id()));
        SendAudioMessage(amsg);

        fiber_usleep(1000000);
    }

    LOG(INFO) << "Quit SendData of PlayingDummyStream=" << this;
}

class PlayingDummyService : public melon::RtmpService {
public:
    PlayingDummyService(int64_t sleep_ms = 0) : _sleep_ms(sleep_ms) {}

private:
    // Called to create a server-side stream.
    virtual melon::RtmpServerStream* NewStream(
        const melon::RtmpConnectRequest&) {
        return new PlayingDummyStream(_sleep_ms);
    }
    int64_t _sleep_ms;
};

class PublishStream : public melon::RtmpServerStream {
public:
    PublishStream(int64_t sleep_ms)
        : _sleep_ms(sleep_ms)
        , _called_on_stop(0)
        , _called_on_first_message(0)
        , _nvideomsg(0)
        , _naudiomsg(0) {
        LOG(INFO) << __FUNCTION__ << "(" << this << ")";
    }
    ~PublishStream() {
        LOG(INFO) << __FUNCTION__ << "(" << this << ")";
        assertions_on_stop();
    }
    void assertions_on_stop() {
        ASSERT_EQ(1, _called_on_stop);
    }
    void OnPublish(const std::string& stream_name,
                   melon::RtmpPublishType publish_type,
                   mutil::Status* status,
                   google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got publish{stream_name=" << stream_name
                  << " type=" << melon::RtmpPublishType2Str(publish_type)
                  << '}';
        if (stream_name == UNEXIST_NAME) {
            status->set_error(EPERM, "Unexist stream");
            return;
        }
        if (_sleep_ms > 0) {
            LOG(INFO) << "Sleep " << _sleep_ms
                      << " ms before responding play request";
            fiber_usleep(_sleep_ms * 1000L);
        }
    }
    void OnFirstMessage() {
        ++_called_on_first_message;
    }
    void OnStop() {
        LOG(INFO) << "OnStop of PublishStream=" << this;
        ++_called_on_stop;
    }
    void OnVideoMessage(melon::RtmpVideoMessage* msg) {
        ++_nvideomsg;
        // video data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
    void OnAudioMessage(melon::RtmpAudioMessage* msg) {
        ++_naudiomsg;
        // audio data is ascii in UT, print it out.
        LOG(INFO) << remote_side() << "|stream=" << stream_id()
                  << ": Got " << *msg << " data=" << msg->data;
    }
private:
    int64_t _sleep_ms;
    int _called_on_stop;
    int _called_on_first_message;
    int _nvideomsg;
    int _naudiomsg;
};

class PublishService : public melon::RtmpService {
public:
    PublishService(int64_t sleep_ms = 0) : _sleep_ms(sleep_ms) {
        pthread_mutex_init(&_mutex, NULL);
    }
    ~PublishService() {
        pthread_mutex_destroy(&_mutex);
    }
    void move_created_streams(
        std::vector<mutil::intrusive_ptr<PublishStream> >* out) {
        out->clear();
        MELON_SCOPED_LOCK(_mutex);
        out->swap(_created_streams);
    }

private:
    // Called to create a server-side stream.
    virtual melon::RtmpServerStream* NewStream(
        const melon::RtmpConnectRequest&) {
        PublishStream* stream = new PublishStream(_sleep_ms);
        {
            MELON_SCOPED_LOCK(_mutex);
            _created_streams.push_back(stream);
        }
        return stream;
    }
    int64_t _sleep_ms;
    pthread_mutex_t _mutex;
    std::vector<mutil::intrusive_ptr<PublishStream> > _created_streams;
};

class RtmpSubStream : public melon::RtmpClientStream {
public:
    explicit RtmpSubStream(melon::RtmpMessageHandler* mh)
        : _message_handler(mh) {}
    // @RtmpStreamBase
    void OnMetaData(melon::RtmpMetaData*, const mutil::StringPiece&);
    void OnSharedObjectMessage(melon::RtmpSharedObjectMessage* msg);
    void OnAudioMessage(melon::RtmpAudioMessage* msg);
    void OnVideoMessage(melon::RtmpVideoMessage* msg);
    void OnFirstMessage();
    void OnStop();
private:
    std::unique_ptr<melon::RtmpMessageHandler> _message_handler;
};

void RtmpSubStream::OnFirstMessage() {
    _message_handler->OnPlayable();
}

void RtmpSubStream::OnMetaData(melon::RtmpMetaData* obj, const mutil::StringPiece& name) {
    _message_handler->OnMetaData(obj, name);
}

void RtmpSubStream::OnSharedObjectMessage(melon::RtmpSharedObjectMessage* msg) {
    _message_handler->OnSharedObjectMessage(msg);
}

void RtmpSubStream::OnAudioMessage(melon::RtmpAudioMessage* msg) {
    _message_handler->OnAudioMessage(msg);
}

void RtmpSubStream::OnVideoMessage(melon::RtmpVideoMessage* msg) {
    _message_handler->OnVideoMessage(msg);
}

void RtmpSubStream::OnStop() {
    _message_handler->OnSubStreamStop(this);
}


class RtmpSubStreamCreator : public melon::SubStreamCreator {
public:
    RtmpSubStreamCreator(const melon::RtmpClient* client);

    ~RtmpSubStreamCreator();

    // @SubStreamCreator
    void NewSubStream(melon::RtmpMessageHandler* message_handler,
                      mutil::intrusive_ptr<melon::RtmpStreamBase>* sub_stream);
    void LaunchSubStream(melon::RtmpStreamBase* sub_stream,
                         melon::RtmpRetryingClientStreamOptions* options);

private:
    const melon::RtmpClient* _client;
};

RtmpSubStreamCreator::RtmpSubStreamCreator(const melon::RtmpClient* client)
    : _client(client) {}

RtmpSubStreamCreator::~RtmpSubStreamCreator() {}
 
void RtmpSubStreamCreator::NewSubStream(melon::RtmpMessageHandler* message_handler,
                                        mutil::intrusive_ptr<melon::RtmpStreamBase>* sub_stream) {
    if (sub_stream) { 
        (*sub_stream).reset(new RtmpSubStream(message_handler));
    }
    return;
}

void RtmpSubStreamCreator::LaunchSubStream(
    melon::RtmpStreamBase* sub_stream,
    melon::RtmpRetryingClientStreamOptions* options) {
    melon::RtmpClientStreamOptions client_options = *options;
    dynamic_cast<RtmpSubStream*>(sub_stream)->Init(_client, client_options);
}

TEST(RtmpTest, parse_rtmp_url) {
    mutil::StringPiece host;
    mutil::StringPiece vhost;
    mutil::StringPiece port;
    mutil::StringPiece app;
    mutil::StringPiece stream_name;

    melon::ParseRtmpURL("rtmp://HOST/APP/STREAM",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_TRUE(vhost.empty());
    ASSERT_EQ("1935", port);
    ASSERT_EQ("APP", app);
    ASSERT_EQ("STREAM", stream_name);

    melon::ParseRtmpURL("HOST/APP/STREAM",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_TRUE(vhost.empty());
    ASSERT_EQ("1935", port);
    ASSERT_EQ("APP", app);
    ASSERT_EQ("STREAM", stream_name);

    melon::ParseRtmpURL("rtmp://HOST:8765//APP?vhost=abc///STREAM?queries",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_EQ("abc", vhost);
    ASSERT_EQ("8765", port);
    ASSERT_EQ("APP", app);
    ASSERT_EQ("STREAM?queries", stream_name);

    melon::ParseRtmpURL("HOST:8765//APP?vhost=abc///STREAM?queries",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_EQ("abc", vhost);
    ASSERT_EQ("8765", port);
    ASSERT_EQ("APP", app);
    ASSERT_EQ("STREAM?queries", stream_name);

    melon::ParseRtmpURL("HOST:8765//APP?vhost=abc///STREAM?queries/",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_EQ("abc", vhost);
    ASSERT_EQ("8765", port);
    ASSERT_EQ("APP", app);
    ASSERT_EQ("STREAM?queries/", stream_name);

    melon::ParseRtmpURL("HOST:8765/APP?vhost=abc",
                             &host, &vhost, &port, &app, &stream_name);
    ASSERT_EQ("HOST", host);
    ASSERT_EQ("abc", vhost);
    ASSERT_EQ("8765", port);
    ASSERT_EQ("APP", app);
    ASSERT_TRUE(stream_name.empty());
}

TEST(RtmpTest, amf) {
    std::string req_buf;
    melon::RtmpInfo info;
    melon::AMFObject obj;
    std::string dummy = "_result";
    {
        google::protobuf::io::StringOutputStream zc_stream(&req_buf);
        melon::AMFOutputStream ostream(&zc_stream);
        melon::WriteAMFString(dummy, &ostream);
        melon::WriteAMFUint32(17, &ostream);
        info.set_code("NetConnection.Connect"); // TODO
        info.set_level("error");
        info.set_description("heheda hello foobar");
        melon::WriteAMFObject(info, &ostream);
        ASSERT_TRUE(ostream.good());
        obj.SetString("code", "foo");
        obj.SetString("level", "bar");
        obj.SetString("description", "heheda");
        melon::WriteAMFObject(obj, &ostream);
        ASSERT_TRUE(ostream.good());
    }

    google::protobuf::io::ArrayInputStream zc_stream(req_buf.data(), req_buf.size());
    melon::AMFInputStream istream(&zc_stream);
    std::string result;
    ASSERT_TRUE(melon::ReadAMFString(&result, &istream));
    ASSERT_EQ(dummy, result);
    uint32_t num = 0;
    ASSERT_TRUE(melon::ReadAMFUint32(&num, &istream));
    ASSERT_EQ(17u, num);
    melon::RtmpInfo info2;
    ASSERT_TRUE(melon::ReadAMFObject(&info2, &istream));
    ASSERT_EQ(info.code(), info2.code());
    ASSERT_EQ(info.level(), info2.level());
    ASSERT_EQ(info.description(), info2.description());
    melon::RtmpInfo info3;
    ASSERT_TRUE(melon::ReadAMFObject(&info3, &istream));
    ASSERT_EQ("foo", info3.code());
    ASSERT_EQ("bar", info3.level());
    ASSERT_EQ("heheda", info3.description());
}

TEST(RtmpTest, successfully_play_streams) {
    PlayingDummyService rtmp_service;
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8571, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8571", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        //opt.publish_name = mutil::string_printf("pub_name_%d", i);
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
    }
    sleep(5);
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i]->assertions_on_successful_play();
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, fail_to_play_streams) {
    PlayingDummyService rtmp_service;
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8571, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8571", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.play_name = UNEXIST_NAME;
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
    }
    sleep(1);
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i]->assertions_on_failure();
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, successfully_publish_streams) {
    PublishService rtmp_service;
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8571, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8571", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.publish_name = mutil::string_printf("pub_name_%d", i);
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
    }
    const int REP = 5;
    for (int i = 0; i < REP; ++i) {
        melon::RtmpVideoMessage vmsg;
        vmsg.timestamp = 1000 + i * 20;
        vmsg.frame_type = melon::FLV_VIDEO_FRAME_KEYFRAME;
        vmsg.codec = melon::FLV_VIDEO_AVC;
        vmsg.data.append(mutil::string_printf("video_%d", i));
        for (int j = 0; j < NSTREAM; j += 2) {
            ASSERT_EQ(0, cstreams[j]->SendVideoMessage(vmsg));
        }
        
        melon::RtmpAudioMessage amsg;
        amsg.timestamp = 1000 + i * 20;
        amsg.codec = melon::FLV_AUDIO_AAC;
        amsg.rate = melon::FLV_SOUND_RATE_44100HZ;
        amsg.bits = melon::FLV_SOUND_16BIT;
        amsg.type = melon::FLV_SOUND_STEREO;
        amsg.data.append(mutil::string_printf("audio_%d", i));
        for (int j = 1; j < NSTREAM; j += 2) {
            ASSERT_EQ(0, cstreams[j]->SendAudioMessage(amsg));
        }
        
        fiber_usleep(500000);
    }
    std::vector<mutil::intrusive_ptr<PublishStream> > created_streams;
    rtmp_service.move_created_streams(&created_streams);
    ASSERT_EQ(NSTREAM, (int)created_streams.size());
    for (int i = 0; i < NSTREAM; ++i) {
        EXPECT_EQ(1, created_streams[i]->_called_on_first_message);
    }
    for (int j = 0; j < NSTREAM; j += 2) {
        ASSERT_EQ(REP, created_streams[j]->_nvideomsg);
    }
    for (int j = 1; j < NSTREAM; j += 2) {
        ASSERT_EQ(REP, created_streams[j]->_naudiomsg);
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, failed_to_publish_streams) {
    PublishService rtmp_service;
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8575, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8575", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.publish_name = UNEXIST_NAME;
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
    }
    sleep(1);
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i]->assertions_on_failure();
    }
    std::vector<mutil::intrusive_ptr<PublishStream> > created_streams;
    rtmp_service.move_created_streams(&created_streams);
    ASSERT_EQ(NSTREAM, (int)created_streams.size());
    for (int i = 0; i < NSTREAM; ++i) {
        ASSERT_EQ(0, created_streams[i]->_called_on_first_message);
        ASSERT_EQ(0, created_streams[i]->_nvideomsg);
        ASSERT_EQ(0, created_streams[i]->_naudiomsg);
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, failed_to_connect_client_streams) {
    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8572", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
        cstreams[i]->assertions_on_failure();
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, destroy_client_streams_before_init) {
    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8573", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    mutil::intrusive_ptr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        cstreams[i]->Destroy();
        ASSERT_EQ(1, cstreams[i]->_called_on_stop);
        ASSERT_EQ(melon::RtmpClientStream::STATE_DESTROYING, cstreams[i]->_state);
        melon::RtmpClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        opt.wait_until_play_or_publish_is_sent = true;
        cstreams[i]->Init(&rtmp_client, opt);
        cstreams[i]->assertions_on_failure();
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, destroy_retrying_client_streams_before_init) {
    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8573", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    mutil::intrusive_ptr<TestRtmpRetryingClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpRetryingClientStream);
        cstreams[i]->Destroy();
        ASSERT_EQ(1, cstreams[i]->_called_on_stop);
        melon::RtmpRetryingClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        melon::SubStreamCreator* sc = new RtmpSubStreamCreator(&rtmp_client);
        cstreams[i]->Init(sc, opt);
        ASSERT_EQ(1, cstreams[i]->_called_on_stop);
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, destroy_client_streams_during_creation) {
    PlayingDummyService rtmp_service(2000/*sleep 2s*/);
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8574, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8574", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    mutil::intrusive_ptr<TestRtmpClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpClientStream);
        melon::RtmpClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        cstreams[i]->Init(&rtmp_client, opt);
        ASSERT_EQ(0, cstreams[i]->_called_on_stop);
        usleep(500*1000);
        ASSERT_EQ(0, cstreams[i]->_called_on_stop);
        cstreams[i]->Destroy();
        usleep(10*1000);
        ASSERT_EQ(1, cstreams[i]->_called_on_stop);
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, destroy_retrying_client_streams_during_creation) {
    PlayingDummyService rtmp_service(2000/*sleep 2s*/);
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8574, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8574", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    mutil::intrusive_ptr<TestRtmpRetryingClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpRetryingClientStream);
        melon::RtmpRetryingClientStreamOptions opt;
        opt.play_name = mutil::string_printf("play_name_%d", i);
        melon::SubStreamCreator* sc = new RtmpSubStreamCreator(&rtmp_client);
        cstreams[i]->Init(sc, opt);
        ASSERT_EQ(0, cstreams[i]->_called_on_stop);
        usleep(500*1000);
        ASSERT_EQ(0, cstreams[i]->_called_on_stop);
        cstreams[i]->Destroy();
        usleep(10*1000);
        ASSERT_EQ(1, cstreams[i]->_called_on_stop);
    }
    LOG(INFO) << "Quiting program...";
}

TEST(RtmpTest, retrying_stream) {
    PlayingDummyService rtmp_service;
    melon::Server server;
    melon::ServerOptions server_opt;
    server_opt.rtmp_service = &rtmp_service;
    ASSERT_EQ(0, server.Start(8576, &server_opt));

    melon::RtmpClientOptions rtmp_opt;
    rtmp_opt.app = "hello";
    rtmp_opt.swfUrl = "anything";
    rtmp_opt.tcUrl = "rtmp://heheda";
    melon::RtmpClient rtmp_client;
    ASSERT_EQ(0, rtmp_client.Init("localhost:8576", rtmp_opt));

    // Create multiple streams.
    const int NSTREAM = 2;
    melon::DestroyingPtr<TestRtmpRetryingClientStream> cstreams[NSTREAM];
    for (int i = 0; i < NSTREAM; ++i) {
        cstreams[i].reset(new TestRtmpRetryingClientStream);
        melon::Controller cntl;
        melon::RtmpRetryingClientStreamOptions opt;
        opt.play_name = mutil::string_printf("name_%d", i);
        melon::SubStreamCreator* sc = new RtmpSubStreamCreator(&rtmp_client);
        cstreams[i]->Init(sc, opt);
    }
    sleep(3);
    LOG(INFO) << "Stopping server";
    server.Stop(0);
    server.Join();
    LOG(INFO) << "Stopped server and sleep for a while";
    sleep(3);
    ASSERT_EQ(0, server.Start(8576, &server_opt));
    sleep(3);
    for (int i = 0; i < NSTREAM; ++i) {
        ASSERT_EQ(1, cstreams[i]->_called_on_first_message);
        ASSERT_EQ(2, cstreams[i]->_called_on_playable);
    }
    LOG(INFO) << "Quiting program...";
}
