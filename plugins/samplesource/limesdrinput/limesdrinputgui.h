///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB                                   //
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

#ifndef PLUGINS_SAMPLESOURCE_LIMESDRINPUT_LIMESDRINPUTGUI_H_
#define PLUGINS_SAMPLESOURCE_LIMESDRINPUT_LIMESDRINPUTGUI_H_

#include <QTimer>

#include "plugin/plugingui.h"
#include "limesdrinput.h"

class DeviceSourceAPI;
class FileRecord;

namespace Ui {
    class LimeSDRInputGUI;
}

class LimeSDRInputGUI : public QWidget, public PluginGUI {
    Q_OBJECT

public:
    explicit LimeSDRInputGUI(DeviceSourceAPI *deviceAPI, QWidget* parent = 0);
    virtual ~LimeSDRInputGUI();
    void destroy();

    void setName(const QString& name);
    QString getName() const;

    void resetToDefaults();
    virtual qint64 getCenterFrequency() const;
    virtual void setCenterFrequency(qint64 centerFrequency);
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    virtual bool handleMessage(const Message& message);

private:
    Ui::LimeSDRInputGUI* ui;

    DeviceSourceAPI* m_deviceAPI;
    LimeSDRInput* m_limeSDRInput; //!< Same object as above but gives easy access to LimeSDRInput methods and attributes that are used intensively
    LimeSDRInputSettings m_settings;
    QTimer m_updateTimer;
    QTimer m_statusTimer;
    DeviceSampleSource* m_sampleSource;
    FileRecord *m_fileSink; //!< File sink to record device I/Q output
    int m_sampleRate;
    quint64 m_deviceCenterFrequency; //!< Center frequency in device
    int m_lastEngineState;
    bool m_doApplySettings;
    int m_statusCounter;

    void displaySettings();
    void setNCODisplay();
    void sendSettings();
    void updateSampleRateAndFrequency();
    void blockApplySettings(bool block);

private slots:
    void handleMessagesToGUI();

    void on_startStop_toggled(bool checked);
    void on_record_toggled(bool checked);
    void on_centerFrequency_changed(quint64 value);
    void on_ncoFrequency_changed(quint64 value);
    void on_ncoEnable_toggled(bool checked);
    void on_ncoReset_clicked(bool checked);
    void on_dcOffset_toggled(bool checked);
    void on_iqImbalance_toggled(bool checked);
    void on_sampleRate_changed(quint64 value);
    void on_hwDecim_currentIndexChanged(int index);
    void on_swDecim_currentIndexChanged(int index);
    void on_lpf_changed(quint64 value);
    void on_lpFIREnable_toggled(bool checked);
    void on_lpFIR_changed(quint64 value);
    void on_gain_valueChanged(int value);
    void on_antenna_currentIndexChanged(int index);

    void updateHardware();
    void updateStatus();
};

#endif /* PLUGINS_SAMPLESOURCE_LIMESDRINPUT_LIMESDRINPUTGUI_H_ */
