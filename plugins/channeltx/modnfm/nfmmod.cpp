///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016 Edouard Griffiths, F4EXB                                   //
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

#include <QTime>
#include <QDebug>
#include <QMutexLocker>
#include <stdio.h>
#include <complex.h>
#include <dsp/upchannelizer.h>
#include "dsp/dspengine.h"
#include "dsp/pidcontroller.h"
#include "nfmmod.h"

MESSAGE_CLASS_DEFINITION(NFMMod::MsgConfigureNFMMod, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgConfigureFileSourceName, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgConfigureFileSourceSeek, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgConfigureAFInput, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgConfigureFileSourceStreamTiming, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgReportFileSourceStreamData, Message)
MESSAGE_CLASS_DEFINITION(NFMMod::MsgReportFileSourceStreamTiming, Message)


NFMMod::NFMMod() :
    m_audioFifo(4, 48000),
	m_settingsMutex(QMutex::Recursive),
	m_fileSize(0),
	m_recordLength(0),
	m_sampleRate(48000),
	m_afInput(NFMModInputNone)
{
	setObjectName("AMMod");

	m_config.m_outputSampleRate = 48000;
	m_config.m_inputFrequencyOffset = 0;
	m_config.m_rfBandwidth = 12500;
	m_config.m_afBandwidth = 3000;
	m_config.m_modFactor = 20;
	m_config.m_audioSampleRate = DSPEngine::instance()->getAudioSampleRate();

	apply();

	//m_audioBuffer.resize(1<<14);
	//m_audioBufferFill = 0;

	m_movingAverage.resize(16, 0);
	m_volumeAGC.resize(4096, 0.003, 0);
	m_magsq = 0.0;

	m_toneNco.setFreq(1000.0, m_config.m_audioSampleRate);
	DSPEngine::instance()->addAudioSource(&m_audioFifo);
}

NFMMod::~NFMMod()
{
    DSPEngine::instance()->removeAudioSource(&m_audioFifo);
}

void NFMMod::configure(MessageQueue* messageQueue,
		Real rfBandwidth,
		Real afBandwidth,
		float modFactor,
		int volumeTenths,
		bool audioMute,
		bool playLoop)
{
	Message* cmd = MsgConfigureNFMMod::create(rfBandwidth, afBandwidth, modFactor, volumeTenths, audioMute, playLoop);
	messageQueue->push(cmd);
}

void NFMMod::pull(Sample& sample)
{
	Complex ci;
	Real t;

	m_settingsMutex.lock();

    if (m_interpolatorDistance > 1.0f) // decimate
    {
        pullAF(t);
        m_modSample.real(((t+1.0f) * m_running.m_modFactor * 16384.0f)); // modulate and scale zero frequency carrier
        m_modSample.imag(0.0f);

        while (!m_interpolator.decimate(&m_interpolatorDistanceRemain, m_modSample, &ci))
        {
            pullAF(t);
            m_modSample.real(((t+1.0f) * m_running.m_modFactor * 16384.0f)); // modulate and scale zero frequency carrier
            m_modSample.imag(0.0f);
        }
    }
    else
    {
        if (m_interpolator.interpolate(&m_interpolatorDistanceRemain, m_modSample, &ci))
        {
            pullAF(t);
            m_modSample.real(((t+1.0f) * m_running.m_modFactor * 16384.0f)); // modulate and scale zero frequency carrier
            m_modSample.imag(0.0f);
        }
    }

    m_interpolatorDistanceRemain += m_interpolatorDistance;

    ci *= m_carrierNco.nextIQ(); // shift to carrier frequency

    m_settingsMutex.unlock();

    Real magsq = ci.real() * ci.real() + ci.imag() * ci.imag();
	magsq /= (1<<30);
	m_movingAverage.feed(magsq);
	m_magsq = m_movingAverage.average();

	sample.m_real = (FixReal) ci.real();
	sample.m_imag = (FixReal) ci.imag();
}

