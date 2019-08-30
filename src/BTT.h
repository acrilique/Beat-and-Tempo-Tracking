/*----------------------------------------------------------------------
         _          __________                              _,
     _.-(_)._     ."          ".      .--""--.          _.-{__}-._
   .'________'.   | .--------. |    .'        '.      .:-'`____`'-:.
  [____________] /` |________| `\  /   .'``'.   \    /_.-"`_  _`"-._\
  /  / .\/. \  \|  / / .\/. \ \  ||  .'/.\/.\'.  |  /`   / .\/. \   `\
  |  \__/\__/  |\_/  \__/\__/  \_/|  : |_/\_| ;  |  |    \__/\__/    |
  \            /  \            /   \ '.\    /.' / .-\                /-.
  /'._  --  _.'\  /'._  --  _.'\   /'. `'--'` .'\/   '._-.__--__.-_.'   \
 /_   `""""`   _\/_   `""""`   _\ /_  `-./\.-'  _\'.    `""""""""`    .'`\
(__/    '|    \ _)_|           |_)_/            \__)|        '       |   |
  |_____'|_____|   \__________/   |              |;`_________'________`;-'
   '----------'    '----------'   '--------------'`--------------------`
------------------------------------------------------------------------
  ___  _    _        _        _
 / _ \| |__| |_ __ _(_)_ _   | |_
| (_) | '_ \  _/ _` | | ' \ _| ' \
 \___/|_.__/\__\__,_|_|_||_(_)_||_|
 
------------------------------------------------------------------------

  Made by Michael Krzyzaniak
 
   Notes. Went from about 30% realtime to about 16% realtime when switching from slowsin to fastsin.
 
  Version:
    1.0

----------------------------------------------------------------------*/
#ifndef __BTT__
#define __BTT__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "DFT.h"

/*--------------------------------------------------------------------*/
typedef enum
{
  BTT_ONSET_TRACKING,
  BTT_ONSET_AND_TEMPO_TRACKING,
  BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING,
}btt_tracking_mode_t;

/*--------------------------------------------------------------------*/
#define BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN         1024  // fft size
#define BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP     8     // hop size will be len / overlap
#define BTT_SUGGESTED_OSS_FILTER_ORDER               15    // this will delay the signal by about 7 samples
#define BTT_SUGGESTED_OSS_LENGTH                     1024  // enough to hold 3 seconds at 44.1kHz and hop of 128
#define BTT_SUGGESTED_ONSET_THRESHOLD_N              1024  // size of moving average of oss to see if onset occured
#define BTT_SUGGESTED_SAMPLE_RATE                    44100 // audio sample rate
#define BTT_SUGGESTED_CBSS_LENGTH                    1024  // should be at least a little bigger than the slowest tempo lag in oss samples

/*--------------------------------------------------------------------*/
#define BTT_DEFAULT_MIN_TEMPO                        50    // BPM
#define BTT_DEFAULT_MAX_TEMPO                        200   // BPM
#define BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA       0     // COMPRESSED(spectrum) = log(1+gamma|spectrum|) / log(1+gamma)
#define BTT_DEFAULT_AUTOCORRELATION_EXPONENT         0.5   // for generalized autocorrelation
#define BTT_DEFAULT_NUM_TEMPO_CANDIDATES             10    //
#define BTT_DEFAULT_TRACKING_MODE                    BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING
#define BTT_DEFAULT_OSS_FILTER_CUTOFF                10    // Hz
#define BTT_DEFAULT_USE_AMP_NORMALIZATION            0     // false
#define BTT_DEFAULT_ONSET_TREHSHOLD                  1     // std devs above mean OSS signal
#define BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD     -74   // dB per freq bin
#define BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN   100   // supress harmonics by favoring tempos closer to 100
#define BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH  75    // oss samples starndard deviation
#define BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY   0.999 //
#define BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH   5     // oss samples starndard deviation
#define BTT_DEFAULT_CBSS_ALPHA                       0.9   // 90% old, 10 percent new
#define BTT_DEFAULT_CBSS_ETA                         300   // width of gaussian window. Larger number is narrower window
#define BTT_DEFAULT_BEAT_PREDICTION_ADJUSTMENT       10    // oss samples earlier than detected
#define BTT_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX     20    //
#define BTT_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH    10    // oss samples
#define BTT_DEFAULT_IGNORE_SPURIOUS_BEATS_DURATION   40    // percent of beat at current tempo

#define BTT_DEFAULT_XCORR_NUM_PULSES                 8     //
#define BTT_DEFAULT_XCORR_PULSE_LOCATIONS            {0, 1, 1.5, 2, 3, 4, 4.5, 6}
#define BTT_DEFAULT_XCORR_PULSE_VALUES               {2.0, 1.0, 0.5, 1.5, 1.5, 0.5, 0.5, 0.5}

