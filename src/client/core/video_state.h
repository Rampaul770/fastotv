/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t, uint8_t, int16_t
#include <memory>    // for shared_ptr
#include <string>    // for string

#include "ffmpeg_config.h"  // for CONFIG_AVFILTER

extern "C" {
#include <libavfilter/avfilter.h>  // for AVFilterContext (ptr only), etc
#include <libavformat/avformat.h>  // for AVInputFormat, etc
#include <libavutil/frame.h>       // for AVFrame
#include <libavutil/rational.h>    // for AVRational
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN, etc
#include <common/smart_ptr.h>
#include <common/threads/types.h>
#include <common/url.h>

#include "client_server_types.h"

#include "client/core/app_options.h"
#include "client/core/audio_params.h"  // for AudioParams
#include "client/core/clock.h"
#include "client/core/types.h"

struct SwrContext;
struct InputStream;

namespace common {
namespace threads {
template <typename RT>
class Thread;
}
}  // namespace common

/* no AV correction is done if too big error */
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
class AudioDecoder;
}
namespace core {
class AudioStream;
}
namespace core {
class VideoDecoder;
}
namespace core {
class VideoStream;
}
namespace core {
struct VideoFrame;
}
namespace core {
template <size_t buffer_size>
class AudioFrameQueue;
}
namespace core {
template <size_t buffer_size>
class VideoFrameQueue;
}
namespace core {

struct Stats {
  Stats() : frame_drops_early(0), frame_drops_late(0) {}

  int frame_drops_early;
  int frame_drops_late;
};

class VideoStateHandler;
class VideoState {
 public:
  enum { invalid_stream_index = -1 };
  VideoState(stream_id id,
             const common::uri::Uri& uri,
             const core::AppOptions& opt,
             const core::ComplexOptions& copt,
             VideoStateHandler* handler);
  int Exec() WARN_UNUSED_RESULT;
  void Abort();

  bool IsReadThread() const;
  bool IsVideoThread() const;
  bool IsAudioThread() const;

  bool IsAborted() const;
  bool IsStreamReady() const;
  stream_id GetId() const;
  const common::uri::Uri& GetUri() const;
  virtual ~VideoState();

  void RefreshRequest();
  /* pause or resume the video */
  void TogglePause();

  void StepToNextFrame();
  void SeekNextChunk();
  void SeekPrevChunk();
  void SeekChapter(int incr);
  void Seek(clock_t msec);
  void SeekMsec(clock_t msec);
  void StreamCycleChannel(AVMediaType codec_type);

  virtual int HandleAllocPictureEvent() WARN_UNUSED_RESULT;

  void TryRefreshVideo();
  void UpdateAudioBuffer(uint8_t* stream, int len, int audio_volume);

 private:
  void StreamSeek(int64_t pos, int64_t rel, bool seek_by_bytes);

  void Close();

  bool IsVideoReady() const;
  bool IsAudioReady() const;

  DISALLOW_COPY_AND_ASSIGN(VideoState);

  /* open a given stream. Return 0 if OK */
  int StreamComponentOpen(int stream_index);
  void StreamComponentClose(int stream_index);
  void StreamTogglePause();

  AvSyncType GetMasterSyncType() const;
  clock_t ComputeTargetDelay(clock_t delay) const;
  clock_t GetMasterClock() const;
#if CONFIG_AVFILTER
  int ConfigureVideoFilters(AVFilterGraph* graph, const std::string& vfilters, AVFrame* frame);
  int ConfigureAudioFilters(const std::string& afilters, int force_output_format);
#endif

  int VideoOpen(core::VideoFrame* vp);
  /* allocate a picture (needs to do that in main thread to avoid
     potential locking problems */
  int AllocPicture();
  void VideoDisplay();

  /* return the wanted number of samples to get better sync if sync_type is video
   * or external master clock */
  int SynchronizeAudio(int nb_samples);
  /**
   * Decode one audio frame and return its uncompressed size.
   *
   * The processed audio frame is decoded, converted if required, and
   * stored in is->audio_buf, with size in bytes given by the return
   * value.
   */
  int AudioDecodeFrame();
  int GetVideoFrame(AVFrame* frame);
  int QueuePicture(AVFrame* src_frame, clock_t pts, clock_t duration, int64_t pos);

  int ReadThread();
  int VideoThread();
  int AudioThread();

  const stream_id id_;
  const common::uri::Uri uri_;

  core::AppOptions opt_;
  core::ComplexOptions copt_;

  common::shared_ptr<common::threads::Thread<int> > read_tid_;
  bool force_refresh_;
  int read_pause_return_;
  AVFormatContext* ic_;
  bool realtime_;

  core::VideoStream* vstream_;
  core::AudioStream* astream_;

  core::VideoDecoder* viddec_;
  core::AudioDecoder* auddec_;

  core::VideoFrameQueue<VIDEO_PICTURE_QUEUE_SIZE>* video_frame_queue_;
  core::AudioFrameQueue<SAMPLE_QUEUE_SIZE>* audio_frame_queue_;

  clock_t audio_clock_;
  clock_t audio_diff_cum_; /* used for AV difference average computation */
  double audio_diff_avg_coef_;
  double audio_diff_threshold_;
  int audio_diff_avg_count_;
  int audio_hw_buf_size_;
  uint8_t* audio_buf_;
  uint8_t* audio_buf1_;
  unsigned int audio_buf_size_; /* in bytes */
  unsigned int audio_buf1_size_;
  int audio_buf_index_; /* in bytes */
  int audio_write_buf_size_;
  struct core::AudioParams audio_src_;
#if CONFIG_AVFILTER
  struct core::AudioParams audio_filter_src_;
#endif
  struct core::AudioParams audio_tgt_;
  struct SwrContext* swr_ctx_;

  clock_t frame_timer_;
  clock_t frame_last_returned_time_;
  clock_t frame_last_filter_delay_;
  clock_t max_frame_duration_;  // maximum duration of a frame - above this, we consider the jump a
                                // timestamp discontinuity

  bool step_;

#if CONFIG_AVFILTER
  AVFilterContext* in_video_filter_;   // the first filter in the video chain
  AVFilterContext* out_video_filter_;  // the last filter in the video chain
  AVFilterContext* in_audio_filter_;   // the first filter in the audio chain
  AVFilterContext* out_audio_filter_;  // the last filter in the audio chain
  AVFilterGraph* agraph_;              // audio filter graph
#endif

  int last_video_stream_;
  int last_audio_stream_;

  common::shared_ptr<common::threads::Thread<int> > vdecoder_tid_;
  common::shared_ptr<common::threads::Thread<int> > adecoder_tid_;

  bool paused_;
  bool last_paused_;
  bool eof_;
  bool abort_request_;

  Stats stats_;
  VideoStateHandler* handler_;
  InputStream* input_st_;

  bool seek_req_;
  int64_t seek_pos_;
  int64_t seek_rel_;
  int seek_flags_;

  common::condition_variable read_thread_cond_;
  common::mutex read_thread_mutex_;
};

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