void NFMMod::pullAF(Real& sample)
{
    int16_t audioSample[2];

    switch (m_afInput)
    {
    case NFMModInputTone:
        sample = m_toneNco.next();
        break;
    case NFMModInputFile:
        // sox f4exb_call.wav --encoding float --endian little f4exb_call.raw
        // ffplay -f f32le -ar 48k -ac 1 f4exb_call.raw
        if (m_ifstream.is_open())
        {
            if (m_ifstream.eof())
            {
            	if (m_running.m_playLoop)
            	{
                    m_ifstream.clear();
                    m_ifstream.seekg(0, std::ios::beg);
            	}
            }

            if (m_ifstream.eof())
            {
            	sample = 0.0f;
            }
            else
            {
            	m_ifstream.read(reinterpret_cast<char*>(&sample), sizeof(Real));
            }
        }
        else
        {
            sample = 0.0f;
        }
        break;
    case NFMModInputAudio:
        m_audioFifo.read(reinterpret_cast<quint8*>(audioSample), 1, 10);
        sample = ((audioSample[0] + audioSample[1]) * m_running.m_volumeFactor) / 6553600.0f;
        break;
    case NFMModInputNone:
    default:
        sample = 0.0f;
        break;
    }
}

void NFMMod::start()
{
	qDebug() << "NFMMod::start: m_outputSampleRate: " << m_config.m_outputSampleRate
			<< " m_inputFrequencyOffset: " << m_config.m_inputFrequencyOffset;

	m_audioFifo.clear();
}

void NFMMod::stop()
{
}

bool NFMMod::handleMessage(const Message& cmd)
{
	qDebug() << "NFMMod::handleMessage";

	if (UpChannelizer::MsgChannelizerNotification::match(cmd))
	{
		UpChannelizer::MsgChannelizerNotification& notif = (UpChannelizer::MsgChannelizerNotification&) cmd;

		m_config.m_outputSampleRate = notif.getSampleRate();
		m_config.m_inputFrequencyOffset = notif.getFrequencyOffset();

		apply();

		qDebug() << "NFMMod::handleMessage: MsgChannelizerNotification:"
				<< " m_outputSampleRate: " << m_config.m_outputSampleRate
				<< " m_inputFrequencyOffset: " << m_config.m_inputFrequencyOffset;

		return true;
	}
	else if (MsgConfigureNFMMod::match(cmd))
	{
		MsgConfigureNFMMod& cfg = (MsgConfigureNFMMod&) cmd;

		m_config.m_rfBandwidth = cfg.getRFBandwidth();
		m_config.m_afBandwidth = cfg.getAFBandwidth();
		m_config.m_modFactor = cfg.getModFactor();
		m_config.m_volumeFactor = cfg.getVolumeFactor();
		m_config.m_audioMute = cfg.getAudioMute();
		m_config.m_playLoop = cfg.getPlayLoop();

		apply();

		qDebug() << "NFMMod::handleMessage: MsgConfigureAMMod:"
				<< " m_rfBandwidth: " << m_config.m_rfBandwidth
				<< " m_afBandwidth: " << m_config.m_afBandwidth
				<< " m_modFactor: " << m_config.m_modFactor
                << " m_volumeFactor: " << m_config.m_volumeFactor
				<< " m_audioMute: " << m_config.m_audioMute
				<< " m_playLoop: " << m_config.m_playLoop;

		return true;
	}
	else if (MsgConfigureFileSourceName::match(cmd))
    {
        MsgConfigureFileSourceName& conf = (MsgConfigureFileSourceName&) cmd;
        m_fileName = conf.getFileName();
        openFileStream();
        return true;
    }
    else if (MsgConfigureFileSourceSeek::match(cmd))
    {
        MsgConfigureFileSourceSeek& conf = (MsgConfigureFileSourceSeek&) cmd;
        int seekPercentage = conf.getPercentage();
        seekFileStream(seekPercentage);

        return true;
    }
    else if (MsgConfigureAFInput::match(cmd))
    {
        MsgConfigureAFInput& conf = (MsgConfigureAFInput&) cmd;
        m_afInput = conf.getAFInput();

        return true;
    }
    else if (MsgConfigureFileSourceStreamTiming::match(cmd))
    {
    	std::size_t samplesCount;

    	if (m_ifstream.eof()) {
    		samplesCount = m_fileSize / sizeof(Real);
    	} else {
    		samplesCount = m_ifstream.tellg() / sizeof(Real);
    	}

    	MsgReportFileSourceStreamTiming *report;
        report = MsgReportFileSourceStreamTiming::create(samplesCount);
        getOutputMessageQueue()->push(report);

        return true;
    }
	else
	{
		return false;
	}
}

