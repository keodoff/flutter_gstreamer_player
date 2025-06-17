import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter/material.dart';

class GstPlayerTextureController {
  static const MethodChannel _channel = MethodChannel(
    'flutter_gstreamer_player',
  );

  int textureId = 0;
  static int _id = 0;
  bool isPlaying = false;

  Future<int> initialize(String pipeline) async {
    // No idea why, but you have to increase `_id` first before pass it to method channel,
    // if not, receiver of method channel always received 0
    GstPlayerTextureController._id = GstPlayerTextureController._id + 1;

    textureId = await _channel.invokeMethod('PlayerRegisterTexture', {
      'pipeline': pipeline,
      'playerId': GstPlayerTextureController._id,
    });

    isPlaying = true;
    return textureId;
  }

  Future<Duration> position() async {
    final int ms = await _channel.invokeMethod('position', {
      'playerId': GstPlayerTextureController._id,
    });
    return Duration(milliseconds: ms);
  }

  Future<Duration> duration() async {
    final int ms = await _channel.invokeMethod('duration', {
      'playerId': GstPlayerTextureController._id,
    });
    return Duration(milliseconds: ms);
  }

  Future<void> play() async {
    await _channel.invokeMethod('play', {
      'playerId': GstPlayerTextureController._id,
    });
    isPlaying = true;
  }

  Future<void> pause() async {
    await _channel.invokeMethod('pause', {
      'playerId': GstPlayerTextureController._id,
    });
    isPlaying = false;
  }

  Future<void> stop() async {
    await _channel.invokeMethod('stop', {
      'playerId': GstPlayerTextureController._id,
    });
    isPlaying = false;
  }

  Future<void> seekTo(Duration position) async {
    await _channel.invokeMethod('seekTo', {
      'playerId': GstPlayerTextureController._id,
      'position': position.inMilliseconds,
    });
  }

  Future<double> aspectRatio() async {
    return await _channel.invokeMethod('aspectRatio', {
      'playerId': GstPlayerTextureController._id,
    });
  }

  Future<void> dispose() async {
    await _channel.invokeMethod('dispose', {'textureId': textureId});
  }

  bool get isInitialized => textureId != null;
}

class GstPlayer extends StatefulWidget {
  String pipeline;

  GstPlayer({Key? key, required this.pipeline}) : super(key: key);

  @override
  State<GstPlayer> createState() => _GstPlayerState();
}

class _GstPlayerState extends State<GstPlayer> {
  final _controller = GstPlayerTextureController();

  @override
  void initState() {
    initializeController();
    super.initState();
  }

  @override
  void didUpdateWidget(GstPlayer oldWidget) {
    if (widget.pipeline != oldWidget.pipeline) {
      initializeController();
    }
    super.didUpdateWidget(oldWidget);
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Future<Null> initializeController() async {
    await _controller.initialize(widget.pipeline);
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    var currentPlatform = Theme.of(context).platform;

    switch (currentPlatform) {
      case TargetPlatform.linux:
      case TargetPlatform.android:
        return Container(
          child: _controller.isInitialized
              ? Texture(textureId: _controller.textureId)
              : null,
        );
        break;
      case TargetPlatform.iOS:
        String viewType = _controller.textureId.toString();
        final Map<String, dynamic> creationParams = <String, dynamic>{};
        return UiKitView(
          viewType: viewType,
          layoutDirection: TextDirection.ltr,
          creationParams: creationParams,
          creationParamsCodec: const StandardMessageCodec(),
        );
        break;
      default:
        throw UnsupportedError('Unsupported platform view');
    }
  }
}
