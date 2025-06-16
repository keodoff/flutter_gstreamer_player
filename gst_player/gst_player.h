#ifndef GST_PLAYER_H_
#define GST_PLAYER_H_

#include <map>
#include <mutex>
#include <vector>
#include <memory>
#include <string>
#include <functional>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

typedef std::function<void(uint8_t *, uint32_t, int32_t, int32_t, int32_t)> VideoFrameCallback;

class GstPlayer
{
public:
  GstPlayer(const std::vector<std::string> &cmd_arguments);

  ~GstPlayer(void);

  void onVideo(VideoFrameCallback callback);

  void play(const gchar *pipelineString);

  void play();

  void pause();

  void stop();

  bool isPlaying();

  int64_t position();

  int64_t duration();

  void seekTo(int64_t position_ms);

  void dispose(int32_t id);

private:
  std::string pipelineString_;
  VideoFrameCallback video_callback_;

  GstElement *pipeline = nullptr;
  GstElement *sink_ = nullptr;

  bool playing_ = false;

  void freeGst(void);

  static GstFlowReturn newSample(GstAppSink *sink, gpointer gSelf);
};

class GstPlayers
{
public:
  GstPlayer *Get(int32_t id, std::vector<std::string> cmd_arguments = {});

  void dispose(int32_t id);

private:
  std::mutex mutex_;
  std::map<int32_t, std::unique_ptr<GstPlayer>> players_;
};

static std::unique_ptr<GstPlayers> g_players = std::make_unique<GstPlayers>();

#endif