void NFMMod::apply()
{

	if ((m_config.m_inputFrequencyOffset != m_running.m_inputFrequencyOffset) ||
	    (m_config.m_outputSampleRate != m_running.m_outputSampleRate))
	{
        m_settingsMutex.lock();
		m_carrierNco.setFreq(m_config.m_inputFrequencyOffset, m_config.m_outputSampleRate);
        m_settingsMutex.unlock();
	}

	if((m_config.m_outputSampleRate != m_running.m_outputSampleRate) ||
		(m_config.m_rfBandwidth != m_running.m_rfBandwidth))
	{
		m_settingsMutex.lock();
		m_interpolatorDistanceRemain = 0;
		m_interpolatorConsumed = false;
		m_interpolatorDistance = (Real) m_config.m_audioSampleRate / (Real) m_config.m_outputSampleRate;
        m_interpolator.create(48, m_config.m_audioSampleRate, m_config.m_rfBandwidth / 2.2, 3.0);
		m_settingsMutex.unlock();
	}

	if((m_config.m_afBandwidth != m_running.m_afBandwidth) ||
		(m_config.m_audioSampleRate != m_running.m_audioSampleRate))
	{
		m_settingsMutex.lock();
		m_lowpass.create(21, m_config.m_audioSampleRate, m_config.m_afBandwidth);
		m_settingsMutex.unlock();
	}

	m_running.m_outputSampleRate = m_config.m_outputSampleRate;
	m_running.m_inputFrequencyOffset = m_config.m_inputFrequencyOffset;
	m_running.m_rfBandwidth = m_config.m_rfBandwidth;
	m_running.m_afBandwidth = m_config.m_afBandwidth;
	m_running.m_modFactor = m_config.m_modFactor;
    m_running.m_volumeFactor = m_config.m_volumeFactor;
	m_running.m_audioSampleRate = m_config.m_audioSampleRate;
	m_running.m_audioMute = m_config.m_audioMute;
	m_running.m_playLoop = m_config.m_playLoop;
}

void NFMMod::openFileStream()
{
    if (m_ifstream.is_open()) {
        m_ifstream.close();
    }

    m_ifstream.open(m_fileName.toStdString().c_str(), std::ios::binary | std::ios::ate);
    m_fileSize = m_ifstream.tellg();
    m_ifstream.seekg(0,std::ios_base::beg);

    m_sampleRate = 48000; // fixed rate
    m_recordLength = m_fileSize / (sizeof(Real) * m_sampleRate);

    qDebug() << "AMMod::openFileStream: " << m_fileName.toStdString().c_str()
            << " fileSize: " << m_fileSize << "bytes"
            << " length: " << m_recordLength << " seconds";

    MsgReportFileSourceStreamData *report;
    report = MsgReportFileSourceStreamData::create(m_sampleRate, m_recordLength);
    getOutputMessageQueue()->push(report);
}

void NFMMod::seekFileStream(int seekPercentage)
{
    QMutexLocker mutexLocker(&m_settingsMutex);

    if (m_ifstream.is_open())
    {
        int seekPoint = ((m_recordLength * seekPercentage) / 100) * m_sampleRate;
        seekPoint *= sizeof(Real);
        m_ifstream.clear();
        m_ifstream.seekg(seekPoint, std::ios::beg);
    }
}