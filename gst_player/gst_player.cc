#include "gst_player.h"

#include <gst/video/video.h>

#define GstPlayer_GstInit_ProgramName "gstreamer"
#define GstPlayer_GstInit_Arg1 "/home/1.ogg"

GstPlayer::GstPlayer(const std::vector<std::string> &cmd_arguments)
{
  if (cmd_arguments.empty())
  {
    char arg0[] = GstPlayer_GstInit_ProgramName;
    char arg1[] = GstPlayer_GstInit_Arg1;
    char *argv[] = {&arg0[0], &arg1[0], NULL};
    int argc = (int)(sizeof(argv) / sizeof(argv[0])) - 1;
    gst_init(&argc, (char ***)&argv);
  }
  else
  {
    // TODO handle this case, pass command line arguments to gstreamer
  }
}

GstPlayer::~GstPlayer()
{
  // TODO Should free GStreamers stuff in destructor,
  // but when implemented, flutter complains something about texture
  // when closing application
  freeGst();
}

void GstPlayer::onVideo(VideoFrameCallback callback)
{
  video_callback_ = callback;
}

void GstPlayer::play(const gchar *pipelineString)
{
  pipelineString_ = pipelineString;

  // Check and free previous playing GStreamers if any
  if (sink_ != nullptr || pipeline != nullptr)
  {
    freeGst();
  }

  pipeline = gst_parse_launch(
      pipelineString_.c_str(),
      nullptr);

  sink_ = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  gst_app_sink_set_emit_signals(GST_APP_SINK(sink_), TRUE);
  g_signal_connect(sink_, "new-sample", G_CALLBACK(newSample), (gpointer)this);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void GstPlayer::freeGst(void)
{
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(sink_);
  gst_object_unref(pipeline);
}

GstFlowReturn GstPlayer::newSample(GstAppSink *sink, gpointer gSelf)
{
  GstSample *sample = NULL;
  GstMapInfo bufferInfo;

  GstPlayer *self = static_cast<GstPlayer *>(gSelf);
  sample = gst_app_sink_pull_sample(GST_APP_SINK(self->sink_));

  if (sample != NULL)
  {
    GstBuffer *buffer_ = gst_sample_get_buffer(sample);
    if (buffer_ != NULL)
    {
      gst_buffer_map(buffer_, &bufferInfo, GST_MAP_READ);

      // Get video width and height
      GstVideoFrame vframe;
      GstVideoInfo video_info;
      GstCaps *sampleCaps = gst_sample_get_caps(sample);
      gst_video_info_from_caps(&video_info, sampleCaps);
      gst_video_frame_map(&vframe, &video_info, buffer_, GST_MAP_READ);

      self->video_callback_(
          (uint8_t *)bufferInfo.data,
          video_info.size,
          video_info.width,
          video_info.height,
          video_info.stride[0]);

      self->aspectRatio_ = (double)video_info.width / (double)video_info.height;

      // g_print("video_info.aspectRatio = %.6f, video_info.width = %d, height = %d\n",
      //         self->aspectRatio_, video_info.width, video_info.height);

      gst_buffer_unmap(buffer_, &bufferInfo);
      gst_video_frame_unmap(&vframe);
    }
    gst_sample_unref(sample);
  }

  return GST_FLOW_OK;
}

GstPlayer *GstPlayers::Get(int32_t id, std::vector<std::string> cmd_arguments)
{
  std::lock_guard<std::mutex> lock(mutex_);
  auto [it, added] = players_.try_emplace(id, nullptr);
  if (added)
  {
    it->second = std::make_unique<GstPlayer>(cmd_arguments);
  }
  return it->second.get();
}

void GstPlayers::dispose(int32_t id)
{
  std::lock_guard<std::mutex> lock(mutex_);
  players_.erase(id);
}

void GstPlayer::play()
{
  if (pipeline)
  {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    playing_ = true;
  }
}

void GstPlayer::pause()
{
  // g_print("[GstPlayer] Pause called. Pipeline is %s\n", pipeline ? "OK" : "NULL");
  if (pipeline)
  {
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    playing_ = false;
  }
}

void GstPlayer::stop()
{
  if (pipeline)
  {
    gst_element_set_state(pipeline, GST_STATE_READY);
    playing_ = false;
  }
}

bool GstPlayer::isPlaying()
{
  return playing_;
}

void GstPlayer::seekTo(int64_t position_ms)
{
  if (!pipeline)
  {
    // g_printerr("[GstPlayer] seekTo failed: pipeline is NULL\n");
    return;
  }

  gint64 current_pos = 0;
  gst_element_query_position(pipeline, GST_FORMAT_TIME, &current_pos);
  gst_app_sink_set_drop(GST_APP_SINK(sink_), TRUE);
  gst_app_sink_set_max_buffers(GST_APP_SINK(sink_), 1);
  gboolean success = gst_element_seek(
      pipeline,
      1.0,
      GST_FORMAT_TIME,
      (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
      GST_SEEK_TYPE_SET, position_ms * GST_MSECOND,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  // 防止 unused variable 警告
  (void)success;

  gst_app_sink_set_drop(GST_APP_SINK(sink_), FALSE);

  // g_print("[GstPlayer] seekTo called. From %.2f sec -> %.2f sec. Success: %s\n",
  //         current_pos / (double)GST_SECOND,
  //         position_ms / 1000.0,
  //         success ? "YES" : "NO");

  // g_printerr("[GstPlayer] seekTo : is playing: %s\n", playing_ ? "true" : "false");

  // 關鍵點：如果是 paused 狀態，播放更新畫面並馬上暫停
  if (!playing_)
  {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    gst_element_get_state(pipeline, nullptr, nullptr, 500 * GST_MSECOND);
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    gst_element_get_state(pipeline, nullptr, nullptr, 500 * GST_MSECOND);
  }
}

int64_t GstPlayer::position()
{
  gint64 pos = 0;
  if (pipeline && gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos))
  {
    return pos / GST_MSECOND;
  }
  return 0;
}

int64_t GstPlayer::duration()
{
  gint64 dur = 0;
  if (pipeline && gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur))
  {
    return dur / GST_MSECOND;
  }
  return 0;
}

double GstPlayer::aspectRatio()
{
  return aspectRatio_;
}