/*
  Copyright 2010 Paulo Henrique Silva <ph.silva@gmail.com>

  This file is part of movie2dng.

  movie2dng is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  movie2dng is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with movie2dng.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "movie.h"

MovieIterator::MovieIterator(Movie* movie, unsigned int start, unsigned int end)
  : movie(movie), start(start), end(end), currentFrame(0)
{
  if (start == 0) start = 1;
  if (end == 0 || end > movie->nFrames()) end = movie->nFrames();
}

MovieIterator::~MovieIterator() {
  av_free_packet(&packet);
}

bool MovieIterator::hasNext() const {
  return currentFrame < end;
}

bool MovieIterator::next(unsigned int* frame, void** data, unsigned int* size) {
  if (currentFrame >= end)
    return false;

  if (currentFrame > 0)
    av_free_packet(&packet);

  int res = av_read_frame(movie->ctx, &packet);
  if (res >= 0) {
    if (frame) *frame = currentFrame+1;
    if (data) *data = packet.data;
    if (size) *size = packet.size;
    currentFrame++;
    return true;
  }

  return false;
}

Movie::Movie(): ctx(NULL) {
}

int Movie::open(const string& filename) {

  av_register_all();

#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(52, 111, 0)
  int err=av_open_input_file(&ctx, filename.c_str(), NULL, 0, NULL);
#else
  int err=avformat_open_input(&ctx, filename.c_str(), NULL, NULL);
#endif
  
  if (err) {
    fprintf(stderr, "ERROR: Cannot open file: '%s'.\n", filename.c_str());
    return 0;
  }

#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 3, 0)
  err=av_find_stream_info(ctx);
#else
  err=avformat_find_stream_info(ctx,NULL);
#endif

  if (err < 0) {
    fprintf(stderr, "ERROR: Cannot find stream info.\n");
    return 0;
  }
  
  AVCodecContext* codecCtx = ctx->streams[0]->codec;
  AVCodec* codec = avcodec_find_decoder(codecCtx->codec_id);
  if (!codec) {
    fprintf(stderr, "ERROR: Cannot find codec.");
    return 0;
  }
  
#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 3, 0)
  err=avcodec_open(codecCtx, codec);
#else
  err=avcodec_open2(codecCtx, codec,NULL);
#endif

  if (err < 0) {
    fprintf(stderr, "ERROR: Cannot open codec.");
    return 0;
  }

  return 1;
}

Movie::~Movie() {
}

unsigned int Movie::nFrames() const {
  return ctx->streams[0]->nb_frames;
}

MovieIterator Movie::iterator(unsigned int start, unsigned int end) {
  return MovieIterator(this, start, end);
}