/*--------------------------------------------------------------------*/
typedef struct Opaque_BTT_Struct BTT;

typedef void (*btt_onset_callback_t)             (void* SELF, unsigned long long sample_time);
typedef void (*btt_tempo_callback_t)             (void* SELF, unsigned long long sample_time, double bpm, int beat_period_in_samples);
typedef void (*btt_beat_callback_t)              (void* SELF, unsigned long long sample_time);

/*--------------------------------------------------------------------*/
BTT*      btt_new                                (int spectral_flux_stft_len, int spectral_flux_stft_overlap,
                                                  int oss_filter_order      , int oss_length,
                                                  int cbss_length           , int onset_threshold_len, double sample_rate);
BTT*      btt_new_default                        ();
BTT*      btt_destroy                            (BTT* self);
void      btt_process                            (BTT* self, dft_sample_t* input, int num_samples);
double    btt_get_sample_rate                    (BTT* self);

/* onset detection adjustments */
void      btt_set_use_amplitude_normalization    (BTT* self, int use);
int       btt_get_use_amplitude_normalization    (BTT* self);
void      btt_set_spectral_compression_gamma     (BTT* self, double gamma);
double    btt_get_spectral_compression_gamma     (BTT* self);
void      btt_set_oss_filter_cutoff              (BTT* self, double Hz);
double    btt_get_oss_filter_cutoff              (BTT* self);
void      btt_set_onset_threshold                (BTT* self, double num_std_devs);
double    btt_get_onset_threshold                (BTT* self);
void      btt_set_noise_cancellation_threshold   (BTT* self, double dB /*probably negative*/);
double    btt_get_noise_cancellation_threshold   (BTT* self);

/* tempo tracking adjustments */
void      btt_set_autocorrelation_exponent       (BTT* self, double exponent);
double    btt_get_autocorrelation_exponent       (BTT* self);
void      btt_set_min_tempo                      (BTT* self, double min_tempo);
double    btt_get_min_tempo                      (BTT* self);
void      btt_set_max_tempo                      (BTT* self, double max_tempo);
double    btt_get_max_tempo                      (BTT* self);
void      btt_set_num_tempo_candidates           (BTT* self, int num_candidates);
int       btt_get_num_tempo_candidates           (BTT* self);
void      btt_set_gaussian_tempo_histogram_decay (BTT* self, double coefficient);
double    btt_get_gaussian_tempo_histogram_decay (BTT* self);
void      btt_set_gaussian_tempo_histogram_width (BTT* self, double width);
double    btt_get_gaussian_tempo_histogram_width (BTT* self);
void      btt_set_log_gaussian_tempo_weight_mean (BTT* self, double bpm);
double    btt_get_log_gaussian_tempo_weight_mean (BTT* self);
void      btt_set_log_gaussian_tempo_weight_width(BTT* self, double bpm);
double    btt_get_log_gaussian_tempo_weight_width(BTT* self);

/* beat tracking adjustments */
void      btt_set_cbss_alpha                     (BTT* self, double alpha);
double    btt_get_cbss_alpha                     (BTT* self);
void      btt_set_cbss_eta                       (BTT* self, double eta);
double    btt_get_cbss_eta                       (BTT* self);
void      btt_set_beat_prediction_adjustment     (BTT* self, int oss_samples_earlier);
int       btt_get_beat_prediction_adjustment     (BTT* self);
void      btt_set_predicted_beat_trigger_index   (BTT* self, int index);
int       btt_get_predicted_beat_trigger_index   (BTT* self);
void      btt_set_predicted_beat_gaussian_width  (BTT* self, double width);
double    btt_get_predicted_beat_gaussian_width  (BTT* self);
void      btt_set_ignore_spurious_beats_duration (BTT* self, double percent_of_tempo);
double    btt_get_ignore_spurious_beats_duration (BTT* self);

void                 btt_set_tracking_mode            (BTT* self, btt_tracking_mode_t mode);
btt_tracking_mode_t  btt_get_tracking_mode            (BTT* self);
void                 btt_set_onset_tracking_callback  (BTT* self, btt_onset_callback_t callback, void* callback_self);
btt_onset_callback_t btt_get_onset_tracking_callback  (BTT* self, void** returned_callback_self);
void                 btt_set_tempo_tracking_callback  (BTT* self, btt_tempo_callback_t callback, void* callback_self);
btt_tempo_callback_t btt_get_tempo_tracking_callback  (BTT* self, void** returned_callback_self);
void                 btt_set_beat_tracking_callback   (BTT* self, btt_beat_callback_t callback, void* callback_self);
btt_beat_callback_t  btt_get_beat_tracking_callback   (BTT* self, void** returned_callback_self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __BTT__