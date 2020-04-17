/*
 * Copyright (C) 2018-2020 Stefan Westerfeld
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sfoutputstream.hh"
#include "utils.hh"

#include <math.h>
#include <assert.h>

using std::string;
using std::vector;

SFOutputStream::~SFOutputStream()
{
  close();
}

Error
SFOutputStream::open (const string& filename, int n_channels, int sample_rate, int bit_depth, size_t n_frames)
{
  assert (m_state == State::NEW);

  m_sample_rate = sample_rate;
  m_n_channels  = n_channels;

  SF_INFO sfinfo = {0,};
  sfinfo.samplerate = sample_rate;
  sfinfo.channels   = n_channels;

  if (bit_depth > 16)
    {
      sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
      m_bit_depth   = 24;
    }
  else
    {
      sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
      m_bit_depth   = 16;
    }

  m_sndfile = sf_open (filename.c_str(), SFM_WRITE, &sfinfo);
  int error = sf_error (m_sndfile);
  if (error)
    {
      string msg = sf_strerror (m_sndfile);
      if (m_sndfile)
        sf_close (m_sndfile);

      return Error (msg);
    }
  m_state       = State::OPEN;
  return Error::Code::NONE;
}

Error
SFOutputStream::close()
{
  if (m_state == State::OPEN)
    {
      assert (m_sndfile);
      if (sf_close (m_sndfile))
        return Error ("sf_close returned an error");

      m_state = State::CLOSED;
    }
  return Error::Code::NONE;
}

Error
SFOutputStream::write_frames (const vector<float>& samples)
{
  vector<int> isamples (samples.size());
  for (size_t i = 0; i < samples.size(); i++)
    {
      const double norm      =  0x80000000LL;
      const double min_value = -0x80000000LL;
      const double max_value =  0x7FFFFFFF;

      isamples[i] = lrint (bound<double> (min_value, samples[i] * norm, max_value));
    }

  sf_count_t frames = samples.size() / m_n_channels;
  sf_count_t count = sf_writef_int (m_sndfile, &isamples[0], frames);

  if (sf_error (m_sndfile))
    return Error (sf_strerror (m_sndfile));

  if (count != frames)
    return Error ("writing sample data failed: short write");

  return Error::Code::NONE;
}

int
SFOutputStream::bit_depth() const
{
  return m_bit_depth;
}

int
SFOutputStream::sample_rate() const
{
  return m_sample_rate;
}

int
SFOutputStream::n_channels() const
{
  return m_n_channels;
}