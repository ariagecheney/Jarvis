#include "audio.h"

Audio::Audio(QString filename)
{
    if(filename.isEmpty()) filename="Test";
    if(!isValuable()){
        qDebug()<<"Library is not avaliable";
        return;
    }
    this->filename=filename;
    file.setFileName(this->filename);
}

bool Audio::isValuable()
{
    unsigned version = avcodec_version();
    qDebug()<<version;
    if(version) return true;
    else return false;
}
/*   * 最简单的基于FFmpeg的音频解码器
     * Simplest FFmpeg Audio Decoder
     * 本程序可以将音频码流（mp3，AAC等）解码为PCM采样数据。
     * 是最简单的FFmpeg音频解码方面的教程。
     * 通过学习本例子可以了解FFmpeg的解码流程。
     * This software decode audio streams (AAC,MP3 ...) to PCM data.
     * Suitable for beginner of FFmpeg.**/
bool Audio::decoder(QString input,QString output)
{
   //FILE *pFile=fopen("output.pcm", "wb");
   QFile outputFile(output);
   outputFile.open(QIODevice::Append|QIODevice::WriteOnly);
   QTextStream a(&outputFile);
   // char url[]="skycity1.mp3";
   const int count=input.count();
   char url[count];
   QByteArray ba=input.toLocal8Bit();
   strcpy(url,ba);
   avformat_network_init();
   pFormatCtx = avformat_alloc_context();
   //Open
   if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){
       qDebug("Couldn't open input stream.\n");
       return -1;
   }
   // Retrieve stream information
   if(avformat_find_stream_info(pFormatCtx,NULL)<0){
       qDebug("Couldn't find stream information.\n");
       return -1;
   }
   // Dump valid information onto standard error
   av_dump_format(pFormatCtx, 0, url, false);
   // Find the first audio stream
   audioStream=-1;
   for(i=0; i < pFormatCtx->nb_streams; i++)
       if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
           audioStream=i;
           break;
   }
   if(audioStream == -1){
       qDebug("Didn't find a audio stream.\n");
       return -1;
   }
   // Get a pointer to the codec context for the audio stream
   pCodecCtx=pFormatCtx->streams[audioStream]->codec;
   // Find the decoder for the audio stream
   pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
   if(pCodec==NULL){
       qDebug("Codec not found.\n");
       return -1;
   }
   // Open codec
   if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
       qDebug("Could not open codec.\n");
       return -1;
   }
   packet=(AVPacket *)av_malloc(sizeof(AVPacket));
   //av_init_packet(packet);
   //Out Audio Param
   uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
   //nb_samples: AAC-1024 MP3-1152
   int out_nb_samples=pCodecCtx->frame_size;
   AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
   int out_sample_rate=44100;
   int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
   //Out Buffer Size
   int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels,out_nb_samples,out_sample_fmt, 1);
   out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
   pFrame=av_frame_alloc();
   //FIX:Some Codec's Context Information is missing
   in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
   //Swr
   au_convert_ctx = swr_alloc();
   au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
            in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
   swr_init(au_convert_ctx);
   while(av_read_frame(pFormatCtx, packet)>=0){
       if(packet->stream_index==audioStream){
           ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
           if ( ret < 0 ){
               qDebug("Error in decoding audio frame.\n");
               return -1;
           }
           if ( got_picture > 0 ){
               swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,
                           (const uint8_t **)pFrame->data , pFrame->nb_samples);
               qDebug("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
               //Write PCM
               //fwrite(out_buffer, 1, out_buffer_size, pFile);
               a<<out_buffer;
               index++;
           }
       }
       av_free_packet(packet);
   }
   swr_free(&au_convert_ctx);
   //fclose(pFile);
   av_free(out_buffer);
   // Close the codec
   avcodec_close(pCodecCtx);
   // Close the video file
   avformat_close_input(&pFormatCtx);
   return 0;
}

void Audio::run()
{
    qDebug()<<"run it";
    decoder(filename,qApp->applicationDirPath()+QDir::separator()+"a.pcm");
    qDebug()<<"Done";
}
