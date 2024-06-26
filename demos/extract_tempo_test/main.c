//compile with:
//gcc *.c ../../src/*.c ../offline/MKAiff.c

#include "../../BTT.h"
#include "../offline/MKAiff.h"

#define AUDIO_BUFFER_SIZE 64

double calcTempo(char *fname);

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  if(argc < 2)
    {
      fprintf(stderr, "Please specify an aiff or wav file you would like to process.\r\n"); exit(-1);
    }
 calcTempo(argv[1]);
}

double calcTempo(char *fname)
{
  long fpos=0;
  int plen;
  double bpm = 0;

  FILE *fp = fopen(fname, "rb");
  if(!fp) return -1;
    
  long packetsize = 1024;
  long outpos = 0;

  int   bStereo = 0;//IsStereo(fp);
  long sampleRate = 44100;//getSampleRate(fp);

  int frame_size = bStereo ? 4 : 2;

  short *packet = (short *)malloc(frame_size * packetsize);
  if(!packet)
  {
    fclose(fp);
    return -1;
  }

  float *buffer =  (float *)malloc(sizeof(float) * packetsize);
 
  BTT* btt =           btt_new(BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                               BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                               BTT_SUGGESTED_OSS_FILTER_ORDER,
                               BTT_SUGGESTED_OSS_LENGTH,
                               BTT_SUGGESTED_ONSET_THRESHOLD_N,
                               BTT_SUGGESTED_CBSS_LENGTH,
                               sampleRate,
                               BTT_DEFAULT_ANALYSIS_LATENCY_ONSET_ADJUSTMENT,
                               BTT_DEFAULT_ANALYSIS_LATENCY_BEAT_ADJUSTMENT
                               );

  btt_set_gaussian_tempo_histogram_decay(btt, 1);

  btt_set_tracking_mode(btt, BTT_ONSET_AND_TEMPO_TRACKING);

  float mult = 1.0f / 32768.0f;

  fseek(fp, 256, SEEK_SET);

  while(1)
  {
    plen = (int)fread(packet, frame_size, packetsize, fp);
    
    outpos = 0;
    
    if(bStereo)
    {
        for(unsigned int i=0; i<plen; i++)
        {
            buffer[i++] = packet[outpos] * mult;
            outpos+=2;
        }
        btt_process(btt, buffer, plen);
    }
    else
    {
        for(unsigned int i=0; i<plen; i++)
        {
            buffer[i++] = packet[outpos++] * mult;
        }
        
        btt_process(btt, buffer, plen);
    }
            
    fpos += plen;
    
    if(plen < packetsize) break;

    bpm = btt_get_tempo_bpm(btt);
    if(bpm != 0)
    {
          break;
    }
  }

  //NSLog(@"Tempo: %.1f", bpm);
  fprintf(stderr, "Tempo: %.1f\r\n", bpm);
  
  free(buffer);
  free(packet);
  fclose(fp);

  btt_destroy(btt);

  return bpm;
}



