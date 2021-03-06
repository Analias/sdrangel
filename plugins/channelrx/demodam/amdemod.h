///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015 Edouard Griffiths, F4EXB.                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_AMDEMOD_H
#define INCLUDE_AMDEMOD_H

#include <dsp/basebandsamplesink.h>
#include <QMutex>
#include <vector>
#include "dsp/nco.h"
#include "dsp/interpolator.h"
#include "dsp/movingaverage.h"
#include "dsp/agc.h"
#include "dsp/bandpass.h"
#include "audio/audiofifo.h"
#include "util/message.h"

class AMDemod : public BasebandSampleSink {
	Q_OBJECT
public:
	AMDemod();
	~AMDemod();

	void configure(MessageQueue* messageQueue, Real rfBandwidth, Real volume, Real squelch, bool audioMute, bool bandpassEnable);

	virtual void feed(const SampleVector::const_iterator& begin, const SampleVector::const_iterator& end, bool po);
	virtual void start();
	virtual void stop();
	virtual bool handleMessage(const Message& cmd);

	double getMagSq() const { return m_magsq; }
	bool getSquelchOpen() const { return m_squelchOpen; }

	void getMagSqLevels(double& avg, double& peak, int& nbSamples)
	{
	    avg = m_magsqCount == 0 ? 1e-10 : m_magsqSum / m_magsqCount;
	    peak = m_magsqPeak == 0.0 ? 1e-10 : m_magsqPeak;
	    nbSamples = m_magsqCount == 0 ? 1 : m_magsqCount;
	    m_magsqSum = 0.0f;
        m_magsqPeak = 0.0f;
        m_magsqCount = 0;
	}

private:
	class MsgConfigureAMDemod : public Message {
		MESSAGE_CLASS_DECLARATION

	public:
		Real getRFBandwidth() const { return m_rfBandwidth; }
		Real getVolume() const { return m_volume; }
		Real getSquelch() const { return m_squelch; }
		bool getAudioMute() const { return m_audioMute; }
		bool getBandpassEnable() const { return m_bandpassEnable; }

		static MsgConfigureAMDemod* create(Real rfBandwidth, Real volume, Real squelch, bool audioMute, bool bandpassEnable)
		{
			return new MsgConfigureAMDemod(rfBandwidth, volume, squelch, audioMute, bandpassEnable);
		}

	private:
		Real m_rfBandwidth;
		Real m_volume;
		Real m_squelch;
		bool m_audioMute;
		bool m_bandpassEnable;

		MsgConfigureAMDemod(Real rfBandwidth, Real volume, Real squelch, bool audioMute, bool bandpassEnable) :
			Message(),
			m_rfBandwidth(rfBandwidth),
			m_volume(volume),
			m_squelch(squelch),
			m_audioMute(audioMute),
			m_bandpassEnable(bandpassEnable)
		{ }
	};

	struct AudioSample {
		qint16 l;
		qint16 r;
	};
	typedef std::vector<AudioSample> AudioVector;

	enum RateState {
		RSInitialFill,
		RSRunning
	};

	struct Config {
		int m_inputSampleRate;
		qint64 m_inputFrequencyOffset;
		Real m_rfBandwidth;
		Real m_squelch;
		Real m_volume;
		quint32 m_audioSampleRate;
		bool m_audioMute;
		bool m_bandpassEnable;

		Config() :
			m_inputSampleRate(-1),
			m_inputFrequencyOffset(0),
			m_rfBandwidth(-1),
			m_squelch(0),
			m_volume(0),
			m_audioSampleRate(0),
			m_audioMute(false),
			m_bandpassEnable(false)
		{ }
	};

	Config m_config;
	Config m_running;

	NCO m_nco;
	Interpolator m_interpolator;
	Real m_interpolatorDistance;
	Real m_interpolatorDistanceRemain;

	Real m_squelchLevel;
	uint32_t m_squelchCount;
	bool m_squelchOpen;
	double m_magsq;
	double m_magsqSum;
	double m_magsqPeak;
	int  m_magsqCount;

	MovingAverage<double> m_movingAverage;
	SimpleAGC m_volumeAGC;
    Bandpass<Real> m_bandpass;

	AudioVector m_audioBuffer;
	uint m_audioBufferFill;

	AudioFifo m_audioFifo;
	QMutex m_settingsMutex;

	void apply();

	void processOneSample(Complex &ci)
	{
        Real magsq = ci.real() * ci.real() + ci.imag() * ci.imag();
        magsq /= (1<<30);
        m_movingAverage.feed(magsq);
        m_magsq = m_movingAverage.average();
        m_magsqSum += magsq;

        if (magsq > m_magsqPeak)
        {
            m_magsqPeak = magsq;
        }

        m_magsqCount++;

        if (m_magsq >= m_squelchLevel)
        {
            if (m_squelchCount <= m_running.m_audioSampleRate / 10)
            {
                if (m_squelchCount == m_running.m_audioSampleRate / 20) {
                    m_volumeAGC.fill(1.0);
                }

                m_squelchCount++;
            }
        }
        else
        {
            if (m_squelchCount > 1)
            {
                m_squelchCount -= 2;
            }
        }

        qint16 sample;

        if ((m_squelchCount >= m_running.m_audioSampleRate / 20) && !m_running.m_audioMute)
        {
            Real demod = sqrt(magsq);
            m_volumeAGC.feed(demod);
            demod /= m_volumeAGC.getValue();

            if (m_running.m_bandpassEnable)
            {
                demod = m_bandpass.filter(demod);
                demod /= 301.0f;
            }

            Real attack = (m_squelchCount - 0.05f * m_running.m_audioSampleRate) / (0.05f * m_running.m_audioSampleRate);
            sample = (0.5 - demod) * attack * 2048 * m_running.m_volume;

            m_squelchOpen = true;
        }
        else
        {
            sample = 0;
            m_squelchOpen = false;
        }

        m_audioBuffer[m_audioBufferFill].l = sample;
        m_audioBuffer[m_audioBufferFill].r = sample;
        ++m_audioBufferFill;

        if (m_audioBufferFill >= m_audioBuffer.size())
        {
            uint res = m_audioFifo.write((const quint8*)&m_audioBuffer[0], m_audioBufferFill, 10);

            if (res != m_audioBufferFill)
            {
                qDebug("AMDemod::feed: %u/%u audio samples written", res, m_audioBufferFill);
            }

            m_audioBufferFill = 0;
        }
	}
};

#endif // INCLUDE_AMDEMOD_H
