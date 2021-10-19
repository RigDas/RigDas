// Copyright 2019 Google LLC, Andrew Hines
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "visqol_api.h"

#include "similarity_result.pb.h"  // Generated by cc_proto_library rule
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "audio_signal.h"
#include "commandline_parser.h"
#include "conformance.h"
#include "file_path.h"
#include "misc_audio.h"
#include "visqol_config.pb.h"  // Generated by cc_proto_library rule

namespace Visqol {
namespace {

const size_t kSampleRate = 48000;
const size_t kUnsupportedSampleRate = 44100;
const double kTolerance = 0.0001;
const char kContrabassoonRef[] =
  "testdata/conformance_testdata_subset/contrabassoon48_stereo.wav";
const char kContrabassoonDeg[] =
  "testdata/conformance_testdata_subset/contrabassoon48_stereo_24kbps_aac.wav";
const char kCleanSpeechRef[] =
  "testdata/clean_speech/CA01_01.wav";
const char kCleanSpeechDeg[] =
  "testdata/clean_speech/transcoded_CA01_01.wav";
const char kNoSampleRateErrMsg[] =
    "INVALID_ARGUMENT: Audio info must be supplied for config.";
const char kNonExistantModelFile[] = "non_existant.txt";
const char kNonExistantModelFileErrMsg[] =
    "INVALID_ARGUMENT: Failed to load the SVR model file: non_existant.txt";
const char kNon48kSampleRateErrMsg[] =
    "INVALID_ARGUMENT: Currently, 48k is the only sample rate supported by "
    "ViSQOL Audio. See README for details of overriding.";

// These values match the known version.
const double kContrabassoonVnsim = 0.90758;
const double kContrabassoonFvnsim[] = {
  0.884680, 0.925437, 0.980274, 0.996635, 0.996060, 0.979772, 0.984409,
  0.986112, 0.977326, 0.982975, 0.958038, 0.971650, 0.964743, 0.959870,
  0.959018, 0.954554, 0.967928, 0.962373, 0.940116, 0.865323, 0.851010,
  0.856138, 0.852182, 0.825574, 0.791404, 0.805591, 0.779993, 0.789653,
  0.805530, 0.786122, 0.823594, 0.878549
};

const double kPerfectScore = 5.0;

/**
 *  Happy path test for the ViSQOL API with a model file specified.
 */
TEST(VisqolApi, happy_path_specified_model) {
  // Build reference and degraded Spans.
  AudioSignal ref_signal = MiscAudio::LoadAsMono(FilePath(kContrabassoonRef));
  AudioSignal deg_signal = MiscAudio::LoadAsMono(FilePath(kContrabassoonDeg));
  auto ref_data = ref_signal.data_matrix.ToVector();
  auto deg_data = deg_signal.data_matrix.ToVector();
  auto ref_span = absl::Span<double>(ref_data);
  auto deg_span = absl::Span<double>(deg_data);

  // Now call the API, specifying the model file location.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);

  VisqolApi visqol;
  auto create_status = visqol.Create(config);
  ASSERT_TRUE(create_status.ok());
  auto result = visqol.Measure(ref_span, deg_span);

  ASSERT_TRUE(result.ok());
  auto sim_result = result.value();
  ASSERT_NEAR(kConformanceContrabassoon24aac, sim_result.moslqo(), kTolerance);
  ASSERT_NEAR(kContrabassoonVnsim, sim_result.vnsim(), kTolerance);
  for (int i = 0; i < sim_result.fvnsim_size(); i++) {
    ASSERT_NEAR(kContrabassoonFvnsim[i], sim_result.fvnsim(i), kTolerance);
  }
}

/**
 *  Happy path test for the ViSQOL API with the default model file used.
 */
TEST(VisqolApi, happy_path_default_model) {
  // Build reference and degraded Spans.
  AudioSignal ref_signal = MiscAudio::LoadAsMono(FilePath(kContrabassoonRef));
  AudioSignal deg_signal = MiscAudio::LoadAsMono(FilePath(kContrabassoonDeg));
  auto ref_data = ref_signal.data_matrix.ToVector();
  auto deg_data = deg_signal.data_matrix.ToVector();
  auto ref_span = absl::Span<double>(ref_data);
  auto deg_span = absl::Span<double>(deg_data);

  // Now call the API without specifying the model file location.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);

  VisqolApi visqol;
  auto create_status = visqol.Create(config);
  ASSERT_TRUE(create_status.ok());
  auto result = visqol.Measure(ref_span, deg_span);

  ASSERT_TRUE(result.ok());
  auto sim_result = result.value();
  ASSERT_NEAR(kConformanceContrabassoon24aac, sim_result.moslqo(), kTolerance);
  ASSERT_NEAR(kContrabassoonVnsim, sim_result.vnsim(), kTolerance);
  for (int i = 0; i < sim_result.fvnsim_size(); i++) {
    ASSERT_NEAR(kContrabassoonFvnsim[i], sim_result.fvnsim(i), kTolerance);
  }
}

/**
 *  Test calling the ViSQOL API without sample rate data for the input signals.
 */

TEST(VisqolApi, no_sample_rate_info) {
  // Create the API with no sample rate data.
  VisqolConfig config;
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);

  VisqolApi visqol;
  auto result = visqol.Create(config);

