import 'package:flutter/material.dart';
import 'package:flutter_gstreamer_player/flutter_gstreamer_player.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      home: MyWidget(),
    );
  }
}

class MyWidget extends StatelessWidget {
  const MyWidget({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          const Text('Test video using Texture'),
          GstPlayer(
            pipeline: '''rtspsrc location=
              rtsp://192.168.17.30:8554/test  latency=0 !
            decodebin !
            glimagesink''',
          ),
        ],
      ),
    );
  }
}
