#include "dsp/channelmarker.h"

QRgb ChannelMarker::m_colorTable[] = {
	qRgb(0xc0, 0x00, 0x00),
	qRgb(0x00, 0xc0, 0x00),
	qRgb(0x00, 0x00, 0xc0),

	qRgb(0xc0, 0xc0, 0x00),
	qRgb(0xc0, 0x00, 0xc0),
	qRgb(0x00, 0xc0, 0xc0),

	qRgb(0xc0, 0x60, 0x00),
	qRgb(0xc0, 0x00, 0x60),
	qRgb(0x60, 0x00, 0xc0),

	qRgb(0x60, 0x00, 0x00),
	qRgb(0x00, 0x60, 0x00),
	qRgb(0x00, 0x00, 0x60),

	qRgb(0x60, 0x60, 0x00),
	qRgb(0x60, 0x00, 0x60),
	qRgb(0x00, 0x60, 0x60),

	0
};
int ChannelMarker::m_nextColor = 0;

ChannelMarker::ChannelMarker(QObject* parent) :
	QObject(parent),
	m_centerFrequency(0),
	m_bandwidth(0),
    m_oppositeBandwidth(0),
	m_lowCutoff(0),
	m_sidebands(dsb),
	m_visible(false),
	m_highlighted(false),
	m_color(m_colorTable[m_nextColor]),
	m_movable(true)
{
	++m_nextColor;
	if(m_colorTable[m_nextColor] == 0)
		m_nextColor = 0;
}

void ChannelMarker::setTitle(const QString& title)
{
	m_title = title;
	emit changed();
}

void ChannelMarker::setCenterFrequency(int centerFrequency)
{
	m_centerFrequency = centerFrequency;
	emit changed();
}

void ChannelMarker::setBandwidth(int bandwidth)
{
	m_bandwidth = bandwidth;
	emit changed();
}

void ChannelMarker::setOppositeBandwidth(int bandwidth)
{
    m_oppositeBandwidth = bandwidth;
    emit changed();
}

void ChannelMarker::setLowCutoff(int lowCutoff)
{
	m_lowCutoff = lowCutoff;
	emit changed();
}

void ChannelMarker::setSidebands(sidebands_t sidebands)
{
	m_sidebands = sidebands;
	emit changed();
}

void ChannelMarker::setVisible(bool visible)
{
	m_visible = visible;
	emit changed();
}

void ChannelMarker::setHighlighted(bool highlighted)
{
	m_highlighted = highlighted;
	emit changed();
}

void ChannelMarker::setColor(const QColor& color)
{
	m_color = color;
	emit changed();
}