  ASSERT_TRUE(!result.ok());
  ASSERT_EQ(kNoSampleRateErrMsg, result.ToString());
}

/**
 *  Test calling the ViSQOL API with an unsupported sample rate and the 'allow
 *  unsupported sample rates' override set to false.
 */
TEST(VisqolApi, unsupported_sample_rate_no_override) {
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kUnsupportedSampleRate);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);

  VisqolApi visqol;
  auto result = visqol.Create(config);

  ASSERT_FALSE(result.ok());
  ASSERT_EQ(kNon48kSampleRateErrMsg, result.ToString());
}

/**
 *  Test calling the ViSQOL API with an unsupported sample rate and the 'allow
 *  unsupported sample rates' override set to true.
 */
TEST(VisqolApi, unsupported_sample_rate_with_override) {
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kUnsupportedSampleRate);
  config.mutable_options()->set_allow_unsupported_sample_rates(true);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);

  VisqolApi visqol;
  auto result = visqol.Create(config);

  ASSERT_TRUE(result.ok());
}

/**
 *  Test calling the ViSQOL API with a model file specified that does not exist.
 */
TEST(VisqolApi, non_existant_mode_file) {
  // Create the API with no sample rate data.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);
  config.mutable_options()->set_svr_model_path(kNonExistantModelFile);

  VisqolApi visqol;
  auto result = visqol.Create(config);
  ASSERT_TRUE(!result.ok());
  ASSERT_EQ(kNonExistantModelFileErrMsg, result.ToString());
}

/**
 * Confirm that when running the ViSQOL API with speech mode disabled (even
 * with the 'use unscaled mapping' bool set to true), the input files will be
 * compared as audio.
 */
TEST(VisqolApi, speech_mode_disabled) {
  // Build reference and degraded Spans.
  AudioSignal ref_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechRef));
  AudioSignal deg_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechDeg));
  auto ref_data = ref_signal.data_matrix.ToVector();
  auto deg_data = deg_signal.data_matrix.ToVector();
  auto ref_span = absl::Span<double>(ref_data);
  auto deg_span = absl::Span<double>(deg_data);

  // Now call the API, specifying the model file location.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);
  config.mutable_options()->set_use_speech_scoring(false);
  config.mutable_options()->set_use_unscaled_speech_mos_mapping(true);

  VisqolApi visqol;
  auto create_status = visqol.Create(config);
  ASSERT_TRUE(create_status.ok());
  auto result = visqol.Measure(ref_span, deg_span);

  ASSERT_TRUE(result.ok());
  auto sim_result = result.value();
  ASSERT_NEAR(kCA01_01AsAudio, sim_result.moslqo(), kTolerance);
}

/**
 * Test the ViSQOL API running in speech mode. Use the same file for both the
 * reference and degraded signals and run in scaled mode. A perfect score of
 * 5.0 is expected.
 */
TEST(VisqolApi, speech_mode_with_scaled_mapping) {
  // Build reference and degraded Spans.
  AudioSignal ref_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechRef));
  AudioSignal deg_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechRef));
  auto ref_data = ref_signal.data_matrix.ToVector();
  auto deg_data = deg_signal.data_matrix.ToVector();
  auto ref_span = absl::Span<double>(ref_data);
  auto deg_span = absl::Span<double>(deg_data);

  // Now call the API, specifying the model file location.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);
  config.mutable_options()->set_use_speech_scoring(true);
  config.mutable_options()->set_use_unscaled_speech_mos_mapping(false);

  VisqolApi visqol;
  auto create_status = visqol.Create(config);
  ASSERT_TRUE(create_status.ok());
  auto result = visqol.Measure(ref_span, deg_span);

  ASSERT_TRUE(result.ok());
  auto sim_result = result.value();
  ASSERT_NEAR(kPerfectScore, sim_result.moslqo(), kTolerance);
}

/**
 * Test the ViSQOL API running in speech mode. Use the same file for both the
 * reference and degraded signals and run in unscaled mode. A score of 4.x is
 * expected.
 */
TEST(VisqolApi, speech_mode_with_unscaled_mapping) {
  // Build reference and degraded Spans.
  AudioSignal ref_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechRef));
  AudioSignal deg_signal = MiscAudio::LoadAsMono(FilePath(kCleanSpeechRef));
  auto ref_data = ref_signal.data_matrix.ToVector();
  auto deg_data = deg_signal.data_matrix.ToVector();
  auto ref_span = absl::Span<double>(ref_data);
  auto deg_span = absl::Span<double>(deg_data);

  // Now call the API, specifying the model file location.
  VisqolConfig config;
  config.mutable_audio()->set_sample_rate(kSampleRate);
  config.mutable_options()->set_svr_model_path(
      FilePath::currentWorkingDir() + kDefaultAudioModelFile);
  config.mutable_options()->set_use_speech_scoring(true);
  config.mutable_options()->set_use_unscaled_speech_mos_mapping(true);

  VisqolApi visqol;
  auto create_status = visqol.Create(config);
  ASSERT_TRUE(create_status.ok());
  auto result = visqol.Measure(ref_span, deg_span);

  ASSERT_TRUE(result.ok());
  auto sim_result = result.value();
  ASSERT_NEAR(kCA01_01UnscaledPerfectScore, sim_result.moslqo(), kTolerance);
}

}  // namespace
}  // namespace Visqol